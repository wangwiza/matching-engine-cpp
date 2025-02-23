#pragma once

#include <string>

struct Order {
    uint32_t order_id;
    uint32_t price;
    uint32_t count;
    std::string instrument;
    uint32_t execution_id;

    Order(uint32_t o, uint32_t p, uint32_t c, std::string i, uint32_t e) : order_id(o), price(p), count(c), instrument(i), execution_id(e) {}
};