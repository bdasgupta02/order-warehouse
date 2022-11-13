#ifndef Order_HPP
#define Order_HPP

#include <string>

enum Side
{
    SELL,
    BUY,
};

enum Category
{
    TRADE,
    NEW,
    CANCEL,
};

struct Order
{
    std::string symbol;
    uint64_t epoch;
    uint64_t id;
    Side side;
    Category category;
    uint32_t qty;
    double price;

    Order() {}

    Order(std::string symbol,
          uint64_t epoch,
          uint64_t id,
          Side side,
          Category category,
          uint32_t qty,
          double price)
        : symbol(symbol),
          epoch(epoch),
          id(id),
          side(side),
          category(category),
          qty(qty),
          price(price) {}
};

struct DataOrder
{
    uint64_t epoch;
    uint64_t id;
    Side side;
    Category category;
    uint64_t qty;
    double price;

    DataOrder(Order &order)
    {
        epoch = order.epoch;
        id = order.id;
        side = order.side;
        category = order.category;
        qty = order.qty;
        price = order.price;
    }

    DataOrder() {}
};

#endif