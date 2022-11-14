#include "include/p_insert.hpp"

#include "include/config.hpp"
#include "include/order.hpp"
#include "include/order_book.hpp"
#include "include/p_query.hpp"
#include "shared.hpp"
#include "header.hpp"
#include "indexer.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <set>
#include <sstream>
#include <thread>
#include <utility>

PInsert::PInsert(Config *conf) : conf(conf) {}

void edit_header(std::string &filename, Header &header)
{
    std::fstream fstr(filename, std::ios::binary | std::ios::out | std::ios::in);
    fstr.write((char *)&header, sizeof(Header));
    fstr.close();
}

void write_order(std::ofstream &fout, Order &order)
{
    DataOrder data_order = DataOrder(order);
    fout.write((char *)&data_order, sizeof(DataOrder));
}

Order convert_line_order(std::string file_line)
{
    std::stringstream stream(file_line);
    std::istream_iterator<std::string> begin(stream);
    std::istream_iterator<std::string> end;
    std::vector<std::string> fields(begin, end);

    Side side = fields[3] == BUY_STR ? BUY : SELL;

    Category cat;
    if (fields[4] == CANCEL_STR)
        cat = CANCEL;
    else if (fields[4] == TRADE_STR)
        cat = TRADE;
    else
        cat = NEW;

    Order order(fields[2],
                std::stoull(fields[0]),
                std::stoull(fields[1]),
                side,
                cat,
                std::stoull(fields[6]),
                atof(fields[5].c_str()));

    return order;
}

bool optimized_file_ingestion(Config *conf,
                              std::string source_file,
                              std::string symbol,
                              OrderBook &order_book,
                              Header &header)
{
    std::ifstream source(source_file);

    std::string file_line = "";
    uint64_t chunk_curr_epoch = 0;
    std::string chunk_curr_name = "";
    std::ofstream fout;

    unsigned long last_trade_qty = header.last_trade_qty;
    unsigned long last_trade_price = header.last_trade_price;
    uint64_t last_trade_epoch = header.last_trade_epoch;

    EpochIndexer *idx = conf->get_or_create_index(symbol);

    bool first = true;
    while (std::getline(source, file_line))
    {
        Order line_order = convert_line_order(file_line);
        std::pair<std::string, uint64_t> chunk =
            generate_filename(conf, line_order.epoch, symbol);

        if (chunk.second != chunk_curr_epoch)
        {
            fout.close();

            uint64_t window_start = generate_epoch_window(conf, line_order.epoch);
            idx->add(window_start);

            if (!first)
                edit_header(chunk_curr_name, header);
            else
                first = false;

            fout = std::ofstream(chunk.first, std::ios::out | std::ios::binary);

            // adding header
            header.base_buy = order_book.buy_map.size();
            header.base_sell = order_book.sell_map.size();
            header.update_size = 0;
            header.last_trade_qty = last_trade_qty;
            header.last_trade_price = last_trade_price;
            header.last_trade_epoch = last_trade_epoch;
            write_header(fout, header);

            // adding base
            write_base_book(fout, order_book);

            chunk_curr_epoch = chunk.second;
            chunk_curr_name = chunk.first;
            first = false;
        }

        if (line_order.category == TRADE)
        {
            last_trade_qty = line_order.qty;
            last_trade_price = line_order.price;
            last_trade_epoch = line_order.epoch;
        }

        order_book.add(line_order);
        write_order(fout, line_order);
        header.update_size++;
    }

    fout.close();
    edit_header(chunk_curr_name, header);
    source.close();
    return true;
}

bool ingest_file_not_exists(Config *conf, std::string source_file, std::string symbol)
{
    OrderBook order_book;
    unsigned long last_trade_qty = 0;
    unsigned long last_trade_price = 0;
    uint64_t last_trade_epoch = 0;
    Header header(0, 0, 1, last_trade_qty, last_trade_price, last_trade_epoch);
    return optimized_file_ingestion(conf,
                                    source_file,
                                    symbol,
                                    order_book,
                                    header);
}

bool ingest_file_singularly(PInsert *inserter, std::string source_file, std::string symbol)
{
    std::ifstream source(source_file);

    std::string file_line = "";
    while (std::getline(source, file_line))
    {
        Order line_order = convert_line_order(file_line);
        inserter->insert(line_order);
    }

    source.close();
    return true;
}

