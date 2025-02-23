// This file contains declarations for the main Engine class. You will
// need to add declarations to this file as you develop your Engine.

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <chrono>
#include "order.hpp"

#include "io.hpp"
#include "order_book.hpp"

struct Engine
{
public:
	void accept(ClientConnection conn);

private:
	OrderBook order_book;
	void connection_thread(ClientConnection conn);
	void process_buy_order(Order order);
	void process_sell_order(Order order);
	void process_cancel_order(uint32_t order_id);
};

inline std::chrono::microseconds::rep getCurrentTimestamp() noexcept
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

#endif
