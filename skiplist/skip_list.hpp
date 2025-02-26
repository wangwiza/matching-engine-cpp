#include <atomic>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <random>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

template <typename T, typename Comp = std::less<T>> class skip_list {
private:
  enum node_type { SENTINEL_HEAD, NORMAL, SENTINEL_TAIL };

  struct Node {
    T value;
    std::vector<std::shared_ptr<Node>> next;
    node_type type;
    std::shared_mutex mutex;

    explicit Node(T val, node_type type)
        : value(val), next(), type(type) {}
  };

  uint32_t max_level;
  std::shared_ptr<Node> head = std::make_shared<Node>(T(), SENTINEL_HEAD);
  std::shared_ptr<Node> tail = std::make_shared<Node>(T(), SENTINEL_TAIL);
  std::atomic<uint64_t> size;
  Comp comp;

  bool random_bool() {
    thread_local std::mt19937 rng([] {
      std::random_device rd;
      return rd();
    }());
    return std::uniform_int_distribution{0, 1}(rng);
  }

  uint32_t random_level() {
    uint32_t level = 0;
    while (random_bool() && level < max_level_index())
      level++;
    return level;
  }

  // returns true if value is strictly less than node's value
  bool is_less_than(const T &value, const std::shared_ptr<Node> &node) const {
    // tail is equivalent to +inf
    if (node->type == SENTINEL_TAIL)
      return true;
    // head is equivalent to -inf
    if (node->type == SENTINEL_HEAD)
      return false;
    return comp(value, node->value);
  }

  bool is_equal(const T &value, const std::shared_ptr<Node> &node) const {
    if (node->type == SENTINEL_HEAD || node->type == SENTINEL_TAIL)
      return false;
    return !comp(value, node->value) && !comp(node->value, value);
  }

  std::shared_ptr<Node> get_node(const T &value) const {
    std::shared_ptr<Node> current = head;
    std::shared_ptr<Node> next = nullptr;
    for (int64_t level = max_level_index(); level >= 0; level--) {
      // move right while not strictly less than value on the right
      next = current->next[level];
      while (next) {
        if (is_equal(value, next)) {
          return next;
        }
        if (is_less_than(value, next)) {
          break;
        }
        current = next;
        next = current->next[level];
      }
      // decrement level if current node is strictly less than value
    }
    return nullptr;
  }

  // Finds the preceding node of value in the skip list at the target_level
  std::shared_ptr<Node> get_largest_smaller_node(const T &value, uint32_t target_level) const {
    std::shared_ptr<Node> curr = head;
    std::shared_ptr<Node> next = nullptr;
    for (int64_t level = max_level_index(); level >= target_level; level--) {
      // move right while not strictly less than value on the right
      next = curr->next[level];
      while (next) {
        // it is possible for us to see the current value as the next value
        // since we are using this for remove as well
        if (is_less_than(value, next) || is_equal(value, next)) {
          break;
        }
        curr = next;
        next = curr->next[level];
      }
      // decrement level if current node is strictly less than value
    }
    assert(curr != nullptr);
    assert(next != nullptr);
    return curr;
  }

  uint32_t max_level_index() const { return max_level - 1; }

public:
  skip_list(uint32_t max_lvl = 64, Comp cmp = Comp())
      : max_level(max_lvl), comp(cmp) {
    head->next.resize(max_level, tail);
    tail->next.resize(max_level, nullptr);
  }

  bool empty() const { return size.load() == 0; }

  // get the smallest element in the skip list
  T &get_head() const {
    if (empty()) {
      throw std::out_of_range("Skip list is empty");
    }
    std::shared_ptr<Node> result = head->next[0];
    std::shared_lock<std::shared_mutex> head_lock(result->mutex);
    return result->value;
  }

  void add(const T &value) {
    uint32_t new_level = random_level();
    std::shared_ptr<Node> new_node = std::make_shared<Node>(value, NORMAL);
    new_node->next.resize(max_level);

    // Now we have all the necessary write locks for nodes we need to modify
    for (uint32_t level = 0; level <= new_level; level++) {
      std::shared_ptr<Node> prev_node = get_largest_smaller_node(value, level);
      std::shared_ptr<Node> next_node = prev_node->next[level];

      // setup the node itself
      new_node->next[level] = next_node;
      prev_node->next[level] = new_node;
    }

    size.fetch_add(1);
    // All locks will be automatically released when vectors go out of scope
  }

  bool contains(const T &value) const {
    std::shared_ptr<Node> target = get_node(value);
    return target != nullptr;
  }

  T &get(const T &value) const {
    std::shared_ptr<Node> target = get_node(value);
    if (!target) {
      throw std::out_of_range("Value not found in skip list");
    }
    return target->value;
  }

  bool remove(const T &value) {
    std::shared_ptr<Node> target = get_node(value);
    if (!target)
      return false;

    for (int64_t level = target->next.size() - 1; level >= 0; level--) {
      // we can skip the levels with nullptr
      if (!target->next[level])
        continue;

      std::shared_ptr<Node> prev_node = get_largest_smaller_node(value, level);
      prev_node->next[level] = target->next[level];
    }
    // No need to delete target, shared_ptr will handle cleanup
    size.fetch_sub(1);
    return true;
  }

  // for debugging purposes
  void display_internals() {
    for (int64_t level = max_level_index(); level >= 0; level--) {
      std::shared_ptr<Node> current = head;
      if (head->next[level]->type == SENTINEL_TAIL)
        continue;
      std::cerr << "Level " << level << ": ";
      while (current) {
        if (current->type == SENTINEL_HEAD)
          std::cerr << "-inf ";
        else if (current->type == SENTINEL_TAIL) {
          std::cerr << "inf ";
          break;
        } else
          std::cerr << current->value << " ";
        current = current->next[level];
      }
      std::cerr << std::endl;
    }
  }
};
