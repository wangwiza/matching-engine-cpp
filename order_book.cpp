#include "order_book.hpp"
#include "engine.hpp"
#include "io.hpp"
#include <cassert>

bool price_matched(std::shared_ptr<order> active_order,
                   std::shared_ptr<order> best_order) {
  return active_order->type == BUY ? active_order->price >= best_order->price
                                   : active_order->price <= best_order->price;
}

template <typename PQ>
void add_order_helper(PQ &pq, std::shared_ptr<order> order) {
  auto id = order->id;
  auto instrument = order->instrument;
  auto price = order->price;
  auto count = order->count;
  auto is_sell = order->type == SELL;
  assert(count > 0 && !order->cancelled);
  order->resting = true;
  pq.add(order);
  // the instant at which the order was added to the order book
  uintmax_t output_time = getCurrentTimestamp();
  Output::OrderAdded(id, instrument, price, count, is_sell, output_time);
}

void order_book::add_order(std::shared_ptr<order> active_order) {
  std::pair<max_sl, min_sl> instrument = book.get(active_order->instrument);
  add_order_helper(active_order->type == BUY ? instrument->second.first
                                             : instrument->second.second,
                   active_order);
}

template <typename SL>
bool try_fill_order(SL &sl, std::shared_ptr<order> active_order) {
  // maintain an invariant that if an order is in the pq, it is available
  // meaning, count > 0 and not cancelled
  while (active_order->available() && !sl.empty()) {
    std::shared_ptr<order> best_order = sl.get_head();
    // price is read-only, so we don't need to lock the best order
    if (!price_matched(active_order, best_order)) {
      break;
    }

    std::unique_lock<std::mutex> bo_wlock(best_order->mutex);
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
      sl.remove(best_order);
    }
  }

  assert(active_order->count >= 0);
  return active_order->count == 0;
}

void order_book::find_match(std::shared_ptr<order> active_order) {
  if (!book.contains(active_order->instrument)) {
    book.try_insert(active_order->instrument,
                    std::make_pair(max_sl{}, min_sl{}));
  }

  std::pair<max_sl, min_sl> instrument = book.get(active_order->instrument);
  bool fully_filled = false;
  if (active_order->type == SELL) {
    fully_filled = try_fill_order(instrument.first, active_order);
  } else {
    fully_filled = try_fill_order(instrument.second, active_order);
  }

  // turn the AO into a RO
  if (!fully_filled) {
    add_order(active_order);
  }
}

void order_book::cancel_order(std::shared_ptr<order> order) {
  bool accepted = false;
  // need to lock in case the order has become an resting order
  std::unique_lock<std::mutex> wlock(order->mutex);
  if (order->available()) {
    order->cancelled = true;
    accepted = true;
    // remove the order from the order book
    // if it is already a resting order
    if (order->resting) {
      std::pair<max_sl, min_sl> instrument = book.get(order->instrument);
      if (order->type == BUY) {
        instrument.second.remove(order);
      } else {
        instrument.first.remove(order);
      }
    }
  }
  // instant that cancel was accepted or rejected
  auto output_time = getCurrentTimestamp();
  Output::OrderDeleted(order->id, accepted, output_time);
}

void order_book::print_top(std::shared_ptr<order> order) {
  if (!book.contains(order->instrument)) {
    std::cerr << "instrument not found: " << order->instrument << std::endl;
    return;
  }
  std::pair<max_sl, min_sl> instrument = book.get(order->instrument);
  max_sl &max_pq = instrument.second.first;
  if (max_pq.empty()) {
    std::cerr << "instrument is empty" << std::endl;
    return;
  }
  std::cerr << "instrument BUY top: " << max_pq.get_head() << std::endl;
  min_sl &min_pq = instrument.second.second;
  if (min_pq.empty()) {
    std::cerr << "instrument is empty" << std::endl;
    return;
  }
  std::cerr << "instrument SELL top: " << min_pq.get_head() << std::endl;
}
