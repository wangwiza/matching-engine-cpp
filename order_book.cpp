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
  auto id = order->id;
  auto instrument = order->instrument;
  auto price = order->price;
  auto count = order->count;
  auto is_sell = order->type == SELL;
  assert(count > 0 && !order->cancelled);
  // the instant at which the order was added to the order book
  std::shared_lock<std::shared_mutex> pq_rlock(pq.mutex);
  uintmax_t output_time = getCurrentTimestamp();
  order->timestamp = output_time;
  pq.add(order);
  Output::OrderAdded(id, instrument, price, count, is_sell, output_time);
}

void order_book::add_order(std::shared_ptr<order> active_order) {
  std::shared_ptr<instrument> instrument = book.get(active_order->instrument);
  if (active_order->type == BUY) {
    add_order_helper(*instrument->buy_sl, active_order);
  } else {
    add_order_helper(*instrument->sell_sl, active_order);
  }
}

template <typename SL>
bool try_fill_order(SL &sl, std::shared_ptr<order> active_order) {
  while (active_order->available() && !sl.empty()) {
    std::shared_ptr<order> best_order;
    std::unique_lock<std::shared_mutex> sl_wlock(sl.mutex);
    try {
      best_order = sl.get_head();
    } catch (std::out_of_range &e) {
      // this may happen since there is time between
      // the time we check if the SL is empty and the
      // time we actually get the head
      break;
    }
    // price is read-only, so we don't need to lock the best order
    if (!price_matched(active_order, best_order)) {
      break;
    }

    std::unique_lock<std::mutex> bo_wlock(best_order->mutex);
    // we have to check if the best order is still available
    // since between the time where we decide to fight for
    // this order's write lock and now, this order could have
    // become unavailabe
    if (!best_order->available()) {
      continue;
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
      bool removed = sl.remove(best_order);
      assert(removed);
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
  std::shared_ptr<sl_bridge> bridge = instrument->bridge;
  bool fully_filled = false;
  if (active_order->type == SELL) {
    bridge->enter_sell();
    fully_filled = try_fill_order(*instrument->buy_sl, active_order);
  } else {
    bridge->enter_buy();
    fully_filled = try_fill_order(*instrument->sell_sl, active_order);
  }

  // turn the AO into a RO
  if (!fully_filled) {
    add_order(active_order);
  }
  if (active_order->type == SELL) {
    bridge->exit_sell();
  } else {
    bridge->exit_buy();
  }
}

void order_book::cancel_order(std::shared_ptr<order> order) {
  bool accepted = false;
  // need to lock in case the order has become an resting order
  std::unique_lock<std::mutex> wlock(order->mutex);
  intmax_t output_time = getCurrentTimestamp();
  if (order->available()) {
    order->cancelled = true;
    accepted = true;
    // remove the order from the order book
    // if it is already a resting order
    std::shared_ptr<instrument> instrument = book.get(order->instrument);
    if (order->type == BUY && instrument->buy_sl->contains(order)) {
      std::shared_lock<std::shared_mutex> sl_rlock(instrument->buy_sl->mutex);
      output_time = getCurrentTimestamp();
      instrument->buy_sl->remove(order);
    } else if (order->type == SELL && instrument->sell_sl->contains(order)) {
      std::shared_lock<std::shared_mutex> sl_rlock(instrument->sell_sl->mutex);
      output_time = getCurrentTimestamp();
      instrument->sell_sl->remove(order);
    }
  }
  // instant that cancel was accepted or rejected
  Output::OrderDeleted(order->id, accepted, output_time);
}

void order_book::print_instr_top(const std::string &instrument_str) {
  if (!book.contains(instrument_str)) {
    std::cerr << "instrument not found: " << instrument_str << std::endl;
    return;
  }
  std::shared_ptr<instrument> instrument = book.get(instrument_str);
  std::shared_ptr<max_sl> max_pq = instrument->buy_sl;
  if (max_pq->empty()) {
    std::cerr << instrument_str << " BUY  top: empty" << std::endl;
  } else {
    std::cerr << instrument_str << " BUY  top: " << *max_pq->get_head()
              << std::endl;
  }
  std::shared_ptr<min_sl> min_pq = instrument->sell_sl;
  if (min_pq->empty()) {
    std::cerr << instrument_str << " SELL top: empty" << std::endl;
  } else {
    std::cerr << instrument_str << " SELL top: " << *min_pq->get_head()
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
