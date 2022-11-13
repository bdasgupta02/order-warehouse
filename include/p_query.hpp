#ifndef PQuery_HPP
#define PQuery_HPP

#include <string>
#include <utility>
#include <vector>
#include <stdint.h>

#include "query_result.hpp"
#include "config.hpp"

struct PQuery
{
    Config *conf;

    PQuery(Config *conf);

    QueryResult query_timestamp(uint64_t epoch, std::string symbol);
    std::vector<QueryResult> query_multiple(const std::vector<uint64_t> &epochs, std::string symbol);
};

#endif