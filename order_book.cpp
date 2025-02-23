#include "order_book.hpp"

OrderBook::OrderBook() {}

bool OrderBook::contains_instrument_book(std::string instrument) {
    std::shared_lock lock(order_book_mutex);
    return order_book.find(instrument_id) != order_book.end();
}

void OrderBook::add_instrument_book(std::string instrument) {
    std::unique_lock lock(order_book_mutex);
    order_book[instrument] = InstrumentBook();
}

InstrumentBook OrderBook::get_instrument_book(std::string instrument) {
    std::shared_lock lock(order_book_mutex);
    return order_book[instrument];
}

