#pragma once

#include <set>
#include "Order.h"

// Comparator for BuyBook (min-heap by price, then by timestamp)
struct BuyComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price != b.price) {
            return a.price < b.price; // Lower price first
        } else {
            return a.timestamp < b.timestamp; // Earlier timestamp first
        }
    }
};

class BuyBook {
    public:
        BuyBook();
        void add_order(const Order& order);
        void remove(uint32_t order_id);
        Order peek();
        Order pop();
        bool empty();
    private:
        std::multiset<Order, BuyComparator> buy_book;
        std::mutex buy_book_mutex;
};