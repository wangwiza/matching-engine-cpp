#include <iostream>
#include <thread>

#include "io.hpp"
#include "engine.hpp"
#include "order.hpp"

void Engine::process_buy_order(Order order)
{
	// add instrument if it doesn't exist in the order book yet
	if (!order_book.contains_instrument_book(order.instrument))
	{
		order_book.add_instrument_book(order.instrument);
	}
	InstrumentBook instrument_book = order_book.get_instrument_book(order.instrument);
	
}

void Engine::accept(ClientConnection connection)
{
	auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
	thread.detach();
}

void Engine::connection_thread(ClientConnection connection)
{	
	std::cout << "Hello World!";
	while(true)
	{
		ClientCommand input {};
		switch(connection.readInput(input))
		{
			case ReadResult::Error: SyncCerr {} << "Error reading input" << std::endl;
			case ReadResult::EndOfFile: return;
			case ReadResult::Success: break;
		}

		// Functions for printing output actions in the prescribed format are
		// provided in the Output class:
		switch(input.type)
		{
			case input_cancel: {
				// SyncCerr {} << "Got cancel: ID: " << input.order_id << std::endl;

				// // Remember to take timestamp at the appropriate time, or compute
				// // an appropriate timestamp!
				// auto output_time = getCurrentTimestamp();
				// Output::OrderDeleted(input.order_id, true, output_time);
				process_cancel_order(input.order_id);
				break;
			}

			case input_buy: {
				Order order = Order(input.order_id, input.price, input.count, std::string{input.instrument}, 0);
				process_buy_order(order);
				break;
			}

			case input_sell: {
				Order order = Order(input.order_id, input.price, input.count, std::string{input.instrument}, 0);
				process_sell_order(order);
				break;
			}

			default: {
				SyncCerr {}
				    << "Got order: " << static_cast<char>(input.type) << " " << input.instrument << " x " << input.count << " @ "
				    << input.price << " ID: " << input.order_id << std::endl;

				// Remember to take timestamp at the appropriate time, or compute
				// an appropriate timestamp!
				auto output_time = getCurrentTimestamp();
				Output::OrderAdded(input.order_id, input.instrument, input.price, input.count, input.type == input_sell,
				    output_time);
				break;
			}
		}

		// Additionally:

		// Remember to take timestamp at the appropriate time, or compute
		// an appropriate timestamp!
		intmax_t output_time = getCurrentTimestamp();

		// Check the parameter names in `io.hpp`.
		Output::OrderExecuted(123, 124, 1, 2000, 10, output_time);
	}
}
