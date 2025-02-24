#include <iostream>
#include <thread>
#include <vector>
#include "hash_map.hpp"

const int NUM_THREADS = 10;
const int NUM_OPERATIONS = 10000;

HashMap<int, int> map;

void insert_operations(int thread_id)
{
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        int key = thread_id * NUM_OPERATIONS + i;
        map.insert(key, key * 2);
    }
}

void get_operations(int thread_id)
{
    int value;
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        int key = thread_id * NUM_OPERATIONS + i;
        if (map.get(key, value) && value != key * 2)
        {
            std::cerr << "Data corruption detected for key: " << key << std::endl;
        }
    }
}

void remove_operations(int thread_id)
{
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        int key = thread_id * NUM_OPERATIONS + i;
        map.remove(key);
    }
}

int main()
{
    std::vector<std::thread> threads;

    // Insertions
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(insert_operations, i);
    }
    for (auto &t : threads)
        t.join();
    threads.clear();

    std::cout << "Size after insertions: " << map.size() << std::endl;

    // Retrievals
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(get_operations, i);
    }
    for (auto &t : threads)
        t.join();
    threads.clear();

    // Removals
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(remove_operations, i);
    }
    for (auto &t : threads)
        t.join();
    threads.clear();

    std::cout << "Size after removals: " << map.size() << std::endl;

    return 0;
}
