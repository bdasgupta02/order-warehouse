#include "include/p_update.hpp"
#include "include/config.hpp"
#include "include/p_insert.hpp"
#include "src/header.hpp"
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>

bool test_update_one()
{
    Config *conf = new Config("test-data/");
    PInsert inserter(conf);
    PUpdate updater(conf);

    Order order("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status = inserter.insert(order);

    order.side = BUY;
    order.category = TRADE;
    order.price = 20.20;
    order.qty = 900;
    if (!updater.update_order(order))
        return false;

    std::ifstream fin(status.first, std::ios::in | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));
    if (header.base_buy != 0 || header.base_sell != 0 ||
        header.update_size != 1)
    {
        fin.close();
        return false;
    }

    DataOrder data_order;
    fin.read((char *)&data_order, sizeof(DataOrder));

    bool is_correct = data_order.epoch == 1609722918907107331 &&
                      data_order.id == 1 &&
                      data_order.side == BUY &&
                      data_order.category == TRADE &&
                      data_order.qty == 900 &&
                      data_order.price == 20.20;

    fin.close();
    return is_correct;
}

bool test_update_reconfig()
{
    Config *conf = new Config("test-data/", 200);
    PInsert inserter(conf);
    PUpdate updater(conf);

    Order order1("SCH", 100, 1, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status1 = inserter.insert(order1);

    Order order2("SCH", 300, 2, BUY, NEW, 10, 10.60);
    std::pair<std::string, bool> status2 = inserter.insert(order2);
    std::ifstream fin(status2.first, std::ios::in | std::ios::binary);
    Header header;
    fin.read((char *)&header, sizeof(Header));
    fin.close();

    if (header.base_buy != 0 || header.base_sell != 1 || header.update_size != 1)
        return false;

    order1.side = BUY;
    order1.price = 20.55;
    updater.update_order(order1);

    std::ifstream fin2(status2.first, std::ios::in | std::ios::binary);
    Header header2;
    fin2.read((char *)&header2, sizeof(Header));
    OrderBook book;
    OrderEntry entry;
    fin2.read((char *)&entry, sizeof(OrderEntry));
    book.buy_map[entry.price] = entry;
    fin2.close();

    std::ifstream fin3(status1.first, std::ios::in | std::ios::binary);
    Header header3;
    fin3.read((char *)&header3, sizeof(Header));
    fin3.close();

    if (header2.base_buy != 1 || header2.base_sell != 0 || header2.update_size != 1)
        return false;
    
    if (header3.base_buy != 0 || header3.base_sell != 0 || header3.update_size != 1)
        return false;
    
    return book.buy_map[order1.price].price == order1.price;
}

bool test_update_many_reconfig()
{
    Config *conf = new Config("test-data/", 200);
    PInsert inserter(conf);
    PUpdate updater(conf);

    Order order1("SCH", 100, 1, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status1 = inserter.insert(order1);

    Order order2("SCH", 300, 2, BUY, NEW, 10, 10.60);
    std::pair<std::string, bool> status2 = inserter.insert(order2);

    Order order3("SCH", 500, 2, BUY, TRADE, 10, 10.60);
    std::pair<std::string, bool> status3 = inserter.insert(order3);

    std::ifstream fin(status2.first, std::ios::in | std::ios::binary);
    Header header;
    fin.read((char *)&header, sizeof(Header));
    fin.close();

    if (header.base_buy != 0 || header.base_sell != 1 || header.update_size != 1)
        return false;
    
    fin = std::ifstream(status3.first, std::ios::in | std::ios::binary);
    fin.read((char *)&header, sizeof(Header));
    fin.close();

    if (header.base_buy != 1 || header.base_sell != 1 || header.update_size != 1)
        return false;

    order1.side = BUY;
    order1.price = 20.55;
    updater.update_order(order1);

    fin = std::ifstream(status2.first, std::ios::in | std::ios::binary);
    fin.read((char *)&header, sizeof(Header));
    fin.close();

    if (header.base_buy != 1 || header.base_sell != 0 || header.update_size != 1)
        return false;

    fin = std::ifstream(status3.first, std::ios::in | std::ios::binary);
    fin.read((char *)&header, sizeof(Header));
    fin.close();

    return header.base_buy == 2 && header.base_sell == 0 && header.update_size == 1;
}

std::pair<int, int> test_updates()
{
    std::cout << "Testing Updater...\n";
    std::vector<std::pair<std::string, bool>> tests;
    std::filesystem::remove_all("test-data");

    tests.push_back({"Updating one order", test_update_one()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Reconfiguring future chunk on update", test_update_reconfig()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Reconfiguring multiple future chunks on update", test_update_many_reconfig()});
    std::filesystem::remove_all("test-data");

    int passed = 0;
    for (std::pair<std::string, bool> &test : tests)
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

    std::cout << "Finished testing Updater: " << passed << "/" << tests.size() << " cases passed.\n\n";
    return {passed, tests.size()};
}