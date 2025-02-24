// This file contains declarations for the main Engine class. You will
// need to add declarations to this file as you develop your Engine.

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <chrono>
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>

#include "io.hpp"

// Order Linked List
struct Order
{
	uint32_t order_id;
	uint32_t price;
	uint32_t count;
	std::string instrument;
	uint32_t execution_id;
	bool is_sell;

	Order(uint32_t order_id, uint32_t price, uint32_t count, std::string instrument, bool is_sell) : order_id(order_id), price(price), count(count), instrument(instrument), is_sell(is_sell) {}
};

// Store the orders by price
struct PriceNode
{
	uint32_t price;
	std::vector<std::shared_ptr<Order>> orders;
	PriceNode *next;
	std::mutex m;

	PriceNode(uint32_t price = 0) : price(price), next(nullptr) {}
};

struct BuyList
{
	PriceNode head;
	void insert(std::shared_ptr<Order> order);
	void match_order(std::shared_ptr<Order> order);

	BuyList() : head() {}
};

struct SellList
{
	PriceNode head;
	void insert(std::shared_ptr<Order> order);
	void match_order(std::shared_ptr<Order> order);

	SellList() : head() {}
};

class InstrumentBook
{
public:
	BuyList buy_list;
	SellList sell_list;

	void process_sell_order(std::shared_ptr<Order> order);
	void process_buy_order(std::shared_ptr<Order> order);
	void process_cancel_order(std::shared_ptr<Order> order);
};

class OrderBook
{
public:
	InstrumentBook &get_instrument_book(std::string instrument);
	Order &get_order(uint32_t order_id);
	void record_order(std::shared_ptr<Order> order);

private:
	std::unordered_map<std::string, InstrumentBook> instrument_map;
	std::unordered_map<uint32_t, std::shared_ptr<Order>> order_map;
	std::shared_mutex m;
};

// Order Book

// Engine Stuff

struct Engine
{
public:
	void accept(ClientConnection conn);

private:
	void connection_thread(ClientConnection conn);
};

inline std::chrono::microseconds::rep getCurrentTimestamp() noexcept
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

#endif
