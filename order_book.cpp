#include "order_book.hpp"
#include "engine.hpp"
#include "io.hpp"
#include <cassert>
#include <cstdint>

bool price_matched(std::shared_ptr<order> active_order,
                   std::shared_ptr<order> best_order) {
  return active_order->type == BUY ? active_order->price >= best_order->price
                                   : active_order->price <= best_order->price;
}

template <typename PQ>
void add_order_helper(PQ &pq, std::shared_ptr<order> order) {
  bool is_sell = order->type == SELL;
  // the instant at which the order was added to the order book
  uintmax_t output_time = getCurrentTimestamp();
  order->timestamp = output_time;
  pq.insert(order);
  Output::OrderAdded(order->id, order->instrument, order->price, order->count,
                     is_sell, output_time);
}

void order_book::add_order(std::shared_ptr<order> active_order) {
  std::shared_ptr<instrument> instrument = book.get(active_order->instrument);
  if (active_order->type == BUY) {
    add_order_helper(*instrument->buy_pq, active_order);
  } else {
    add_order_helper(*instrument->sell_pq, active_order);
  }
}

template <typename PQ>
bool try_fill_order(PQ &pq, std::shared_ptr<order> active_order) {
  while (active_order->available() && !pq.empty()) {
    // if we are able to get the order, it is guaranteed to be available
    std::shared_ptr<order> best_order = *pq.begin();

    // price is read-only, so we don't need to lock the best order
    if (!price_matched(active_order, best_order)) {
      break;
    }

    // instant at which the AO was matched with the best order and
    // the order book was updated
    uintmax_t output_time = getCurrentTimestamp();
    uintmax_t m = std::min(active_order->count, best_order->count);
    assert(m > 0);
    active_order->count -= m;
    best_order->count -= m;
    Output::OrderExecuted(best_order->id, active_order->id,
                          best_order->execution_id, best_order->price, m,
                          output_time);
    best_order->execution_id++;

    assert(best_order->count >= 0);
    if (best_order->count == 0) {
      // this should succeed since we are the first
      // to see count == 0
      assert(pq.erase(best_order));
    }
  }

  assert(active_order->count >= 0);
  return active_order->count == 0;
}

void order_book::find_match(std::shared_ptr<order> active_order) {
  if (!book.contains(active_order->instrument)) {
    book.try_insert(active_order->instrument, std::make_shared<instrument>());
  }

  std::shared_ptr<instrument> instrument = book.get(active_order->instrument);
  std::unique_lock<std::mutex> lock(instrument->mtx);
  bool fully_filled = false;
  if (active_order->type == SELL) {
    fully_filled = try_fill_order(*instrument->buy_pq, active_order);
  } else {
    fully_filled = try_fill_order(*instrument->sell_pq, active_order);
  }

  if (!fully_filled) {
    add_order(active_order);
  }
}

void order_book::cancel_order(std::shared_ptr<order> order) {
  bool accepted = false;
  std::shared_ptr<instrument> instrument = book.get(order->instrument);
  std::unique_lock<std::mutex> lock(instrument->mtx);
  intmax_t output_time = getCurrentTimestamp();
  if (order->available()) {
    order->cancelled = true;
    accepted = true;
    if (order->type == BUY && instrument->buy_pq->contains(order)) {
      output_time = getCurrentTimestamp();
      instrument->buy_pq->erase(order);
    } else if (order->type == SELL && instrument->sell_pq->contains(order)) {
      output_time = getCurrentTimestamp();
      instrument->sell_pq->erase(order);
    }
  }
  // instant that cancel was accepted or rejected
  Output::OrderDeleted(order->id, accepted, output_time);
}

// Debugging functions
void order_book::print_instr_top(const std::string &instrument_str) {
  if (!book.contains(instrument_str)) {
    std::cerr << "instrument not found: " << instrument_str << std::endl;
    return;
  }
  std::shared_ptr<instrument> instrument = book.get(instrument_str);
  std::shared_ptr<max_pq> max_pq = instrument->buy_pq;
  if (max_pq->empty()) {
    std::cerr << instrument_str << " BUY  top: empty" << std::endl;
  } else {
    std::cerr << instrument_str << " BUY  top: " << *max_pq->begin()
              << std::endl;
  }
  std::shared_ptr<min_pq> min_pq = instrument->sell_pq;
  if (min_pq->empty()) {
    std::cerr << instrument_str << " SELL top: empty" << std::endl;
  } else {
    std::cerr << instrument_str << " SELL top: " << *min_pq->begin()
              << std::endl;
  }
}

void order_book::print_all_top() {
  std::vector<std::string> instruments = book.keys();
  std::cerr << "===============================" << std::endl;
  std::cerr << "Printing top of all instruments" << std::endl;
  for (const std::string &instrument : instruments) {
    print_instr_top(instrument);
  }
  std::cerr << "===============================" << std::endl;
}
