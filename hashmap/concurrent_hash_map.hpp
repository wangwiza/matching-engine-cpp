#include <functional>
#include <list>
#include <vector>

const size_t DEFAULT_INITIAL_SIZE = 100;
const float DEFAULT_LOAD_FACTOR = 0.7f;

template <typename K, typename V> class HashMap {
private:
  using Bucket = std::list<std::pair<K, V>>;
  std::vector<Bucket> buckets;
  size_t numElements;
  float maxLoadFactor = DEFAULT_LOAD_FACTOR;

  size_t hash(const K &key) const {
    return std::hash<K>{}(key) % buckets.size();
  }

  void rehash() {
    size_t newSize = buckets.size() * 2;
    std::vector<Bucket> newBuckets(newSize);

    for (auto &bucket : buckets) {
      for (auto &pair : bucket) {
        size_t newIndex = std::hash<K>{}(pair.first) % newSize;
        newBuckets[newIndex].push_back(pair);
      }
    }

    buckets = std::move(newBuckets);
  }

public:
  HashMap(size_t initialSize = DEFAULT_INITIAL_SIZE)
      : buckets(initialSize), numElements(0) {}

  void insert(const K &key, const V &value) {
    size_t index = hash(key);

    // Check for existing key
    for (auto &pair : buckets[index]) {
      if (pair.first == key) {
        pair.second = value;
        return;
      }
    }

    // Insert new pair
    buckets[index].emplace_back(key, value);
    numElements++;

    // Check load factor
    if (static_cast<float>(numElements) / buckets.size() > maxLoadFactor) {
      rehash();
    }
  }

  bool get(const K &key, V &value) const {
    size_t index = hash(key);
    for (const auto &pair : buckets[index]) {
      if (pair.first == key) {
        value = pair.second;
        return true;
      }
    }
    return false;
  }

  bool remove(const K &key) {
    size_t index = hash(key);
    auto &bucket = buckets[index];

    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
      if (it->first == key) {
        bucket.erase(it);
        numElements--;
        return true;
      }
    }
    return false;
  }

  size_t size() const { return numElements; }
  bool empty() const { return numElements == 0; }
};
