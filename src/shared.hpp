#ifndef Shared_HPP
#define Shared_HPP

#include "include/order_book.hpp"
#include "include/config.hpp"
#include "header.hpp"
#include "indexer.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <type_traits>
#include <utility>

static const std::string OLD = "_OLD.dat";
namespace fs = std::filesystem;

void remove_string_end(int times, std::string &source);
uint64_t generate_epoch_window(Config *conf, uint64_t epoch);
std::pair<std::string, uint64_t> generate_filename(Config *conf, uint64_t epoch, std::string symbol);
void write_header(std::ofstream &fout, Header &header);
void write_base_book(std::ofstream &fout, OrderBook &book);

template <typename T>
Order reverse_polarity(T &data, std::string symbol)
{
    Order reversed(symbol, data.epoch, data.id, data.side, data.category, data.qty, data.price);

    if (reversed.category == NEW)
        reversed.category = CANCEL;
    else
        reversed.category = NEW;

    return reversed;
}

template <typename... Args>
using all_same = std::conjunction<std::is_same<Order, Args>...>;

// Takes one or more orders and permeates them through future chunk files - to modify base states
template <typename... Args, typename = std::enable_if_t<all_same<Order, Args...>::value, void>>
void reconfig_ahead(Config *conf, uint64_t epoch, std::string symbol, Args... args)
{

    uint64_t timestamp_from = generate_epoch_window(conf, epoch);
    EpochIndexer *idx = conf->get_or_create_index(symbol);
    std::vector<uint64_t> epochs = idx->epoch_list_from(timestamp_from);

    unsigned long last_traded_qty = 0;
    double last_traded_price = 0;
    uint64_t last_trade_epoch = 0;

    // under the assumption that only one of the given orders will be trade
    // as 2 orders with the same polarity will never be sent here
    bool is_traded = false;
    for (auto &order : {args...})
        if (order.category == TRADE)
        {
            if (is_traded)
                return;

            last_traded_qty = order.qty;
            last_traded_price = order.price;
            last_trade_epoch = order.epoch;
            is_traded = true;
        }

    uint32_t i = 0;
    while (i < epochs.size())
    {
        std::string old_name = generate_filename(conf, epochs[i], symbol).first;
        std::string cached_name = old_name;
        remove_string_end(4, old_name);
        old_name.append(OLD);
        fs::rename(cached_name, old_name);

        std::ifstream fin(old_name, std::ios::in | std::ios::binary);
        std::ofstream fout(cached_name, std::ios::out | std::ios::binary);

        // need to edit the header
        Header header;
        fin.read((char *)&header, sizeof(Header));

        // Transferring base state for buy and sell orders
        OrderBook book;
        for (int j = 0; j < header.base_buy; j++)
        {
            OrderEntry entry;
            fin.read((char *)&entry, sizeof(OrderEntry));
            book.buy_map[entry.price] = entry;
        }

        for (int j = 0; j < header.base_sell; j++)
        {
            OrderEntry entry;
            fin.read((char *)&entry, sizeof(OrderEntry));
            book.sell_map[entry.price] = entry;
        }

        for (auto order : {args...})
            book.add(order);    

        header.base_buy = book.buy_map.size();
        header.base_sell = book.sell_map.size();
        if (last_trade_epoch > header.last_trade_epoch)
        {
            header.last_trade_epoch = last_trade_epoch;
            header.last_trade_price = last_traded_price;
            header.last_trade_qty = last_traded_qty;
        }

        write_header(fout, header);
        write_base_book(fout, book);

        // Transfering updates for this file to new copy
        for (int j = 0; j < header.update_size; j++)
        {
            DataOrder stored_order;
            fin.read((char *)&stored_order, sizeof(DataOrder));
            fout.write((char *)&stored_order, sizeof(DataOrder));
        }

        fout.close();
        fin.close();
        fs::remove(old_name);
        i++;
    }
}

#endif