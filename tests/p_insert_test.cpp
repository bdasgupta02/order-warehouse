#include "include/p_insert.hpp"
#include "include/p_query.hpp"
#include "src/header.hpp"
#include "src/shared.hpp"
#include <fstream>
#include <iostream>
#include <utility>

bool test_insert_new_when_not_exists()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert i(conf);

    Order order("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status = i.insert(order);

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

    bool is_order = data_order.epoch == 1609722918907107331 &&
                    data_order.id == 1 &&
                    data_order.side == SELL &&
                    data_order.category == NEW &&
                    data_order.qty == 10 &&
                    data_order.price == 10.60;
    fin.close();
    return is_order;
}

bool test_ingest_file_small()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    inserter.ingest_file("tests/test-ingest/test_small.log", "SCH");

    std::ifstream fin("test-data/SCH/1609731000000000000.dat", std::ios::in | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));

    if (header.base_buy != 0 || header.base_sell != 0 ||
        header.update_size != 4)
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

    bool check_second = orders[1].epoch == 1609732197102157041 &&
                        orders[1].id == 7374421476726859286 &&
                        orders[1].price == 107.29 &&
                        orders[1].qty == 2 &&
                        orders[1].side == SELL &&
                        orders[1].category == NEW;

    fin.close();
    return check_second;
    return false;
}

bool test_chunking_split()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert i(conf);
    i.ingest_file("tests/test-ingest/test.log", "SCH");

    std::ifstream fin("test-data/SCH/1609997400000000000.dat", std::ios::in | std::ios::binary);

    Header header;
    fin.read((char *)&header, sizeof(Header));
    if (header.base_buy != 2 || header.base_sell != 2 || header.update_size != 2)
    {
        fin.close();
        return false;
    }

    fin.close();
    return true;
}

bool test_reformat_next_file()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert i(conf);

    // first order
    Order order_1("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status = i.insert(order_1);

    // checking first file's validity
    std::ifstream fin_1(status.first, std::ios::in | std::ios::binary);
    Header header_1;
    fin_1.read((char *)&header_1, sizeof(Header));
    if (header_1.base_buy != 0 || header_1.base_sell != 0 ||
        header_1.update_size != 1)
    {
        fin_1.close();
        return false;
    }

    fin_1.close();

    // second order
    Order order_2("SCH", 100, 2, SELL, NEW, 10, 10.60);
    reconfig_ahead(conf, order_2.epoch, order_2.symbol, order_2);

    // checking changed validity after reformating base forward
    std::ifstream fin_2(status.first, std::ios::in | std::ios::binary);
    Header header_2;
    fin_2.read((char *)&header_2, sizeof(Header));
    if (header_2.base_buy != 0 || header_2.base_sell != 1 ||
        header_2.update_size != 1)
    {
        fin_2.close();
        return false;
    }

    // checking changed base state data
    OrderEntry entry;
    fin_2.read((char *)&entry, sizeof(OrderEntry));
    if (entry.qty != 10 || entry.price != 10.60)
    {
        fin_2.close();
        return false;
    }

    // checking update data
    DataOrder update;
    fin_2.read((char *)&update, sizeof(DataOrder));

    fin_2.close();
    return update.epoch == 1609722918907107331 &&
           update.category == NEW &&
           update.side == SELL &&
           update.price == 10.90 &&
           update.qty == 10 &&
           update.id == 1;
}

