#include <cassert>
#include <iostream>
#include <memory>
#include <thread>

#include "engine.hpp"
#include "io.hpp"
#include "order_book.hpp"

order_book order_book;

void Engine::accept(ClientConnection connection) {
  auto thread =
      std::thread(&Engine::connection_thread, this, std::move(connection));
  thread.detach();
}

void Engine::connection_thread(ClientConnection connection) {
  // thread local
  std::unordered_map<uint32_t, std::shared_ptr<order>> client_orders;
  while (true) {
    ClientCommand input{};
    switch (connection.readInput(input)) {
    case ReadResult::Error:
      SyncCerr{} << "Error reading input" << std::endl;
    case ReadResult::EndOfFile:
      return;
    case ReadResult::Success:
      break;
    }

    // Functions for printing output actions in the prescribed format are
    // provided in the Output class:
    switch (input.type) {
    // Finally, order cancel requests can only come from the client that
    // originally sent the order – that is, a client cannot cancel an order that
    // did not originate from itself.
    case input_cancel: {
      /*SyncCerr{} << "Got cancel: ID: " << input.order_id << std::endl;*/

      // Remember to take timestamp at the appropriate time, or compute
      // an appropriate timestamp!
      /*auto output_time = getCurrentTimestamp();*/
      /*Output::OrderDeleted(input.order_id, true, output_time);*/
      auto search = client_orders.find(input.order_id);
      if (search == client_orders.end()) {
        // order not found
        auto output_time = getCurrentTimestamp();
        Output::OrderDeleted(input.order_id, false, output_time);
        break;
      }
      std::shared_ptr<order> order = search->second;
      order_book.cancel_order(order);
      break;
    }

    case input_buy:
    case input_sell: {
      /*SyncCerr{} << "Got sell order: " << static_cast<char>(input.type) << "
       * "*/
      /*           << input.instrument << " x " << input.count << " @ "*/
      /*           << input.price << " ID: " << input.order_id << std::endl;*/
      auto order_type = input.type == input_sell ? SELL : BUY;
      auto timestamp = static_cast<uintmax_t>(getCurrentTimestamp());
      std::shared_ptr<order> ptr =
          std::make_shared<order>(input.order_id, input.instrument, input.price,
                                  input.count, order_type, timestamp);
      client_orders[input.order_id] = ptr;
      order_book.find_match(ptr);
      break;
    }

      /*default: {*/
      /*    SyncCerr {} << "Unknown command type: " <<
       * static_cast<char>(input.type) << std::endl;*/
      /*    break;*/
      /*}*/
    }
  }
}
