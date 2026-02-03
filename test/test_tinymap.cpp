#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "../src/TinyMap.h"
#include <string>
#include <vector>
#include <iostream>

TEST_CASE("TinyMap Basic Operations", "[tinymap]") {
    vectordb::TinyMap<std::string, int, 5> map;
    
    SECTION("Initial state") {
        REQUIRE(map.size() == 0);
        REQUIRE(map.empty() == true);
    }
    
    SECTION("Insert and size") {
        REQUIRE(map.insert("apple", 1) == true);
        REQUIRE(map.size() == 1);
        REQUIRE(map.empty() == false);
        
        REQUIRE(map.insert("banana", 2) == true);
        REQUIRE(map.size() == 2);
        
        // Test capacity limit
        REQUIRE(map.insert("cherry", 3) == true);
        REQUIRE(map.insert("date", 4) == true);
        REQUIRE(map.insert("elderberry", 5) == true);
        REQUIRE(map.size() == 5);
        
        // Should fail - capacity reached
        REQUIRE(map.insert("fig", 6) == false);
        REQUIRE(map.size() == 5); // Still 5
    }
    
    SECTION("Get operations") {
        map.insert("apple", 42);
        map.insert("banana", 99);
        
        auto apple_val = map.get("apple");
        REQUIRE(apple_val.has_value() == true);
        REQUIRE(apple_val.value() == 42);
        
        auto banana_val = map.get("banana");
        REQUIRE(banana_val.has_value() == true);
        REQUIRE(banana_val.value() == 99);
        
        // Non-existent key
        auto cherry_val = map.get("cherry");
        REQUIRE(cherry_val.has_value() == false);
    }
    
    SECTION("Contains check") {
        map.insert("test", 123);
        
        REQUIRE(map.contains("test") == true);
        REQUIRE(map.contains("nonexistent") == false);
    }
    
    SECTION("Update existing key") {
        map.insert("key", 10);
        REQUIRE(map.get("key").value() == 10);
        
        // Update should return true and change value
        REQUIRE(map.insert("key", 20) == true);
        REQUIRE(map.size() == 1); // Size shouldn't change
        REQUIRE(map.get("key").value() == 20);
    }
}

TEST_CASE("TinyMap Erase and Clear", "[tinymap]") {
    vectordb::TinyMap<std::string, std::string, 5> map;
    
    SECTION("Erase existing key") {
        map.insert("a", "alpha");
        map.insert("b", "beta");
        map.insert("c", "gamma");
        
        REQUIRE(map.size() == 3);
        REQUIRE(map.erase("b") == true);
        REQUIRE(map.size() == 2);
        REQUIRE(map.contains("b") == false);
        REQUIRE(map.contains("a") == true);
        REQUIRE(map.contains("c") == true);
        
        // Verify other keys still have correct values
        REQUIRE(map.get("a").value() == "alpha");
        REQUIRE(map.get("c").value() == "gamma");
    }
    
    SECTION("Erase non-existent key") {
        map.insert("x", "ex");
        REQUIRE(map.erase("y") == false);
        REQUIRE(map.size() == 1);
    }
    
    SECTION("Erase from different positions") {
        map.insert("first", "1");
        map.insert("second", "2");
        map.insert("third", "3");
        map.insert("fourth", "4");
        
        // Erase first element
        REQUIRE(map.erase("first") == true);
        REQUIRE(map.size() == 3);
        REQUIRE(map.contains("first") == false);
        
        // Erase last element
        REQUIRE(map.erase("fourth") == true);
        REQUIRE(map.size() == 2);
        
        // Erase middle element
        REQUIRE(map.erase("second") == true);
        REQUIRE(map.size() == 1);
        REQUIRE(map.get("third").value() == "3");
    }
    
    SECTION("Clear operation") {
        map.insert("one", "1");
        map.insert("two", "2");
        map.insert("three", "3");
        
        REQUIRE(map.size() == 3);
        map.clear();
        REQUIRE(map.size() == 0);
        REQUIRE(map.empty() == true);
        REQUIRE(map.contains("one") == false);
        
        // Should be able to insert after clear
        REQUIRE(map.insert("new", "value") == true);
        REQUIRE(map.size() == 1);
    }
}

