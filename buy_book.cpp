#include "buy_book.hpp"

BuyBook::BuyBook() {}

void BuyBook::add_order(const Order& order) {
    std::lock_guard<std::mutex> lock(buy_book_mutex);
    buy_book.insert(order);
}

void BuyBook::remove(uint32_t order_id) {
    std::lock_guard<std::mutex> lock(buy_book_mutex);
    auto it = std::find_if(buy_book.begin(), buy_book.end(), [order_id](const Order& order) {
        return order.order_id == order_id;
    });
    if (it != buy_book.end()) {
        buy_book.erase(it);
    }
}

Order BuyBook::peek() {
    std::lock_guard<std::mutex> lock(buy_book_mutex);
    return *buy_book.begin();
}

Order BuyBook::pop() {
    std::lock_guard<std::mutex> lock(buy_book_mutex);
    Order order = *buy_book.begin();
    buy_book.erase(buy_book.begin());
    return order;
}

bool BuyBook::empty() {
    std::lock_guard<std::mutex> lock(buy_book_mutex);
    return buy_book.empty();
}

