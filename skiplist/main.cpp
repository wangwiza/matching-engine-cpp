#include "skip_list.hpp"
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <string>
#include <functional>

// Test basic functionality
void test_basic_operations() {
  skip_list<int> list;

  // Test empty list
  assert(!list.contains(0));
  try {
    list.get_head();
    assert(false);
  } catch(const std::out_of_range&) {}

  try {
    list.get_tail();
    assert(false);
  } catch(const std::out_of_range&) {}

  // Add elements
  list.add(3);
  list.add(1);
  /*list.display_internals();*/
  list.add(4);
  list.add(2);

  // Verify contains and ordering
  assert(list.contains(1));
  assert(list.contains(2));
  assert(list.contains(3));
  assert(list.contains(4));

  assert(list.get_head() == 1);
  assert(list.get_tail() == 4);

  // Test get method
  assert(list.get(3) == 3);
  try {
    list.get(5);
    assert(false);
  } catch(const std::out_of_range&) {}

  // Remove elements
  assert(list.remove(2));
  assert(!list.remove(2));  // Duplicate remove
  assert(list.remove(1));
  assert(list.get_head() == 3);
  assert(list.get_tail() == 4);
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
  assert(list.get_tail() == 10);
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
  assert(desc_list.get_tail() == 1);
}

// Test object storage
void test_objects() {
  struct Person {
    std::string name;
    int age;
    bool operator<(const Person& other) const {
      return age < other.age;
    }
  };

  skip_list<Person> people;
  people.add({"Alice", 35});
  people.add({"Bob", 25});
  people.add({"Charlie", 30});

  assert(people.get_head().name == "Bob");
  assert(people.get_tail().name == "Alice");
  assert(people.remove({"Bob", 25}));
  assert(people.get_head().name == "Charlie");
  assert(people.get_tail().name == "Alice");
}

int main() {
  test_basic_operations();
  test_edge_cases();
  test_data_types();
  test_objects();

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
