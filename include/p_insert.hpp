#ifndef PInsert_HPP
#define PInsert_HPP

#include "order.hpp"
#include "config.hpp"
#include <string>

static const std::string BUY_STR = "BUY";
static const std::string CANCEL_STR = "CANCEL";
static const std::string NEW_STR = "NEW";
static const std::string TRADE_STR = "TRADE";

struct PInsert
{
    Config *conf;
    
    PInsert();
    PInsert(Config *conf);

    bool ingest_file(std::string source_file, std::string symbol);
    std::pair<std::string, bool> insert(Order &order);
};

#endif