#include "include/p_update.hpp"
#include "include/config.hpp"
#include "include/order_book.hpp"
#include "include/order.hpp"
#include "header.hpp"
#include "shared.hpp"
#include <mutex>
#include <thread>
#include <filesystem>
#include <fstream>
#include <utility>
#include <stdint.h>

PUpdate::PUpdate(Config *conf) : conf(conf) {}

bool match_polarity(DataOrder &a, DataOrder &b)
{
    bool n_polarity = (a.category == TRADE || a.category == CANCEL) && (b.category == TRADE || b.category == CANCEL);
    bool p_polarity = a.category == NEW && b.category == NEW;
    return n_polarity || p_polarity;
}

// a mock order is created to account for differences, then permeated to future chunk base states in the same propagation
void reconfig_difference(Config *conf, Order &a, DataOrder &b)
{
    Order b_order = Order(a.symbol,
                          b.epoch,
                          b.id,
                          b.side,
                          b.category,
                          b.qty,
                          b.price);

    Order b_rev = reverse_polarity(b_order, b_order.symbol);
    reconfig_ahead(conf, a.epoch, a.symbol, b_rev, a);
}

bool PUpdate::update_order(Order &order)
{
    std::lock_guard<std::mutex> lock(conf->locks[order.symbol]);

    if (!conf->indexes[order.symbol]->find(generate_epoch_window(conf, order.epoch)))
        return false;

    std::pair<std::string, uint64_t> filename =
        generate_filename(conf, order.epoch, order.symbol);

    std::ifstream fin(filename.first, std::ios::in | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));

    OrderBook book;
    for (int i = 0; i < header.base_buy; i++)
    {
        OrderEntry entry;
        fin.read((char *)&entry, sizeof(OrderEntry));
        book.buy_map[entry.price] = entry;
    }

    for (int i = 0; i < header.base_sell; i++)
    {
        OrderEntry entry;
        fin.read((char *)&entry, sizeof(OrderEntry));
        book.sell_map[entry.price] = entry;
    }

    DataOrder stored_order;
    uint64_t start_idx = 0;
    bool is_found = false;
    for (int i = 0; i < header.update_size; i++)
    {
        fin.read((char *)&stored_order, sizeof(DataOrder));
        if (stored_order.id == order.id && stored_order.epoch == order.epoch)
        {
            start_idx = fin.tellg();
            start_idx -= sizeof(DataOrder);
            is_found = true;
            break;
        }
    }

    fin.close();

    if (!is_found || stored_order.epoch != order.epoch || stored_order.id != order.id)
        return false;

    DataOrder new_replacement(order);
    std::fstream fstr(filename.first, std::ios::binary | std::ios::out | std::ios::in);
    fstr.seekp(start_idx);
    fstr.write((char *)&new_replacement, sizeof(DataOrder));
    fstr.close();
    
    if (conf->indexes[order.symbol]->exists_higher(order.epoch))
        reconfig_difference(conf, order, stored_order);

    return true;
}