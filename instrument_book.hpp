#pragma once

#include <string>
#include <mutex>

#include "buy_book.hpp"
#include "sell_book.hpp"
#include "order.hpp"

class InstrumentBook {
    public:
        InstrumentBook ();
        void execute_buy_order(Order buy_order);
        void execute_sell_order(Order sell_order);
        void execute_cancel_order(uint32_t order_id);
    private:
        SellBook sell_book;
        BuyBook buy_book;
        std::mutex sell_mutex;
        std::mutex buy_mutex;
        void match_buy_against_sell(Order buy_order);
        void match_sell_against_buy(Order sell_order);
        void add_buy_order(Order buy_order);
        void add_sell_order(Order sell_order);
}