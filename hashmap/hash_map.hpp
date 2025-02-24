#pragma once

#include <functional>
#include <list>
#include <vector>
#include <shared_mutex>
#include <mutex>

const size_t DEFAULT_INITIAL_SIZE = 100;
const float DEFAULT_LOAD_FACTOR = 0.7f;

template <typename K, typename V>
class HashMap
{
private:
  using Bucket = std::list<std::pair<K, V>>;
  std::vector<Bucket> buckets;
  std::vector<std::shared_mutex> bucket_mutexes;
  std::atomic<size_t> num_elements; // atomic to prevent data races, ordering doesn't matter as much
  float max_load_factor = DEFAULT_LOAD_FACTOR;
  std::shared_mutex rehash_mutex;

  size_t hash(const K &key) const
  {
    return std::hash<K>{}(key) % buckets.size();
  }

  void rehash()
  {
    std::unique_lock<std::shared_mutex> lock(rehash_mutex);
    size_t new_size = buckets.size() * 2;
    std::vector<Bucket> new_buckets(new_size);
    std::vector<std::shared_mutex> new_bucket_mutexes(new_size);

    for (auto &bucket : buckets)
    {
      for (auto &pair : bucket)
      {
        size_t new_index = std::hash<K>{}(pair.first) % new_size;
        new_buckets[new_index].push_back(pair);
      }
    }

    buckets = std::move(new_buckets);
    bucket_mutexes = std::move(new_bucket_mutexes);
  }

public:
  HashMap(size_t initial_size = DEFAULT_INITIAL_SIZE)
      : buckets(initial_size), bucket_mutexes(initial_size), num_elements(0) {}

  void insert(const K &key, const V &value)
  {
    std::shared_lock<std::shared_mutex> rehash_lock(rehash_mutex);
    size_t index = hash(key);
    auto &m = bucket_mutexes[index];
    std::unique_lock<std::shared_mutex> lock(m);

    // Check for existing key
    for (auto &pair : buckets[index])
    {
      if (pair.first == key)
      {
        pair.second = value;
        return;
      }
    }

    // Insert new pair
    buckets[index].emplace_back(key, value);
    num_elements++;

    // Check load factor
    if (static_cast<float>(num_elements) / static_cast<float>(buckets.size()) > max_load_factor)
    {
      lock.unlock(); // unlock before rehashing to prevent lock-order-inversion (potential deadlock)
      rehash_lock.unlock();
      rehash();
    }
  }

  bool get(const K &key, V &value)
  {
    std::shared_lock<std::shared_mutex> rehash_lock(rehash_mutex);
    size_t index = hash(key);
    auto &m = bucket_mutexes[index];
    std::shared_lock<std::shared_mutex> lock(m);

    for (const auto &pair : buckets[index])
    {
      if (pair.first == key)
      {
        value = pair.second;
        return true;
      }
    }
    return false;
  }

  bool remove(const K &key)
  {
    std::shared_lock<std::shared_mutex> rehash_lock(rehash_mutex);
    size_t index = hash(key);
    auto &m = bucket_mutexes[index];
    std::unique_lock<std::shared_mutex> lock(m);

    auto &bucket = buckets[index];

    for (auto it = bucket.begin(); it != bucket.end(); ++it)
    {
      if (it->first == key)
      {
        bucket.erase(it);
        num_elements--;
        return true;
      }
    }
    return false;
  }

  size_t size() const { return num_elements; }
  bool empty() const { return num_elements == 0; }
};
