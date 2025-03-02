#include "hash_map.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <limits>
#include <string>

// Test basic operations: insert, update, get, and remove.
void test_basic_operations() {
  HashMap<int, std::string> map;
  // Initially empty
  assert(map.empty());

  // Insert key-value pairs
  map.insert(1, "one");
  map.insert(2, "two");
  map.insert(3, "three");

  // Check existence
  assert(map.contains(1));
  assert(map.contains(2));
  assert(map.contains(3));

  // Test get
  assert(map.get(2) == "two");

  // Update the value for key 2
  map.insert(2, "deux");
  assert(map.get(2) == "deux");

  // Remove a key
  assert(map.remove(2));
  assert(!map.contains(2));

  // Attempt to remove a non-existent key
  assert(!map.remove(2));
}

// Test edge cases such as retrieval from an empty map or non-existent keys.
void test_edge_cases() {
  HashMap<int, int> map;
  map.insert(42, 100);
  assert(map.get(42) == 100);

  // Remove the only element and verify map emptiness.
  assert(map.remove(42));
  assert(map.empty());

  // get() should throw if key does not exist.
  try {
    map.get(42);
    assert(false); // Should not reach here
  } catch (const std::out_of_range &) {
    // Expected behavior.
  }
}

// Test with different data types (using strings as keys and values).
void test_data_types() {
  HashMap<std::string, std::string> map;
  map.insert("apple", "red");
  map.insert("banana", "yellow");
  map.insert("grape", "purple");

  assert(map.contains("apple"));
  assert(map.get("banana") == "yellow");

  // Update a value and verify the update.
  map.insert("apple", "green");
  assert(map.get("apple") == "green");
}

// Test object storage by using a custom structure as the value.
void test_objects() {
  struct Person {
    std::string name;
    int age;
  };

  HashMap<int, Person> people;
  people.insert(1, {"Alice", 30});
  people.insert(2, {"Bob", 25});
  people.insert(3, {"Charlie", 35});

  // Check retrieval and update of object values.
  assert(people.get(1).name == "Alice");
  people.insert(1, {"Alice", 31});
  assert(people.get(1).age == 31);
  assert(people.remove(2));
  assert(!people.contains(2));
}

// --- Concurrent Test Functions ---

const int NUM_INSERT_THREADS = 4;
const int NUM_CONTAINS_THREADS = 4;
const int NUM_OPS = 1000;

// Returns a random integer using thread-local generators.
int getRandomInt() {
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());
  thread_local std::uniform_int_distribution<int> dist(
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
  return dist(gen);
}

// Function for threads that concurrently insert random key-value pairs.
void insertValues(HashMap<int, int> &map, int thread_id) {
  for (int i = 0; i < NUM_OPS; ++i) {
    int key = getRandomInt();
    map.insert(key, key);
  }
  std::cout << "Insert thread " << thread_id << " finished" << std::endl;
}

// Function for threads that concurrently check for key existence.
void checkContains(HashMap<int, int> &map, int thread_id) {
  for (int i = 0; i < NUM_OPS; ++i) {
    int key = getRandomInt();
    // We're not asserting here since many random keys won't be present.
    map.contains(key);
  }
  std::cout << "Contains thread " << thread_id << " finished" << std::endl;
}

int main() {
  // Run basic, edge, data type, and object tests.
  test_basic_operations();
  test_edge_cases();
  test_data_types();
  test_objects();

  // Concurrent test: perform concurrent insertions and lookups.
  HashMap<int, int> concurrent_map;
  std::vector<std::thread> threads;

  // Spawn threads that insert values.
  for (int i = 0; i < NUM_INSERT_THREADS; ++i) {
    threads.emplace_back(insertValues, std::ref(concurrent_map), i);
  }
  // Spawn threads that check for key existence.
  for (int i = 0; i < NUM_CONTAINS_THREADS; ++i) {
    threads.emplace_back(checkContains, std::ref(concurrent_map), i);
  }

  // Wait for all threads to finish.
  for (auto &thread : threads) {
    thread.join();
  }

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
