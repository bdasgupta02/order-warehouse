#ifndef PUpdate_HPP
#define PUpdate_HPP

#include "config.hpp"
#include "order.hpp"

class PUpdate
{
    Config *conf;

public:
    PUpdate(Config *conf);

    bool update_order(Order &order);
};

#endif