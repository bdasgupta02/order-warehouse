#ifndef PDelete_HPP
#define PDelete_HPP

#include "config.hpp"
#include <mutex>
#include <string>
#include <unordered_map>
#include <stdint.h>

class PDelete
{
    Config *conf;

public:
    PDelete(Config *conf);

    bool delete_order(std::string symbol, uint64_t id, uint64_t epoch);
};

#endif