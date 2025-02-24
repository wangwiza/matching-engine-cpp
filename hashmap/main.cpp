#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <chrono>
#include "hash_map.hpp"

const int NUM_INSERT_THREADS = 4;
const int NUM_GET_THREADS = 4;
const int NUM_INSERTS = 100000;

void insertValues(HashMap<int, int>& map, int thread_id) {
    for (int i = thread_id * NUM_INSERTS; i < (thread_id + 1) * NUM_INSERTS; ++i) {
        map.insert(i, i * 10);
    }
}

void getValues(HashMap<int, int>& map, int thread_id) {
    for (int i = thread_id * NUM_INSERTS; i < (thread_id + 1) * NUM_INSERTS; ++i) {
        if (map.contains(i)) {
            assert(map.get(i) == i * 10);
        }
    }
}

int main() {
    HashMap<int, int> map;
    std::vector<std::thread> threads;

    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch threads to insert values
    for (int i = 0; i < NUM_INSERT_THREADS; ++i) {
        threads.emplace_back(insertValues, std::ref(map), i);
    }
    
    // Launch threads to get values concurrently
    for (int i = 0; i < NUM_GET_THREADS; ++i) {
        threads.emplace_back(getValues, std::ref(map), i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    
    std::cout << "Concurrent insert and retrieval time: " << duration.count() << " seconds\n";
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
