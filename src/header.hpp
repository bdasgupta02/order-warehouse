#ifndef Header_HPP
#define Header_HPP

#include <stdint.h>

struct Header
{
    unsigned long base_buy;
    unsigned long base_sell;
    unsigned long update_size;
    unsigned long last_trade_qty;
    double last_trade_price;
    uint64_t last_trade_epoch;

    Header() {}

    Header(unsigned int base_buy,
           unsigned int base_sell,
           unsigned int update_size,
           unsigned long last_trade_qty,
           double last_trade_price,
           uint64_t last_trade_epoch)
        : base_buy(base_buy),
          base_sell(base_sell),
          update_size(update_size),
          last_trade_qty(last_trade_qty),
          last_trade_price(last_trade_price),
          last_trade_epoch(last_trade_epoch) {}
};

#endif