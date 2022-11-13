#include "src/indexer.hpp"
#include <utility>
#include <filesystem>
#include <iostream>

bool test_addition_new()
{
    EpochIndexer indexer("SCH", "test-data/", 1800000000000);

    indexer.add(100);
    indexer.add(200);
    indexer.add(300);

    return indexer.find(100) &&
           indexer.find(200) &&
           indexer.find(300) &&
           !indexer.find(400);
}

bool test_removal()
{
    EpochIndexer indexer("SCH", "test-data/", 1800000000000);

    indexer.add(100);

    if (!indexer.find(100))
        return false;

    indexer.remove(100);

    return !indexer.find(100);
}

bool test_exists_higher()
{
    EpochIndexer indexer("SCH", "test-data/", 1800000000000);
    uint64_t test = 100;

    if (indexer.exists_higher(100))
        return false;

    indexer.add(120);

    if (indexer.find(100))
        return false;

    return indexer.exists_higher(100);
}

bool test_exists_lower()
{
    EpochIndexer indexer("SCH", "test-data/", 1800000000000);
    uint64_t test = 100;

    if (indexer.exists_lower(100))
        return false;

    indexer.add(90);

    if (indexer.find(100))
        return false;

    return indexer.exists_lower(100);
}

bool test_indexer_empty()
{
    EpochIndexer indexer("SCH", "test-data/", 1800000000000);
    if (!indexer.empty())
        return false;

    indexer.add(100);

    return !indexer.empty();
}

std::pair<int, int> test_indexer()
{
    std::cout << "Testing Indexer...\n";
    std::vector<std::pair<std::string, bool>> tests;
    std::filesystem::remove_all("test-data");

    tests.push_back({"Adding elements into a new index", test_addition_new()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Removing an element from a new index", test_removal()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Checking if a higher epoch exists", test_exists_higher()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Checking if a lower epoch exists", test_exists_lower()});
    std::filesystem::remove_all("test-data");

    tests.push_back({"Testing empty function for indexer", test_indexer_empty()});
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

    std::cout << "Finished testing Indexer: " << passed << "/" << tests.size() << " cases passed.\n\n";
    return {passed, tests.size()};
}