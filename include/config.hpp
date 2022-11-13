#ifndef Config_HPP
#define Config_HPP

#include "../src/indexer.hpp"
#include <mutex>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <stdint.h>

// Default chunk window of 10 minutes
static const uint64_t TEN_MINUTES = 600000000000;

struct Config
{
    std::unordered_map<std::string, std::mutex> locks;
    std::unordered_map<std::string, EpochIndexer *> indexes;
    std::string data_dir;
    const uint64_t epoch_window; // has to be constant for all usage

    Config(std::string data_dir, uint64_t epoch_window) : data_dir(data_dir), epoch_window(epoch_window)
    {
        if (!std::filesystem::exists(data_dir))
            std::filesystem::create_directory(data_dir);
    }

    Config(std::string data_dir) : data_dir(data_dir), epoch_window(TEN_MINUTES)
    {
        if (!std::filesystem::exists(data_dir))
            std::filesystem::create_directory(data_dir);
    }

    EpochIndexer *get_or_create_index(std::string symbol)
    {
        if (indexes.find(symbol) == indexes.end())
        {
            indexes[symbol] = new EpochIndexer(symbol, data_dir, epoch_window);
        }
        return indexes[symbol];
    }
};

struct ConfigData
{
    uint64_t epoch_window;

    ConfigData(Config &conf) : epoch_window(conf.epoch_window) {}
};

#endif