TEST_CASE("TinyMap Iteration", "[tinymap]") {
    vectordb::TinyMap<int, std::string, 10> map;
    
    SECTION("Empty map iteration") {
        int count = 0;
        for (const auto& [k, v] : map) {
            (void)k; (void)v; // Avoid unused warnings
            count++;
        }
        REQUIRE(count == 0);
    }
    
    SECTION("Basic iteration") {
        map.insert(1, "one");
        map.insert(2, "two");
        map.insert(3, "three");
        
        std::vector<int> keys;
        std::vector<std::string> values;
        
        for (const auto& [key, value] : map) {
            keys.push_back(key);
            values.push_back(value);
        }
        
        REQUIRE(keys.size() == 3);
        REQUIRE(values.size() == 3);
        
        // Check all keys are present (order may vary due to erase implementation)
        REQUIRE(std::find(keys.begin(), keys.end(), 1) != keys.end());
        REQUIRE(std::find(keys.begin(), keys.end(), 2) != keys.end());
        REQUIRE(std::find(keys.begin(), keys.end(), 3) != keys.end());
        
        // Find corresponding values
        for (size_t i = 0; i < keys.size(); i++) {
            if (keys[i] == 1) REQUIRE(values[i] == "one");
            if (keys[i] == 2) REQUIRE(values[i] == "two");
            if (keys[i] == 3) REQUIRE(values[i] == "three");
        }
    }
    
    SECTION("Iteration after erase") {
        map.insert(10, "ten");
        map.insert(20, "twenty");
        map.insert(30, "thirty");
        map.insert(40, "forty");
        
        map.erase(20); // Erase middle
        map.erase(10); // Erase first
        
        std::vector<int> remaining_keys;
        for (const auto& [k, v] : map) {
            remaining_keys.push_back(k);
        }
        
        REQUIRE(remaining_keys.size() == 2);
        REQUIRE((remaining_keys[0] == 40 || remaining_keys[1] == 40)); // 40 might be swapped
        REQUIRE((remaining_keys[0] == 30 || remaining_keys[1] == 30));
    }
}

TEST_CASE("TinyMap with complex value types", "[tinymap]") {
    SECTION("Vector values") {
        vectordb::TinyMap<std::string, std::vector<float>, 3> map;
        
        std::vector<float> vec1 = {1.0f, 2.0f, 3.0f};
        std::vector<float> vec2 = {4.0f, 5.0f};
        
        REQUIRE(map.insert("embeddings", vec1) == true);
        REQUIRE(map.insert("metadata", vec2) == true);
        
        auto emb = map.get("embeddings");
        REQUIRE(emb.has_value() == true);
        REQUIRE(emb.value().size() == 3);
        REQUIRE(emb.value()[0] == 1.0f);
        
        // Modify the original vector - shouldn't affect stored copy
        vec1[0] = 99.0f;
        auto emb_again = map.get("embeddings");
        REQUIRE(emb_again.value()[0] == 1.0f); // Still original value
        
        // Update with new vector
        std::vector<float> vec3 = {7.0f, 8.0f, 9.0f};
        REQUIRE(map.insert("embeddings", vec3) == true);
        REQUIRE(map.get("embeddings").value()[0] == 7.0f);
    }
    
    SECTION("Struct values") {
        struct Point {
            float x, y, z;
            bool operator==(const Point& other) const {
                return x == other.x && y == other.y && z == other.z;
            }
        };
        
        vectordb::TinyMap<int, Point, 4> map;
        
        Point p1{1.0f, 2.0f, 3.0f};
        Point p2{4.0f, 5.0f, 6.0f};
        
        map.insert(100, p1);
        map.insert(200, p2);
        
        auto retrieved = map.get(100);
        REQUIRE(retrieved.has_value() == true);
        // REQUIRE(retrieved.value() == p1);
    }
}

TEST_CASE("TinyMap Edge Cases", "[tinymap][edge]") {
    SECTION("Zero capacity map") {
        vectordb::TinyMap<int, int, 0> map;
        REQUIRE(map.size() == 0);
        REQUIRE(map.empty() == true);
        REQUIRE(map.insert(1, 1) == false);
        REQUIRE(map.get(1).has_value() == false);
        REQUIRE(map.erase(1) == false);
        
        // Should compile and work (no iteration)
        for (const auto& kv : map) {
            (void)kv;
            REQUIRE(false); // Should never reach here
        }
    }
    
    SECTION("Capacity 1 map") {
        vectordb::TinyMap<std::string, int, 1> map;
        
        REQUIRE(map.insert("only", 42) == true);
        REQUIRE(map.size() == 1);
        REQUIRE(map.get("only").value() == 42);
        
        REQUIRE(map.insert("another", 99) == false); // Should fail
        REQUIRE(map.size() == 1);
        
        REQUIRE(map.erase("only") == true);
        REQUIRE(map.empty() == true);
        REQUIRE(map.insert("new", 100) == true);
    }
    
    SECTION("Many insert/erase operations") {
        vectordb::TinyMap<int, int, 10> map;
        
        // Fill the map
        for (int i = 0; i < 10; i++) {
            REQUIRE(map.insert(i, i * 10) == true);
        }
        REQUIRE(map.size() == 10);
        
        // Erase some
        for (int i = 0; i < 5; i += 2) {
            REQUIRE(map.erase(i) == true);
        }
        REQUIRE(map.size() == 7);
        
        // Insert new ones
        REQUIRE(map.insert(100, 1000) == true);
        REQUIRE(map.size() == 8);
        
        // Verify remaining values
        REQUIRE(map.get(1).value() == 10);
        REQUIRE(map.get(3).value() == 30);
        REQUIRE(map.get(100).value() == 1000);
    }
}

