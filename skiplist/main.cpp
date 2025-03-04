#include "../order_book.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>

// Test basic functionality
void test_basic_operations() {
  skip_list<int> list;

  // Test empty list
  assert(!list.contains(0));
  try {
    list.get_head();
    assert(false);
  } catch (const std::out_of_range &) {
  }

  // Add elements
  list.add(3);
  list.add(1);
  list.add(4);
  list.add(2);

  // Verify contains and ordering
  assert(list.contains(1));
  assert(list.contains(2));
  assert(list.contains(3));
  assert(list.contains(4));

  assert(list.get_head() == 1);

  // Test get method
  assert(list.get(3) == 3);
  try {
    list.get(5);
    assert(false);
  } catch (const std::out_of_range &) {
  }

  // Remove elements
  assert(list.remove(2));
  assert(!list.remove(2)); // Duplicate remove
  assert(list.remove(1));
  assert(list.get_head() == 3);
}

// Test edge cases
void test_edge_cases() {
  skip_list<int> list;

  // Single element list
  list.add(5);
  assert(list.remove(5));
  assert(!list.contains(5));

  // Add after removal
  list.add(10);
  assert(list.get_head() == 10);
}

// Test different data types
void test_data_types() {
  // Test with strings
  skip_list<std::string> str_list;
  str_list.add("zebra");
  str_list.add("apple");
  str_list.add("monkey");
  assert(str_list.get_head() == "apple");
  assert(str_list.contains("monkey"));

  // Test with custom comparator
  skip_list<int, std::greater<int>> desc_list;
  desc_list.add(3);
  desc_list.add(1);
  desc_list.add(4);
  assert(desc_list.get_head() == 4);
}

void test_min_order_sl() {
  std::vector<std::shared_ptr<order>> orders;
  orders.push_back(std::make_shared<order>(1, "AAPL", 1, 10, SELL, 1));
  orders.push_back(std::make_shared<order>(2, "AAPL", 3, 10, SELL, 1));
  orders.push_back(std::make_shared<order>(3, "AAPL", 5, 10, SELL, 1));
  orders.push_back(std::make_shared<order>(4, "AAPL", 7, 10, SELL, 1));
  orders.push_back(std::make_shared<order>(5, "AAPL", 9, 10, SELL, 1));

  min_sl min_sl;
  for (auto &order : orders) {
    min_sl.add(order);
  }

  for (auto &order : orders) {
    assert(min_sl.contains(order));
  }

  assert(!min_sl.contains(std::make_shared<order>(6, "AAPL", 8, 10, SELL, 1)));

  assert(min_sl.get_head()->price == 1);
  assert(min_sl.remove(orders[0]));
  assert(min_sl.get_head()->price == 3);
  assert(min_sl.remove(orders[1]));
  assert(min_sl.get_head()->price == 5);
  assert(min_sl.remove(orders[2]));
  assert(min_sl.get_head()->price == 7);
  assert(min_sl.remove(orders[3]));
  assert(min_sl.get_head()->price == 9);
}

void test_max_order_sl() {
  std::vector<std::shared_ptr<order>> orders;
  orders.push_back(std::make_shared<order>(1, "AAPL", 1, 10, BUY, 1));
  orders.push_back(std::make_shared<order>(2, "AAPL", 3, 10, BUY, 1));
  orders.push_back(std::make_shared<order>(3, "AAPL", 5, 10, BUY, 1));
  orders.push_back(std::make_shared<order>(4, "AAPL", 7, 10, BUY, 1));
  orders.push_back(std::make_shared<order>(5, "AAPL", 9, 10, BUY, 1));

  max_sl max_sl;
  for (auto &order : orders) {
    max_sl.add(order);
  }

  for (auto &order : orders) {
    assert(max_sl.contains(order));
  }

  assert(!max_sl.contains(std::make_shared<order>(6, "AAPL", 8, 10, BUY, 1)));

  assert(max_sl.get_head()->price == 9);
  assert(max_sl.remove(orders[4]));
  assert(max_sl.get_head()->price == 7);
  assert(max_sl.remove(orders[3]));
  assert(max_sl.get_head()->price == 5);
  assert(max_sl.remove(orders[2]));
  assert(max_sl.get_head()->price == 3);
  assert(max_sl.remove(orders[1]));
  assert(max_sl.get_head()->price == 1);
  assert(max_sl.remove(orders[0]));
  assert(max_sl.empty());
}

void test_order_sl_timestamp() {
  std::vector<std::shared_ptr<order>> orders;
  orders.push_back(std::make_shared<order>(1, "AAPL", 5, 10, SELL, 10));
  orders.push_back(std::make_shared<order>(2, "AAPL", 5, 10, SELL, 20));
  orders.push_back(std::make_shared<order>(3, "AAPL", 5, 10, SELL, 30));
  orders.push_back(std::make_shared<order>(4, "AAPL", 5, 10, SELL, 40));
  orders.push_back(std::make_shared<order>(5, "AAPL", 5, 10, SELL, 50));

  min_sl min_sl;
  max_sl max_sl;
  for (auto &order : orders | std::views::reverse) {
    min_sl.add(order);
    max_sl.add(order);
  }

  for (auto &order : orders) {
    assert(min_sl.contains(order));
    assert(max_sl.contains(order));
  }

  assert(!min_sl.contains(std::make_shared<order>(6, "AAPL", 5, 10, SELL, 25)));
  assert(!max_sl.contains(std::make_shared<order>(6, "AAPL", 5, 10, SELL, 25)));

  for (size_t i = 0; i < orders.size(); i++) {
    assert(min_sl.get_head()->timestamp == orders[i]->timestamp);
    assert(max_sl.get_head()->timestamp == orders[i]->timestamp);
    assert(min_sl.remove(orders[i]));
    assert(max_sl.remove(orders[i]));
  }
}

