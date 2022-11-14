#include "include/p_query.hpp"
#include "include/order_book.hpp"
#include "header.hpp"
#include "shared.hpp"
#include "indexer.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

PQuery::PQuery(Config *conf) : conf(conf) {}

QueryResult proc_for_epoch(Config *conf, uint64_t epoch, uint64_t file_epoch, std::string symbol)
{
    std::pair<std::string, uint64_t> file_pair =
        generate_filename(conf, file_epoch, symbol);

    std::ifstream fin(file_pair.first, std::ios::in | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));

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

    for (int j = 0; j < header.update_size; j++)
    {
        DataOrder stored_order;
        fin.read((char *)&stored_order, sizeof(DataOrder));
        if (epoch < stored_order.epoch)
            break;

        if (stored_order.category == TRADE)
        {
            header.last_trade_epoch = stored_order.epoch;
            header.last_trade_qty = stored_order.qty;
            header.last_trade_price = stored_order.price;
        }

        Order new_order =
            Order(symbol, stored_order.epoch, stored_order.id, stored_order.side,
                  stored_order.category, stored_order.qty, stored_order.price);

        book.add(new_order);
    }

    fin.close();
    return QueryResult(book,
                       header.last_trade_epoch,
                       header.last_trade_qty,
                       header.last_trade_price);
}

QueryResult get_base_epoch(Config *conf, uint64_t epoch, uint64_t file_epoch, std::string symbol)
{
    std::pair<std::string, uint64_t> file_pair =
        generate_filename(conf, file_epoch, symbol);

    std::ifstream fin(file_pair.first, std::ios::in | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));

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

    fin.close();
    return QueryResult(book, header.last_trade_epoch, header.last_trade_qty, header.last_trade_price);
}

QueryResult PQuery::query_timestamp(uint64_t epoch, std::string symbol)
{
    if (!fs::exists(conf->data_dir + symbol + "/"))
        return QueryResult();

    // calculating
    EpochIndexer *idx = conf->get_or_create_index(symbol);
    std::vector<uint64_t> file_epochs = idx->epoch_list();

    // if there are no orders on or before this epoch, empty result is returned
    if (file_epochs.empty() || epoch < file_epochs[0])
        return QueryResult();

    QueryResult query_result;

    // if the epoch is after all files, calculate the base and return result
    if (epoch > file_epochs[file_epochs.size() - 1] + conf->epoch_window)
    {
        query_result = proc_for_epoch(conf, epoch, file_epochs[file_epochs.size() - 1], symbol);
    }
    else
    {
        unsigned int i = 0;
        while (i < file_epochs.size())
        {
            uint64_t file_epoch = file_epochs[i];

            // if epoch is within one file, then search in that file -
            // or if epoch is in middle of two files, get base from the next
            if (epoch >= file_epoch && epoch < file_epoch + conf->epoch_window)
            {
                query_result = proc_for_epoch(conf, epoch, file_epoch, symbol);
                break;
            }
            else if (epoch < file_epoch) {
                query_result = get_base_epoch(conf, epoch, file_epoch, symbol);
                break;
            }
            
            i++;
        }
    }
    
    return query_result;
}

std::vector<QueryResult> PQuery::query_multiple(const std::vector<uint64_t> &epochs, std::string symbol)
{
    std::vector<QueryResult> result;
    for (uint64_t epoch : epochs)
    {
        QueryResult query = query_timestamp(epoch, symbol);
        result.push_back(query);
    }
    return result;
}
