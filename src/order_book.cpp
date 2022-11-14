#include "include/order_book.hpp"
#include <algorithm>

void add_new(OrderBook *book, Order &order)
{
    if (order.side == BUY)
    {
        if (book->buy_map.count(order.price))
        {
            OrderEntry old = book->buy_map[order.price];
            old.qty += order.qty;
            book->buy_map[order.price] = old;
        }
        else
        {
            book->buy_map[order.price] = OrderEntry(order.qty, order.price);
        }
    }
    else
    {
        if (book->sell_map.count(order.price))
        {
            OrderEntry old = book->sell_map[order.price];
            old.qty += order.qty;
            book->sell_map[order.price] = old;
        }
        else
        {
            book->sell_map[order.price] = OrderEntry(order.qty, order.price);
        }
    }
}

void remove_qty(OrderBook *book, Order &order)
{
    OrderEntry old;

    if (order.side == BUY && book->buy_map.count(order.price))
    {
        OrderEntry old = book->buy_map[order.price];

        if (order.qty >= old.qty)
        {
            book->buy_map.erase(order.price);
            return;
        }

        old.qty -= order.qty;
        book->buy_map[order.price] = old;
    }
    else if (book->sell_map.count(order.price))
    {
        OrderEntry old = book->sell_map[order.price];

        if (order.qty >= old.qty)
        {
            book->sell_map.erase(order.price);
            return;
        }

        old.qty -= order.qty;
        book->sell_map[order.price] = old;
    }
}

void OrderBook::add(Order &order)
{
    if (order.category == NEW)
    {
        add_new(this, order);
    }
    else if (order.category == CANCEL)
    {
        remove_qty(this, order);
    }
    else if (order.category == TRADE)
    {
        remove_qty(this, order);
    }
}

bool OrderBook::empty() { return buy_map.empty() && sell_map.empty(); }

std::vector<OrderEntry> OrderBook::buy_list()
{
    std::vector<OrderEntry> buy_vect;

    for (auto &entry_pair : buy_map)
        buy_vect.push_back(entry_pair.second);

    return buy_vect;
}

std::vector<OrderEntry> OrderBook::sell_list()
{
    std::vector<OrderEntry> sell_vect;

    for (auto &entry_pair : sell_map)
        sell_vect.push_back(entry_pair.second);

    return sell_vect;
}