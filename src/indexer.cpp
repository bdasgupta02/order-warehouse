#include "indexer.hpp"
#include <filesystem>
#include <thread>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

EpochIndexer::EpochIndexer(std::string symbol, std::string data_dir, uint64_t epoch_window)
    : symbol(symbol), data_dir(data_dir), epoch_window(epoch_window)
{
    read();
}

EpochIndexer::~EpochIndexer()
{
    flush();

    // can make this periodic
    for (std::thread &thread : flush_threads)
        thread.join();
}

void EpochIndexer::read()
{
    if (!std::filesystem::exists(data_dir))
    {
        std::filesystem::create_directory(data_dir);
    }

    std::string sub_dir = data_dir + symbol + '/';
    if (!std::filesystem::exists(sub_dir))
    {
        std::filesystem::create_directory(sub_dir);
    }

    std::string file_dir = data_dir + symbol + '/' + IDX;
    if (!std::filesystem::exists(file_dir))
        return;

    std::vector<epoch_data> nodes;
    std::ifstream fin(file_dir, std::ios::in | std::ios::binary);

    uint64_t epoch;
    fin.read((char *)&epoch, sizeof(uint64_t));
    epoch_window = epoch;

    idx_header header;
    fin.read((char *)&header, sizeof(idx_header));

    for (int i = 0; i < header; i++)
    {
        uint64_t new_node;
        fin.read((char *)&new_node, sizeof(epoch_data));
        nodes.push_back(new_node);
        
        if (new_node != AVL_EMPTY_NODE)
            epoch_set.insert(new_node);
    }

    avl_tree.deserialize(nodes);
}

void EpochIndexer::flush()
{
    std::lock_guard<std::mutex> guard(flush_mutex);

    std::string file_dir = data_dir + symbol + '/' + IDX;
    std::vector<epoch_data> nodes = avl_tree.serialize();
    std::ofstream fout(file_dir, std::ios::out | std::ios::binary);

    fout.write((char *)&epoch_window, sizeof(uint64_t));

    idx_header tree_size = avl_tree.size();
    fout.write((char *)&tree_size, sizeof(idx_header));

    for (int i = 0; i < tree_size; i++)
        fout.write((char *)&nodes[i], sizeof(epoch_data));

    fout.close();
}

bool EpochIndexer::find(uint64_t epoch)
{
    return epoch_set.count(epoch);
}

void EpochIndexer::add(uint64_t epoch)
{
    avl_tree.insert(epoch);
    epoch_set.insert(epoch);
    flush_threads.push_back(std::thread([&]()
                                        { flush(); }));
}

void EpochIndexer::remove(uint64_t epoch)
{
    avl_tree.erase(epoch);
    epoch_set.erase(epoch);
    flush_threads.push_back(std::thread([&]()
                                        { flush(); }));
}

bool EpochIndexer::exists_higher(uint64_t epoch)
{
    return avl_tree.first_higher(avl_tree.root, epoch) < AVL_EMPTY_NODE;
}

bool EpochIndexer::exists_lower(uint64_t epoch)
{
    return avl_tree.first_lower(avl_tree.root, epoch) < AVL_EMPTY_NODE;
}

bool EpochIndexer::empty() { return avl_tree.empty(); }

std::vector<uint64_t> EpochIndexer::epoch_list()
{
    return avl_tree.serialize_inorder_incl();
}

std::vector<uint64_t> EpochIndexer::epoch_list_from(uint64_t epoch)
{
    return avl_tree.serialize_inorder_from(epoch);
}