#include <atomic>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <random>
#include <stdexcept>

// 64 should be enough for everyone since the ideal height of a skip list is
// log(n)
const uint32_t MAX_LEVEL = 64;
const uint32_t MAX_LEVEL_INDEX = MAX_LEVEL - 1;

template <typename T, typename Comp = std::less<T>> class skip_list {
private:
  enum node_type { SENTINEL_HEAD, NORMAL, SENTINEL_TAIL };

  struct Node {
    // safe concurrent access to value has to be handled by the user
    T value;
    std::atomic<std::shared_ptr<Node>> next[MAX_LEVEL];
    // following fields are only written to at initialization so no need for
    // atomic
    uint32_t max_level_idx;
    node_type type;

    explicit Node(T val, uint32_t lvl_idx, node_type type)
        : value(val), max_level_idx(lvl_idx), type(type) {
      // Initialize the vector with max_level atomic shared_ptrs, each
      // containing nullptr
      for (uint32_t i = 0; i < MAX_LEVEL; i++) {
        next[i].store(nullptr);
      }
    }
  };

  std::atomic<std::shared_ptr<Node>> head;
  std::atomic<std::shared_ptr<Node>> tail;
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
    while (random_bool() && level < MAX_LEVEL_INDEX)
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
    std::shared_ptr<Node> current = head.load();
    std::shared_ptr<Node> next = nullptr;
    for (int64_t level = MAX_LEVEL_INDEX; level >= 0; level--) {
      // move right while not strictly less than value on the right
      next = current->next[level].load();
      while (next) {
        if (is_equal(value, next)) {
          return next;
        }
        if (is_less_than(value, next)) {
          break;
        }
        current = next;
        next = current->next[level].load();
      }
      // decrement level if current node is strictly less than value
    }
    return nullptr;
  }

  // Finds the preceding node of value in the skip list at the target_level
  std::shared_ptr<Node> get_preceding_node(const T &value,
                                           uint32_t target_level) const {
    std::shared_ptr<Node> curr = head.load();
    std::shared_ptr<Node> next = nullptr;
    for (int64_t level = MAX_LEVEL_INDEX; level >= target_level; level--) {
      // move right while not strictly less than value on the right
      next = curr->next[level].load();
      while (next) {
        // it is possible for us to see the current value as the next value
        // since we are using this for remove as well
        if (is_less_than(value, next) || is_equal(value, next)) {
          break;
        }
        curr = next;
        next = curr->next[level].load();
      }
      // decrement level if current node is strictly less than value
    }
    assert(curr != nullptr);
    assert(next != nullptr);
    return curr;
  }

public:
  skip_list(Comp cmp = Comp()) : comp(cmp) {
    auto head_node =
        std::make_shared<Node>(T(), MAX_LEVEL_INDEX, SENTINEL_HEAD);
    auto tail_node =
        std::make_shared<Node>(T(), MAX_LEVEL_INDEX, SENTINEL_TAIL);

    for (uint32_t i = 0; i < MAX_LEVEL; ++i) {
      head_node->next[i].store(tail_node);
    }

    head.store(std::move(head_node));
    tail.store(std::move(tail_node));
  }

  bool empty() const { return head.load()->next[0].load() == tail.load(); }

  // get the smallest element in the skip list
  T &get_head() const {
    if (empty()) {
      throw std::out_of_range("Skip list is empty");
    }
    std::shared_ptr<Node> result = head.load()->next[0].load();
    return result->value;
  }

  T &get(const T &value) const {
    std::shared_ptr<Node> target = get_node(value);
    if (!target) {
      throw std::out_of_range("Value not found in skip list");
    }
    return target->value;
  }

  bool contains(const T &value) const {
    std::shared_ptr<Node> target = get_node(value);
    return target != nullptr;
  }

  void add(const T &value) {
    uint32_t new_level = random_level();
    std::shared_ptr<Node> new_node =
        std::make_shared<Node>(value, new_level, NORMAL);

    for (uint32_t level = 0; level <= new_level; level++) {
      bool valid_next_node = false;
      std::shared_ptr<Node> prev_node;
      std::shared_ptr<Node> next_node;
      do {
        prev_node = get_preceding_node(value, level);
        next_node = prev_node->next[level].load();
        // There might be a case where a node whose value is < value is inserted
        // after the preceding node is found, so we need to check if the next
        // node is greater than or equal to value
        valid_next_node = is_less_than(value, next_node);
        // If this happens then we have duplicate values, which is not allowed
        assert(!is_equal(value, next_node));
        if (valid_next_node) {
          new_node->next[level].store(next_node);
        }
      } while (
          !valid_next_node ||
          !prev_node->next[level].compare_exchange_weak(next_node, new_node));
    }
  }

  bool remove(const T &value) {
    std::shared_ptr<Node> target = get_node(value);
    if (target == nullptr)
      return false;

    for (int64_t level = target->max_level_idx; level >= 0; level--) {
      assert(target->next[level].load() != nullptr);

      std::shared_ptr<Node> prev_node = get_preceding_node(value, level);
      std::shared_ptr<Node> next_node = target->next[level].load();
      while (!prev_node->next[level].compare_exchange_weak(target, next_node)) {
        // reload target since it would've been overwritten
        target = get_node(value);
        // this means someone else removed the node
        if (target == nullptr)
          return false;
        // the prev_node might have changed, so we need to find it again
        prev_node = get_preceding_node(value, level);
        next_node = target->next[level].load();
      }
    }
    return true;
  }

  // for debugging purposes
  void display_node(const std::shared_ptr<Node> &node) {
    std::cout << "========================================" << std::endl;
    std::cout << "Value: " << node->value << std::endl;
    std::cout << "Max Level: " << node->max_level_idx << std::endl;
    for (int64_t i = node->max_level_idx; i > 0; i--) {
      std::cout << "Level " << i << ": ";
      if (node->next[i].load() == nullptr)
        std::cout << "nullptr";
      else
        std::cout << node->next[i].load()->value;
      std::cout << std::endl;
    }
    std::cout << "========================================" << std::endl;
  }

  // for debugging purposes
  void display_internals(uint64_t n) {
    for (int64_t level = MAX_LEVEL_INDEX; level >= 0; level--) {
      uint64_t count = 0;
      std::shared_ptr<Node> current = head.load();
      if (head.load()->next[level].load()->type == SENTINEL_TAIL)
        continue;
      std::cerr << "Level " << level << ": ";
      while (current) {
        if (current->type == SENTINEL_HEAD)
          std::cerr << "-inf ";
        else if (current->type == SENTINEL_TAIL) {
          std::cerr << "inf ";
          break;
        } else {
          std::cerr << current->value << " ";
          count++;
          if (count >= n)
            break;
        }
        current = current->next[level].load();
      }
      std::cerr << std::endl;
    }
  }

  // for debugging purposes
  void display(uint64_t n) {
    std::shared_ptr<Node> current = head.load()->next[0].load();
    uint64_t count = 0;
    while (current->type != SENTINEL_TAIL) {
      std::cout << current->value << " ";
      current = current->next[0].load();
      count++;
      if (count >= n)
        break;
    }
    std::cout << std::endl;
  }
};