// if symbol exists in the data AND all of the data is before
bool ingest_file_exists(PInsert *inserter,
                        std::string source_file,
                        std::string symbol)
{
    std::ifstream source(source_file);
    std::string first_line;
    std::getline(source, first_line);
    source.close();

    Order line_order = convert_line_order(first_line);
    EpochIndexer *idx = inserter->conf->get_or_create_index(symbol);
    if (idx->exists_higher(line_order.epoch))
    {
        PQuery processor(inserter->conf);
        QueryResult query = processor.query_timestamp(line_order.epoch, line_order.symbol);

        unsigned long last_trade_qty = 0;
        unsigned long last_trade_price = 0;
        uint64_t last_trade_epoch = 0;
        Header header(query.book.buy_map.size(),
                      query.book.sell_map.size(),
                      1,
                      last_trade_qty,
                      last_trade_price, last_trade_epoch);

        return optimized_file_ingestion(inserter->conf,
                                        source_file,
                                        symbol,
                                        query.book,
                                        header);
    }
    else
    {
        return ingest_file_singularly(inserter, source_file, symbol);
    }
}

bool PInsert::ingest_file(std::string source_file, std::string symbol)
{
    std::lock_guard<std::mutex> lock(conf->locks[symbol]);
    EpochIndexer *indexer = conf->get_or_create_index(symbol);

    if (indexer->empty())
        return ingest_file_not_exists(conf, source_file, symbol);
    else
        return ingest_file_exists(this, source_file, symbol);
}

bool create_new_file_order(EpochIndexer *indexer, uint64_t window_start, std::string filename, Order &order)
{
    // If category is not NEW for an initial order,
    // it cannot be processed, as there is nothing to trade or cancel
    if (order.category != NEW)
        return false;

    Header header(0, 0, 1, 0, 0, 0);

    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    write_header(fout, header);
    write_order(fout, order);
    fout.close();

    indexer->add(window_start);

    return true;
}

bool create_file_existing_symbol(Config *conf, uint64_t window_start, std::string filename, Order &order)
{
    PQuery processor(conf);
    QueryResult query = processor.query_timestamp(order.epoch, order.symbol);

    unsigned long buy_size = query.book.buy_map.size();
    unsigned long sell_size = query.book.sell_map.size();
    Header header(buy_size, sell_size, 1, query.last_trade_qty, query.last_trade_price, query.last_trade_epoch);

    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    write_header(fout, header);
    write_base_book(fout, query.book);
    write_order(fout, order);
    fout.close();

    conf->indexes[order.symbol]->add(window_start);

    return true;
}

bool add_order_to_file(Config *conf, std::string filename, Order &order)
{
    std::string old_name = filename;
    remove_string_end(4, old_name);
    old_name.append(OLD);
    fs::rename(filename, old_name);

    std::ifstream fin(old_name, std::ios::in | std::ios::binary);
    std::ofstream fout(filename, std::ios::out | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));
    header.update_size++;

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

    write_header(fout, header);
    write_base_book(fout, book);

    // Transfering updates for this file to new copy
    bool is_written = false;
    for (int j = 0; j < header.update_size - 1; j++)
    {
        DataOrder stored_order;
        fin.read((char *)&stored_order, sizeof(DataOrder));

        if (order.epoch < stored_order.epoch)
        {
            write_order(fout, order);
            is_written = true;
        }

        fout.write((char *)&stored_order, sizeof(DataOrder));
    }

    if (!is_written)
    {
        write_order(fout, order);
    }

    fout.close();
    fin.close();
    fs::remove(old_name);

    reconfig_ahead(conf, order.epoch, order.symbol, order);
    return true;
}

std::pair<std::string, bool> PInsert::insert(Order &order)
{
    std::lock_guard<std::mutex> lock(conf->locks[order.symbol]);
    EpochIndexer *indexer = conf->get_or_create_index(order.symbol);

    std::string filename = generate_filename(conf, order.epoch, order.symbol).first;
    uint64_t window_start = generate_epoch_window(conf, order.epoch);

    // if no file for this symbol exists, create a new one
    if (indexer->empty())
    {
        bool success = create_new_file_order(indexer, window_start, filename, order);
        return {filename, success};
    }

    // if epoch's rightful file is before any existing window, create a new one and
    // reformat the next ones
    if (!indexer->exists_lower(order.epoch))
    {
        bool success = create_new_file_order(indexer, window_start, filename, order);
        reconfig_ahead(conf, order.epoch, order.symbol, order);
        return {filename, success};
    }

    // if order is after any existing window (and its limits), query latest base and
    // create a new file
    if (!indexer->exists_higher(window_start))
    {
        bool success = create_file_existing_symbol(conf, window_start, filename, order);
        return {filename, success};
    }

    if (indexer->find(window_start))
    {
        bool success = add_order_to_file(conf, filename, order);
        return {filename, success};
    }

    if (indexer->exists_lower(window_start) && indexer->exists_higher(window_start))
    {
        bool success = create_file_existing_symbol(conf, window_start, filename, order);
        reconfig_ahead(conf, order.epoch, order.symbol, order);
        return {filename, success};
    }

    return {filename, false};
}