#include <atomic>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <ostream>
#include <random>
#include <stdexcept>
#include <vector>

template <typename T, typename Comp = std::less<T>> class skip_list {
private:
  enum node_type { SENTINEL_HEAD, NORMAL, SENTINEL_TAIL };

  struct Node {
    T value;
    std::vector<Node *> next;
    std::vector<Node *> prev;
    node_type type;

    explicit Node(T val, node_type type)
        : value(val), next(), prev(), type(type) {}
  };

  uint32_t max_level;
  Node *head = new Node(T(), SENTINEL_HEAD);
  Node *tail = new Node(T(), SENTINEL_TAIL);
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
  bool is_less_than(const T &value, Node *node) const {
    // tail is equivalent to +inf
    if (node->type == SENTINEL_TAIL)
      return true;
    // head is equivalent to -inf
    if (node->type == SENTINEL_HEAD)
      return false;
    return comp(value, node->value);
  }

  bool is_equal(const T &value, Node *node) const {
    if (node->type == SENTINEL_HEAD || node->type == SENTINEL_TAIL)
      return false;
    return !comp(value, node->value) && !comp(node->value, value);
  }

  Node *get_node(const T &value) const {
    Node *current = head;
    for (int64_t level = max_level_index(); level >= 0; level--) {
      // move right while not strictly less than value on the right
      while (current->next[level]) {
        if (is_equal(value, current->next[level])) {
          /*std::cerr << "current->typenext[level]: " << current->type*/
          /*          << std::endl;*/
          return current->next[level];
        }
        if (is_less_than(value, current->next[level])) {
          break;
        }
        current = current->next[level];
      }
      // decrement level if current node is strictly less than value
    }
    return nullptr;
  }

  uint32_t max_level_index() const { return max_level - 1; }

public:
  skip_list(uint32_t max_lvl = 64, Comp cmp = Comp())
      : max_level(max_lvl), comp(cmp) {
    head->next.resize(max_level, tail);
    tail->prev.resize(max_level, head);
  }

  ~skip_list() {
    Node *current = head->next[0];
    while (current && current->type != SENTINEL_TAIL) {
      Node *next_node = current->next[0];
      delete current;
      current = next_node;
    }
    delete head;
    delete tail;
  }

  // get the smallest element in the skip list
  T &get_head() const {
    if (size.load() == 0) {
      throw std::out_of_range("Skip list is empty");
    }
    return head->next[0]->value;
  }

  // get the largest element in the skip list
  T &get_tail() const {
    if (size.load() == 0) {
      throw std::out_of_range("Skip list is empty");
    }
    return tail->prev[0]->value;
  }

  void add(const T &value) {
    Node *current = head;
    std::vector<Node *> nodes_visited(max_level, nullptr);
    for (int64_t level = max_level_index(); level >= 0; level--) {
      // move right while not strictly less than value on the right
      while (current->next[level]) {
        // since we are adding a new node, we should never reach a node
        assert(!is_equal(value, current->next[level]));
        if (is_less_than(value, current->next[level]))
          break;
        current = current->next[level];
      }
      nodes_visited[level] = current;
      // decrement level if current node is strictly less than value
    }

    uint32_t new_level = random_level();
    Node *new_node = new Node(value, NORMAL);
    new_node->next.resize(max_level, nullptr);
    new_node->prev.resize(max_level, nullptr);

    for (uint32_t i = 0; i <= new_level; i++) {
      new_node->next[i] = nodes_visited[i]->next[i];
      nodes_visited[i]->next[i] = new_node;
      new_node->prev[i] = nodes_visited[i];
      new_node->next[i]->prev[i] = new_node;
    }
    size.fetch_add(1);
  }

  bool contains(const T &value) const {
    Node *target = get_node(value);
    return target != nullptr;
  }

  T &get(const T &value) const {
    Node *target = get_node(value);
    if (!target) {
      throw std::out_of_range("Value not found in skip list");
    }
    return target->value;
  }

  bool remove(const T &value) {
    Node *target = get_node(value);
    if (!target)
      return false;

    for (uint32_t level = 0; level < target->next.size(); level++) {
      // once we see a nullptr, we can break, since the levels are contiguous
      if (target->next[level] == nullptr)
        break;
      target->prev[level]->next[level] = target->next[level];
      target->next[level]->prev[level] = target->prev[level];
    }
    delete target;
    size.fetch_sub(1);
    return true;
  }

  // for debugging purposes
  void display_internals() {
    for (int64_t level = max_level_index(); level >= 0; level--) {
      Node *current = head;
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
