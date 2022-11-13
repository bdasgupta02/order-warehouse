#ifndef QueryResult_HPP
#define QueryResult_HPP

#include "order_book.hpp"
#include <stdint.h>

struct QueryResult
{
    OrderBook book;
    uint64_t last_trade_epoch = 0;
    unsigned long last_trade_qty = 0;
    double last_trade_price = 0;

    QueryResult() {}

    QueryResult(OrderBook book,
                uint64_t last_trade_epoch,
                unsigned long last_trade_qty,
                double last_trade_price)
        : book(book),
          last_trade_epoch(last_trade_epoch),
          last_trade_price(last_trade_price),
          last_trade_qty(last_trade_qty) {}

    inline bool empty() { return book.empty(); }
};

#endif