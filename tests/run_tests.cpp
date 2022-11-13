#include "p_insert_test.cpp"
#include "p_query_test.cpp"
#include "p_delete_test.cpp"
#include "avl_tree_test.cpp"
#include "indexer_test.cpp"
#include "p_update_test.cpp"
#include "order_book_test.cpp"

int main()
{
    std::pair<int, int> total = {0, 0};

    std::pair<int, int> avl = test_avl();
    total.first += avl.first;
    total.second += avl.second;

    std::pair<int, int> indexer = test_indexer();
    total.first += indexer.first;
    total.second += indexer.second;

    std::pair<int, int> book = test_orderbook();
    total.first += book.first;
    total.second += book.second;

    std::pair<int, int> inserter = test_insertions();
    total.first += inserter.first;
    total.second += inserter.second;

    std::pair<int, int> query = test_queries();
    total.first += query.first;
    total.second += query.second;

    std::pair<int, int> deleter = test_deletions();
    total.first += deleter.first;
    total.second += deleter.second;

    std::pair<int, int> updater = test_updates();
    total.first += updater.first;
    total.second += updater.second;

    std::cout << "Finished all testing: " << total.first << "/" << total.second << " cases passed.\n\n";
    return 0;
}