// Test concurrent removals
void test_concurrent_removal() {
  const int NUM_REMOVE_THREADS = 4;
  const int VALUES_PER_THREAD = 250;

  // First, create a skip list and populate it with known values
  skip_list<int32_t> list;
  std::vector<std::vector<int32_t>> thread_values(NUM_REMOVE_THREADS);

  // Generate unique values for each thread to remove
  for (int t = 0; t < NUM_REMOVE_THREADS; ++t) {
    for (int i = 0; i < VALUES_PER_THREAD; ++i) {
      int32_t value = t * VALUES_PER_THREAD + i;
      thread_values[t].push_back(value);
      list.add(value);
    }
  }

  // Function to remove values
  auto remove_values = [](skip_list<int32_t>& lst, const std::vector<int32_t>& values, int thread_id) {
    uint32_t removed_count = 0;
    for (const auto& value : values) {
      if (lst.remove(value)) {
        removed_count++;
      }
    }
    std::cout << "Remove Thread " << thread_id << " removed " << removed_count << " elements" << std::endl;
  };

  // Start concurrent removal threads
  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_REMOVE_THREADS; ++t) {
    threads.push_back(std::thread(remove_values, std::ref(list), std::ref(thread_values[t]), t));
  }

  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }

  // Verify list is empty
  assert(list.empty());
  std::cout << "Concurrent removal test passed!" << std::endl;
}

// Test concurrent insertions and retrievals

const int NUM_INSERT_THREADS = 4;
const int NUM_GET_THREADS = 4;
const int NUM_OPS = 1000;

int32_t getRandomInt32() {
  // Thread-local storage - each thread gets its own instance
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());
  thread_local std::uniform_int_distribution<int32_t> dist(
      std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());

  // Generate and return the random number
  return dist(gen);
}

void addValues(skip_list<int32_t> &list, int thread_id) {
  for (int i = 0; i < NUM_OPS; ++i) {
    int32_t value = getRandomInt32();
    while (value == INT32_MIN || list.contains(value)) {
      value = getRandomInt32();
    }
    list.add(value);
  }
  std::cout << "Add Thread " << thread_id << " finished" << std::endl;
}

void get_head(skip_list<int32_t> &list, int thread_id) {
  for (int i = 0; i < NUM_OPS; ++i) {
    while (list.empty()) {
      // wait for adder threads to add some values
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    int value = list.get_head();
    assert(value != INT32_MIN); // mainly to ensure value doesn't get optimized
                                // out by compiler
  }
  std::cout << "Get Thread " << thread_id << " finished" << std::endl;
}

void get_ensure_min(skip_list<int32_t> &list, int thread_id) {
  for (int i = 0; i < NUM_OPS; ++i) {
    int value = list.get_head();
    assert(value == INT32_MIN);
  }
  std::cout << "Ensure Get Thread " << thread_id << " finished" << std::endl;
}

int main() {
  test_basic_operations();
  test_edge_cases();
  test_data_types();
  test_min_order_sl();
  test_max_order_sl();
  test_order_sl_timestamp();
  test_concurrent_removal();

  std::vector<std::thread> threads;
  skip_list<int32_t> list;

  for (int i = 0; i < NUM_INSERT_THREADS; ++i) {
    threads.push_back(std::thread(addValues, std::ref(list), i));
  }

  for (int i = 0; i < NUM_GET_THREADS; ++i) {
    threads.push_back(std::thread(get_head, std::ref(list), i));
  }

  for (auto &thread : threads | std::views::reverse) {
    thread.join();
  }

  threads.clear();
  list.add(INT32_MIN);

  for (int i = 0; i < NUM_INSERT_THREADS; ++i) {
    threads.push_back(std::thread(addValues, std::ref(list), i));
  }
  for (int i = 0; i < NUM_GET_THREADS; ++i) {
    threads.push_back(std::thread(get_ensure_min, std::ref(list), i));
  }

  for (auto &thread : threads | std::views::reverse) {
    thread.join();
  }

  uint32_t expected_size = (2 * NUM_INSERT_THREADS * NUM_OPS) + 1;
  // ensure ordering is maintained
  int32_t prev = INT32_MIN;
  uint32_t count = 0;
  while (!list.empty()) {
    int32_t curr = list.get_head();
    if (prev > curr) {
      list.display(5);
    }
    assert(prev <= curr);
    bool removed = list.remove(curr);
    assert(removed);
    prev = curr;
    count++;
  }
  std::cout << "Expected size: " << expected_size << ", Actual size: " << count
            << std::endl;
  assert(count == expected_size);

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
