#pragma once

#include <set>
#include "Order.h"

// Comparator for SellBook (max-heap by price, then by timestamp)
struct SellComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price != b.price) {
            return a.price > b.price; // Higher price first
        } else {
            return a.timestamp < b.timestamp; // Earlier timestamp first
        }
    }
};

class SellBook {
    public:
        SellBook();
        void add_order(const Order& order);
        void remove(uint32_t order_id);
        Order peek();
        Order pop();
        bool empty();
    private:
        std::multiset<Order, SellComparator> sell_book;
        std::mutex sell_book_mutex;
};