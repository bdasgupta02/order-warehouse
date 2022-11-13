#include "include/order_book.hpp"

#include <iostream>
#include <utility>

bool test_insert_new_order_buy()
{
    OrderBook order_book;

    Order order("SCH", 1609722918907107331, 1, BUY, NEW, 10, 10.60);
    order_book.add(order);

    if (order_book.buy_map.empty())
        return false;
    if (!order_book.buy_map.count(order.price))
        return false;
    if (order_book.sell_map.count(order.price))
        return false;

    OrderEntry entry = order_book.buy_map[order.price];

    return entry.qty == order.qty && entry.price == order.price;
}

bool test_insert_new_order_sell()
{
    OrderBook order_book;

    Order order("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.60);
    order_book.add(order);

    if (order_book.sell_map.empty())
        return false;
    if (!order_book.sell_map.count(order.price))
        return false;
    if (order_book.buy_map.count(order.price))
        return false;

    OrderEntry entry = order_book.sell_map[order.price];

    return entry.qty == order.qty && entry.price == order.price;
}

bool test_insert_cancel()
{
    OrderBook order_book;

    Order order("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.60);
    order_book.add(order);

    if (!order_book.sell_map.count(order.price))
        return false;

    Order cancel_order("SCH", 1609722918907107336, 1, SELL, CANCEL, 10, 10.60);
    order_book.add(cancel_order);

    if (order_book.sell_map.count(order.price))
        return false;
    return true;
}

bool test_insert_trade()
{
    OrderBook order_book;

    Order order("SCH", 1609722918907107331, 1, BUY, NEW, 10, 10.60);
    order_book.add(order);

    if (!order_book.buy_map.count(order.price))
        return false;

    Order cancel_order_1("SCH", 1609722918907107336, 1, BUY, TRADE, 5, 10.60);
    order_book.add(cancel_order_1);

    if (!order_book.buy_map.count(order.price))
        return false;
    if (order_book.buy_map[order.price].qty != 5)
        return false;

    Order cancel_order_2("SCH", 1609722918907107338, 1, BUY, TRADE, 5, 10.60);
    order_book.add(cancel_order_2);

    if (order_book.buy_map.count(order.price))
        return false;
    return true;
}

std::pair<int, int> test_orderbook()
{
    std::cout << "Testing Order Book...\n";
    std::vector<std::pair<std::string, bool>> tests;

    tests.push_back({"Inserting new buy order into order book", test_insert_new_order_buy()});

    tests.push_back({"Inserting new buy order into order book", test_insert_new_order_sell()});

    tests.push_back({"Inserting cancel order into order book", test_insert_cancel()});

    tests.push_back({"Inserting trade order into order book", test_insert_trade()});

    int passed = 0;
    for (std::pair<std::string, bool> test : tests)
    {
        if (test.second)
        {
            std::cout << "PASSED: " << test.first << "\n";
            passed++;
        }
        else
        {
            std::cout << "FAILED: " << test.first << "\n";
        }
    }

    std::cout << "Finished testing Order Book: " << passed << "/" << tests.size()
              << " cases passed.\n\n";
    return {passed, tests.size()};
}
