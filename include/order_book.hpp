#ifndef OrderBook_HPP
#define OrderBook_HPP

#include <functional>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include "order.hpp"

struct OrderEntry
{
    uint64_t qty;
    double price;

    OrderEntry() {}
    OrderEntry(uint64_t qty, double price) : qty(qty), price(price) {}
};

struct OrderBook
{
    std::unordered_map<double, OrderEntry> buy_map;
    std::unordered_map<double, OrderEntry> sell_map;

    bool empty();
    void add(Order &order);
    std::vector<OrderEntry> buy_list();
    std::vector<OrderEntry> sell_list();
};

#endif