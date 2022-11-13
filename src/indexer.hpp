#ifndef EpochIndexer_HPP
#define EpochIndexer_HPP

#include "avl_tree.hpp"
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <stdint.h>

static const std::string IDX = "IDX.dat";

// Manages all indexes in an AVL BST
// Carries the risk of having another instance open for index race conditions
class EpochIndexer
{
    std::vector<std::thread> flush_threads;
    std::mutex flush_mutex; // still fine-grained as every symbol has its own indexer
    std::string symbol;
    std::string data_dir;
    uint64_t epoch_window;

    void read();
    void flush();

public:
    AVLTree<uint64_t> avl_tree;

    typedef size_t idx_header;
    typedef uint64_t epoch_data;
    
    EpochIndexer(std::string symbol, std::string data_dir, uint64_t epoch_window);
    ~EpochIndexer();

    bool find(uint64_t epoch);
    void add(uint64_t epoch);
    void remove(uint64_t epoch);
    bool exists_higher(uint64_t epoch);
    bool exists_lower(uint64_t epoch);
    bool empty();
    std::vector<uint64_t> epoch_list();
    std::vector<uint64_t> epoch_list_from(uint64_t epoch);
};

#endif