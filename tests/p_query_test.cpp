#include "include/p_query.hpp"
#include "include/p_insert.hpp"
#include <utility>
#include <iostream>
#include <vector>
#include <filesystem>

bool test_single_query_inside_file()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    inserter.ingest_file("tests/test-ingest/test_small.log", "SCH");
    PQuery query(conf);

    QueryResult res = query.query_timestamp(1609732199727381553, "SCH");

    return res.book.buy_map.size() == 0 && res.book.sell_map.size() == 2;
}

bool test_single_query_middle()
{
    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    inserter.ingest_file("tests/test-ingest/test_small.log", "SCH");
    PQuery query(conf);

    QueryResult res = query.query_timestamp(1609732199727380552, "SCH");

    return res.book.buy_map.size() == 1 && res.book.sell_map.size() == 2;
}

bool test_multiple_epochs()
{
    std::vector<uint64_t> epoch_list = {
        1609732197102157041,
        1609732198544838732,
        1609934199727381552,
    };

    Config *conf = new Config("test-data/", 1800000000000);
    PInsert inserter(conf);
    inserter.ingest_file("tests/test-ingest/test_small.log", "SCH");
    PQuery query(conf);

    std::vector<QueryResult> res = query.query_multiple(epoch_list, "SCH");

    return res[0].book.sell_map.size() == 2 && res[0].book.buy_map.size() == 0 &&
           res[1].book.sell_map.size() == 2 && res[1].book.buy_map.size() == 1 &&
           res[2].book.sell_map.size() == 4 && res[2].book.buy_map.size() == 2;
}

std::pair<int, int> test_queries()
{
    std::cout << "Testing Query Processor...\n";
    std::vector<std::pair<std::string, bool>> tests;
    std::filesystem::remove_all("test-data");

    tests.push_back({"Querying within a file for an epoch", test_single_query_inside_file()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Querying in middle of two files for an epoch", test_single_query_middle()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Querying multiple epochs", test_multiple_epochs()});
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

    std::cout << "Finished testing Query Processor: " << passed << "/" << tests.size() << " cases passed.\n\n";
    return {passed, tests.size()};
}