TEST_CASE("TinyMap Named Vectors Use Case", "[tinymap][usecase]") {
    // Simulating the named vectors use case from comments
    struct NamedVectors {
        using Vector = std::vector<float>;
        vectordb::TinyMap<std::string, Vector, 3> vectors;
        
        void add_vector(const std::string& name, const Vector& vec) {
            vectors.insert(name, vec);
        }
        
        std::optional<Vector> get_vector(const std::string& name) const {
            return vectors.get(name);
        }
    };
    
    NamedVectors point;
    
    SECTION("Point with multiple vector types") {
        std::vector<float> image_vec = {0.1f, 0.2f, 0.3f, 0.4f};
        std::vector<float> text_vec = {0.5f, 0.6f};
        std::vector<float> meta_vec = {0.7f, 0.8f, 0.9f};
        
        point.add_vector("image", image_vec);
        point.add_vector("text", text_vec);
        point.add_vector("meta", meta_vec);
        
        REQUIRE(point.vectors.size() == 3);
        
        auto retrieved_image = point.get_vector("image");
        REQUIRE(retrieved_image.has_value() == true);
        REQUIRE(retrieved_image.value().size() == 4);
        REQUIRE(retrieved_image.value()[0] == 0.1f);
        
        auto retrieved_text = point.get_vector("text");
        REQUIRE(retrieved_text.has_value() == true);
        REQUIRE(retrieved_text.value().size() == 2);
        
        // Update image vector
        std::vector<float> new_image_vec = {1.0f, 1.1f, 1.2f, 1.3f};
        point.add_vector("image", new_image_vec);
        
        auto updated_image = point.get_vector("image");
        REQUIRE(updated_image.value()[0] == 1.0f);
        
        // Try to add 4th vector (should fail with capacity 3)
        std::vector<float> extra_vec = {2.0f};
        point.add_vector("extra", extra_vec);
        REQUIRE(point.vectors.size() == 3); // Still 3
        REQUIRE(point.get_vector("extra").has_value() == false);
    }
}

TEST_CASE("TinyMap Move Semantics", "[tinymap]") {
    // Note: Your current implementation doesn't support move semantics
    // This test shows what could be tested if you add move support
    
    SECTION("Insert with move would be nice") {
        vectordb::TinyMap<std::string, std::vector<int>, 3> map;
        std::vector<int> large_vec(1000, 42); // Large vector
        
        // Current: copies the vector
        REQUIRE(map.insert("large", large_vec) == true);
        
        // If you had move support:
        // REQUIRE(map.insert("moved", std::move(large_vec)) == true);
        // REQUIRE(large_vec.empty()); // Vector was moved
    }
}

TEST_CASE("TinyMap Const Correctness", "[tinymap]") {
    vectordb::TinyMap<int, std::string, 5> map;
    map.insert(1, "one");
    map.insert(2, "two");
    
    SECTION("Const methods") {
        const auto& const_map = map;
        
        // Should compile - all these are const methods
        REQUIRE(const_map.size() == 2);
        REQUIRE(const_map.empty() == false);
        REQUIRE(const_map.contains(1) == true);
        REQUIRE(const_map.get(1).has_value() == true);
        
        // Iteration on const map
        int count = 0;
        for (const auto& [k, v] : const_map) {
            (void)k; (void)v;
            count++;
        }
        REQUIRE(count == 2);
    }
}

TEST_CASE("TinyMap Performance", "[tinymap][performance]") {
    constexpr size_t CAPACITY = 100;
    vectordb::TinyMap<int, int, CAPACITY> map;
    
    BENCHMARK("Insert 100 elements") {
        map.clear();
        for (int i = 0; i < CAPACITY; i++) {
            map.insert(i, i * 2);
        }
        return map.size();
    };
    
    BENCHMARK("Lookup existing key") {
        return map.get(50).has_value();
    };
    
    BENCHMARK("Lookup non-existent key") {
        return map.get(999).has_value();
    };
    
    BENCHMARK("Erase and reinsert") {
        map.erase(50);
        map.insert(50, 100);
        return map.size();
    };
}