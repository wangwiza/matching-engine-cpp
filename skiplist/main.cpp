#include "skip_list.hpp"
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

// Test object storage
void test_objects() {
  struct Person {
    std::string name;
    int age;
    bool operator<(const Person &other) const { return age < other.age; }
  };

  skip_list<Person> people;
  people.add({"Alice", 35});
  people.add({"Bob", 25});
  people.add({"Charlie", 30});

  assert(people.get_head().name == "Bob");
  assert(people.remove({"Bob", 25}));
  assert(people.get_head().name == "Charlie");
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
    while (value == INT32_MIN) {
      value = getRandomInt32();
    }
    list.add(value);
  }
  std::cout << "Add Thread " << thread_id << " finished" << std::endl;
}

void get_head(skip_list<int32_t> &list, int thread_id) {
  for (int i = 0; i < NUM_OPS; ++i) {
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
  test_objects();

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
  if (!list.contains(INT32_MIN)) {
    list.add(INT32_MIN);
  }

  for (int i = 0; i < NUM_INSERT_THREADS; ++i) {
    threads.push_back(std::thread(addValues, std::ref(list), i));
  }
  for (int i = 0; i < NUM_GET_THREADS; ++i) {
    threads.push_back(std::thread(get_ensure_min, std::ref(list), i));
  }

  for (auto &thread : threads | std::views::reverse) {
    thread.join();
  }

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
