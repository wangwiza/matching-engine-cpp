#pragma once

#include "hashmap/hash_map.hpp"
#include <memory>
#include <mutex>
#include <ostream>
#include <set>

enum order_type { BUY, SELL };

class order {
public:
  uintmax_t id;
  std::string instrument;
  uintmax_t price;
  uintmax_t count;
  order_type type;
  uintmax_t timestamp;
  uintmax_t execution_id;
  bool cancelled;

  order(uintmax_t id, const char *instrument, uintmax_t price, uintmax_t count,
        order_type type, uintmax_t timestamp)
      : id(id), instrument(std::string(instrument)), price(price), count(count),
        type(type), timestamp(timestamp), execution_id(1), cancelled(false) {}

  bool available() { return (!cancelled) && count > 0; }

  friend std::ostream &operator<<(std::ostream &os, const order &order) {
    os << order.id << " " << order.instrument << " " << order.price << " "
       << order.count << " " << order.type << " " << order.timestamp << " "
       << order.execution_id << " " << order.cancelled;
    return os;
  }

  bool operator==(const order &other) const { return id == other.id; }
};

// Comparator for low price (min-heap)
struct MinPriceComparator {
  bool operator()(std::shared_ptr<order> a, std::shared_ptr<order> b) const {
    if (a->price == b->price) {
      if (a->timestamp == b->timestamp) {
        return a->id < b->id;
      }
      return a->timestamp < b->timestamp;
    }
    return a->price < b->price;
  }
};

// Comparator for high price (max-heap)
struct MaxPriceComparator {
  bool operator()(std::shared_ptr<order> a, std::shared_ptr<order> b) const {
    if (a->price == b->price) {
      if (a->timestamp == b->timestamp) {
        return a->id < b->id;
      }
      return a->timestamp < b->timestamp;
    }
    return a->price > b->price;
  }
};

// Define specific types for each comparator
using min_pq = std::set<std::shared_ptr<order>, MinPriceComparator>;
using max_pq = std::set<std::shared_ptr<order>, MaxPriceComparator>;

class instrument {
public:
  std::shared_ptr<max_pq> buy_pq;
  std::shared_ptr<min_pq> sell_pq;
  std::mutex mtx;

  instrument()
      : buy_pq(std::make_shared<max_pq>()),
        sell_pq(std::make_shared<min_pq>()) {}
};

class order_book {
private:
  // max_pq for buy orders, min_pq for sell orders
  HashMap<std::string, std::shared_ptr<instrument>> book;

public:
  void add_order(std::shared_ptr<order> order);
  void add_instrument(const std::string &instrument);
  bool instrument_exists(const std::string &instrument);
  void find_match(std::shared_ptr<order> active_order);
  void cancel_order(std::shared_ptr<order> order);
  void print_instr_top(const std::string &instrument_str);
  void print_all_top();
};
