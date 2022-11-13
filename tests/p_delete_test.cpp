#include "include/p_delete.hpp"
#include "include/p_insert.hpp"
#include "src/header.hpp"
#include <fstream>
#include <utility>
#include <iostream>
#include <filesystem>

bool delete_sole_order()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    PDelete deleter(conf);

    Order order("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status = inserter.insert(order);
    if (!std::filesystem::exists(status.first))
        return false;

    deleter.delete_order("SCH", 1, 1609722918907107331);

    return !std::filesystem::exists(status.first);
}

bool delete_from_multiple()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    PDelete deleter(conf);

    Order order_1("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.40);
    inserter.insert(order_1);

    Order order_2("SCH", 1609722918907107332, 2, SELL, NEW, 10, 10.60);
    inserter.insert(order_2);

    Order order_3("SCH", 1609722918907107333, 3, SELL, NEW, 10, 10.20);
    std::pair<std::string, bool> status = inserter.insert(order_3);

    deleter.delete_order("SCH", 2, 1609722918907107332);
    if (!std::filesystem::exists(status.first))
        return false;

    std::ifstream fin(status.first, std::ios::in | std::ios::binary);
    Header header;
    fin.read((char *)&header, sizeof(Header));
    if (header.base_buy != 0 || header.base_sell != 0 ||
        header.update_size != 2)
    {
        fin.close();
        return false;
    }

    std::vector<DataOrder> orders;
    for (int i = 0; i < header.update_size; i++)
    {
        DataOrder data;
        fin.read((char *)&data, sizeof(DataOrder));
        orders.push_back(data);
    }

    fin.close();
    return orders[0].epoch == 1609722918907107331 && orders[0].id == 1 &&
           orders[1].epoch == 1609722918907107333 && orders[1].id == 3;
}

bool delete_sole_multiple_files()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    PDelete deleter(conf);

    Order order_1("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.40);
    inserter.insert(order_1);

    Order order_2("SCH", 1629722918907107332, 2, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status = inserter.insert(order_2);

    Order order_3("SCH", 1649722918907107333, 3, SELL, NEW, 10, 10.20);
    std::pair<std::string, bool> status_3 = inserter.insert(order_3);

    std::ifstream fin_3(status_3.first, std::ios::in | std::ios::binary);
    Header header_3;
    fin_3.read((char *)&header_3, sizeof(Header));
    if (header_3.base_buy != 0 || header_3.base_sell != 2 ||
        header_3.update_size != 1)
    {
        fin_3.close();
        return false;
    }
    fin_3.close();

    deleter.delete_order("SCH", 2, 1629722918907107332);
    if (std::filesystem::exists(status.first))
        return false;

    // checking if the deleted file has had an effect into the future
    fin_3 = std::ifstream(status_3.first, std::ios::in | std::ios::binary);
    fin_3.read((char *)&header_3, sizeof(Header));
    if (header_3.base_buy != 0 || header_3.base_sell != 1 ||
        header_3.update_size != 1)
    {
        fin_3.close();
        return false;
    }

    fin_3.close();
    return true;
}

std::pair<int, int> test_deletions()
{
    std::cout << "Testing Deleter...\n";
    std::vector<std::pair<std::string, bool>> tests;
    std::filesystem::remove_all("test-data");

    tests.push_back({"Deleting the sole order for the symbol", delete_sole_order()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Deleting from a file with multiple orders", delete_from_multiple()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Deleting from a file with one order, but multiple files for symbol", delete_sole_multiple_files()});
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

    std::cout << "Finished testing Deleter: " << passed << "/" << tests.size() << " cases passed.\n\n";
    return {passed, tests.size()};
}