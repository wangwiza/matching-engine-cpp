#include <unordered_map>
#include <string>
#include <mutex>

#include "instrument_book.hpp"

class OrderBook {
    public:
        OrderBook();
        bool contains_instrument_book(std::string instrument);
        void add_instrument_book(std::string instrument);
        InstrumentBook get_instrument_book(std::string instrument);
    private:
        std::unordered_map<std::string, InstrumentBook> order_book;
        std::shared_mutex order_book_mutex;
};