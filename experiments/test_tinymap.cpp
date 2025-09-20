#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "TinyMap.h"

using namespace vectordb;

void test_capacity_limits() {
    std::cout << "\nTesting capacity limits:\n";
    
    // Test with small capacity
    {
        TinyMap<std::string, int, 5> map;
        assert(map.insert("one", 1));
        assert(map.insert("two", 2));
        assert(map.insert("three", 3));
        assert(map.insert("four", 4));
        assert(map.insert("five", 5));
        assert(map.size() == 5);
        
        // 6th insert should fail
        assert(!map.insert("six", 6));
        assert(map.size() == 5);
    }

    // Test with medium capacity
    {
        TinyMap<std::string, int, 20> map;
        for (int i = 0; i < 20; i++) {
            std::string key = "key" + std::to_string(i);
            assert(map.insert(key, i));
        }
        assert(map.size() == 20);
        
        // 21st insert should fail
        assert(!map.insert("key20", 20));
        assert(map.size() == 20);
    }

    // Test with large capacity
    {
        TinyMap<std::string, int, 100> map;
        for (int i = 0; i < 100; i++) {
            std::string key = "k" + std::to_string(i);
            assert(map.insert(key, i));
        }
        assert(map.size() == 100);
        
        // 101st insert should fail
        assert(!map.insert("k100", 100));
        assert(map.size() == 100);
    }

    // Test iteration with many elements
    {
        TinyMap<int, std::string, 1000> map;
        for (int i = 0; i < 1000; i++) {
            assert(map.insert(i, "val" + std::to_string(i)));
        }
        
        // Verify all elements exist
        for (int i = 0; i < 1000; i++) {
            assert(map.contains(i));
            assert(map.get(i).value().get() == "val" + std::to_string(i));
        }
        
        std::cout << "Verified 1000 inserts and lookups\n";
    }

    std::cout << "All capacity limit tests passed!\n";
}

void test_tinymap() {
    // Previous test cases would go here...
    test_capacity_limits();
}

int main() {
    test_tinymap();
    return 0;
}