bool test_insert_existing_file()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);

    // first order
    Order order_1("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status_1 = inserter.insert(order_1);

    // checking first file's validity
    std::ifstream fin_1(status_1.first, std::ios::in | std::ios::binary);
    Header header_1;
    fin_1.read((char *)&header_1, sizeof(Header));
    if (header_1.base_buy != 0 || header_1.base_sell != 0 ||
        header_1.update_size != 1)
    {
        fin_1.close();
        return false;
    }

    fin_1.close();

    // third order
    Order order_3("SCH", 1609722928907107334, 3, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status_3 = inserter.insert(order_3);

    // checking first file's validity
    std::ifstream fin_3(status_3.first, std::ios::in | std::ios::binary);
    Header header_3;
    fin_3.read((char *)&header_3, sizeof(Header));
    if (header_3.base_buy != 0 || header_3.base_sell != 0 ||
        header_3.update_size != 2)
    {
        fin_3.close();
        return false;
    }

    fin_3.close();

    // second order
    Order order_2("SCH", 1609722918907107333, 2, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status_2 = inserter.insert(order_2);

    // checking second file's validity
    std::ifstream fin_2(status_2.first, std::ios::in | std::ios::binary);
    Header header_2;
    fin_2.read((char *)&header_2, sizeof(Header));
    if (header_2.base_buy != 0 || header_2.base_sell != 0 ||
        header_2.update_size != 3)
    {
        fin_2.close();
        return false;
    }

    std::vector<DataOrder> orders;
    for (int i = 0; i < 4; i++)
    {
        DataOrder data;
        fin_2.read((char *)&data, sizeof(DataOrder));
        orders.push_back(data);
    }

    fin_2.close();

    if (orders[0].epoch != 1609722918907107331 ||
        orders[1].epoch != 1609722918907107333 ||
        orders[2].epoch != 1609722928907107334)
    {
        return false;
    }

    return true;
}

bool test_insert_later_new_file()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);

    // first order
    Order order_1("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status_1 = inserter.insert(order_1);

    // checking first file's validity
    std::ifstream fin_1(status_1.first, std::ios::in | std::ios::binary);
    Header header_1;
    fin_1.read((char *)&header_1, sizeof(Header));
    if (header_1.base_buy != 0 || header_1.base_sell != 0 ||
        header_1.update_size != 1)
    {
        fin_1.close();
        return false;
    }

    fin_1.close();

    // second order
    Order order_2("SCH", 1629722918907107333, 2, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status_2 = inserter.insert(order_2);

    // checking second file's validity
    std::ifstream fin_2(status_2.first, std::ios::in | std::ios::binary);
    Header header_2;
    fin_2.read((char *)&header_2, sizeof(Header));
    if (header_2.base_buy != 0 || header_2.base_sell != 1 ||
        header_2.update_size != 1)
    {
        fin_2.close();
        return false;
    }

    return status_1.first != status_2.first;
}

bool test_insert_middle()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);

    // first order
    Order order_1("SCH", 1609722918907107331, 1, SELL, NEW, 10, 10.90);
    std::pair<std::string, bool> status_1 = inserter.insert(order_1);

    // checking first file's validity
    std::ifstream fin_1(status_1.first, std::ios::in | std::ios::binary);
    Header header_1;
    fin_1.read((char *)&header_1, sizeof(Header));
    if (header_1.base_buy != 0 || header_1.base_sell != 0 ||
        header_1.update_size != 1)
    {
        fin_1.close();
        return false;
    }

    fin_1.close();

    // third order
    Order order_3("SCH", 1639722928907107334, 3, SELL, NEW, 10, 10.30);
    std::pair<std::string, bool> status_3 = inserter.insert(order_3);

    // checking first file's validity
    std::ifstream fin_3(status_3.first, std::ios::in | std::ios::binary);
    Header header_3;
    fin_3.read((char *)&header_3, sizeof(Header));
    if (header_3.base_buy != 0 || header_3.base_sell != 1 ||
        header_3.update_size != 1)
    {
        fin_3.close();
        return false;
    }

    fin_3.close();

    // second order
    Order order_2("SCH", 1629722918907107333, 2, SELL, NEW, 10, 10.60);
    std::pair<std::string, bool> status_2 = inserter.insert(order_2);

    // checking second file's validity
    std::ifstream fin_2(status_2.first, std::ios::in | std::ios::binary);
    Header header_2;
    fin_2.read((char *)&header_2, sizeof(Header));
    if (header_2.base_buy != 0 || header_2.base_sell != 1 ||
        header_2.update_size != 1)
    {
        fin_2.close();
        return false;
    }

    fin_2.close();

    // rechecking 3 to see if it has been reformatted with the insertion of 2 in
    // between
    fin_3 = std::ifstream(status_3.first, std::ios::in | std::ios::binary);
    fin_3.read((char *)&header_3, sizeof(Header));
    if (header_3.base_buy != 0 || header_3.base_sell != 2 ||
        header_3.update_size != 1)
    {
        std::cout << header_3.base_sell << " 3\n";
        fin_3.close();
        return false;
    }

    fin_3.close();
    return true;
}

std::pair<int, int> test_insertions()
{
    std::cout << "Testing Inserter...\n";
    std::vector<std::pair<std::string, bool>> tests;
    std::filesystem::remove_all("test-data");

    tests.push_back({"Inserting new order to new file when symbol does not exist", test_insert_new_when_not_exists()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Ingesting small .log file to data storage", test_ingest_file_small()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Ingesting chunk split", test_chunking_split()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Reformat base data of following file", test_reformat_next_file()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Inserting new order to new file when symbol exists", test_insert_existing_file()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Inserting file to existing file", test_insert_later_new_file()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Inserting new order in middle of 2 files", test_insert_middle()});
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

    std::cout << "Finished testing Inserter: " << passed << "/" << tests.size() << " cases passed.\n\n";
    return {passed, tests.size()};
}