#include "include/p_delete.hpp"
#include "include/config.hpp"
#include "include/order_book.hpp"
#include "header.hpp"
#include "shared.hpp"
#include <filesystem>
#include <fstream>
#include <stdint.h>
#include <utility>
#include <mutex>
#include <thread>

PDelete::PDelete(Config *conf) : conf(conf) {}

bool PDelete::delete_order(std::string symbol, uint64_t id, uint64_t epoch)
{
    std::lock_guard<std::mutex> lock(conf->locks[symbol]);
    EpochIndexer *idx = conf->get_or_create_index(symbol);

    if (!idx->find(generate_epoch_window(conf, epoch)))
        return false;

    std::pair<std::string, uint64_t> filename =
        generate_filename(conf, epoch, symbol);

    std::string old_name = filename.first;
    remove_string_end(4, old_name);
    old_name.append(OLD);
    fs::rename(filename.first, old_name);

    std::ifstream fin(old_name, std::ios::in | std::ios::binary);
    std::ofstream fout(filename.first, std::ios::out | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));
    header.update_size--;

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

    if (header.update_size > 0)
    {
        write_header(fout, header);
        write_base_book(fout, book);
    }

    unsigned int j = 0;
    DataOrder found_order;
    bool is_found = false;
    while (j < header.update_size + 1)
    {
        DataOrder stored_order;
        fin.read((char *)&stored_order, sizeof(DataOrder));

        // skip this
        if (!is_found && stored_order.id == id && stored_order.epoch == epoch)
        {
            found_order = stored_order;
            j++;
            is_found = true;
            continue;
        }

        // cannot find order
        if (!is_found && stored_order.epoch > epoch)
            return false;

        j++;
        if (header.update_size > 0)
            fout.write((char *)&stored_order, sizeof(DataOrder));
    }

    fout.close();
    fin.close();
    fs::remove(old_name);

    Order reversed = reverse_polarity(found_order, symbol);
    reconfig_ahead(conf, reversed.epoch, reversed.symbol, reversed);

    if (header.update_size == 0)
        fs::remove(filename.first);

    return true;
}