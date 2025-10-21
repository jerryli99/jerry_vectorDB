#include "../src/SmartArray2.h"
#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>

#include <random>
using namespace vectordb;

int main() {
    std::cout << "Adaptive SmartArray test (Linux) - weights persistence enabled\n";

    // parameters
    size_t initial_capacity = 200;
    size_t min_chunk = 100;
    size_t max_chunk = 5000;
    SmartArray<int> arr(initial_capacity, min_chunk, max_chunk, true, ".smartarray_weights", "smartarray_log.csv");

    // random workload: bursts + steady
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> value_dist(1, 100);
    std::uniform_int_distribution<int> burst_len(10, 200);
    std::bernoulli_distribution burst_prob(0.02); // occasional bursts

    const int TOTAL_INSERTS = 20000;

    for (int i = 0; i < TOTAL_INSERTS; ++i) {
        // occasionally produce a burst of rapid inserts
        if (burst_prob(rng)) {
            int b = burst_len(rng);
            for (int j = 0; j < b; ++j) {
                arr.push_back(value_dist(rng));
            }
        } else {
            arr.push_back(value_dist(rng));
            // simulate some inter-arrival delay for more realistic fill times
            if ((i & 0x3F) == 0) std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // print progress occasionally
        if ((i+1) % 50000 == 0) {
            std::cout << "Inserted " << (i+1) << " elements. Chunks: " << arr.chunk_count() << "\n";
        }
    }

    arr.getMemoryStats();

    // explicitly save learned weights (also saved automatically on destructor)
    if (arr.save_weights()) {
        std::cout << "Predictor weights saved to .smartarray_weights\n";
    } else {
        std::cout << "Failed to save predictor weights\n";
    }

    std::cout << "Log file: smartarray_log.csv\n";
    return 0;
}


// int main() {
//     SmartArray arr(50, 2000);

//     std::mt19937 rng(42);
//     std::uniform_int_distribution<int> dist(1, 50);

//     // Simulate incremental inserts
//     for (int i = 0; i < 10000; ++i) {
//         arr.push_back(dist(rng));
//     }

//     arr.printStats();
// }

// using namespace vectordb;

// void test_basic_push_and_access() {
//     std::cout << "\n[Test] Basic push_back & access...\n";
//     SmartArray<int> arr(GrowthModel::S_CURVE, 10, 1000, 0.002);

//     for (int i = 0; i < 500; ++i)
//         arr.push_back(i);

//     assert(arr.size() == 500);
//     for (int i = 0; i < 500; ++i)
//         assert(arr[i] == i);

//     std::cout << "  ✓ push_back and indexing OK\n";
// }

// void test_emplace_and_move() {
//     std::cout << "\n[Test] emplace_back and move semantics...\n";
//     struct Obj {
//         int x;
//         Obj(int v): x(v) {}
//     };

//     SmartArray<Obj> arr(GrowthModel::LINEAR);
//     for (int i = 0; i < 50; ++i)
//         arr.emplace_back(i * 2);

//     assert(arr.size() == 50);
//     for (int i = 0; i < 50; ++i)
//         assert(arr[i].x == i * 2);

//     // move it
//     SmartArray<Obj> moved = std::move(arr);
//     assert(moved.size() == 50);
//     std::cout << "  ✓ emplace_back and move OK\n";
// }

// void test_bounds_and_exceptions() {
//     std::cout << "\n[Test] Bounds checking...\n";
//     SmartArray<int> arr(GrowthModel::EXPONENTIAL);
//     for (int i = 0; i < 20; ++i) arr.push_back(i);

//     bool caught = false;
//     try {
//         auto x = arr.at(100);
//         (void)x;
//     } catch (const std::out_of_range&) {
//         caught = true;
//     }
//     assert(caught);
//     std::cout << "  ✓ at() throws on OOB\n";
// }

// void test_growth_models() {
//     std::cout << "\n[Test] Growth model predictions...\n";
//     SmartArray<int> s(GrowthModel::S_CURVE);
//     SmartArray<int> e(GrowthModel::EXPONENTIAL);
//     SmartArray<int> l(GrowthModel::LINEAR);
//     SmartArray<int> lg(GrowthModel::LOGARITHMIC);

//     // prefill a few elements to trigger multiple chunk allocations
//     for (int i = 0; i < 1000; ++i) s.push_back(i);
//     for (int i = 0; i < 1000; ++i) e.push_back(i);
//     for (int i = 0; i < 1000; ++i) l.push_back(i);
//     for (int i = 0; i < 1000; ++i) lg.push_back(i);

//     for (int i = 0; i < 1000; ++i) s.push_back(i);
//     for (int i = 0; i < 1000; ++i) e.push_back(i);
//     for (int i = 0; i < 1000; ++i) l.push_back(i);
//     for (int i = 0; i < 1000; ++i) lg.push_back(i);

//     std::cout << "\n--- S_CURVE ---\n";
//     s.getMemoryStats();

//     std::cout << "\n--- EXPONENTIAL ---\n";
//     e.getMemoryStats();

//     std::cout << "\n--- LINEAR ---\n";
//     l.getMemoryStats();

//     std::cout << "\n--- LOGARITHMIC ---\n";
//     lg.getMemoryStats();

//     std::cout << "  ✓ Growth model stats printed\n";
// }

// void test_reserve_behavior() {
//     std::cout << "\n[Test] Reserve() behavior...\n";
//     SmartArray<int> arr(GrowthModel::LOGARITHMIC);
//     arr.reserve(2000);
//     assert(arr.chunk_count() > 0);
//     std::cout << "  ✓ reserve() preallocates OK\n";
// }

// void test_clear_and_reuse() {
//     std::cout << "\n[Test] clear() and reuse...\n";
//     SmartArray<int> arr(GrowthModel::LINEAR);
//     for (int i = 0; i < 100; ++i) arr.push_back(i);
//     arr.clear();
//     assert(arr.size() == 0);
//     arr.push_back(123);
//     assert(arr[0] == 123);
//     std::cout << "  ✓ clear() resets OK\n";
// }

// void test_chunk_access() {
//     std::cout << "\n[Test] get_chunk() access...\n";
//     SmartArray<int> arr(GrowthModel::S_CURVE);
//     for (int i = 0; i < 500; ++i) arr.push_back(i);

//     for (size_t i = 0; i < arr.chunk_count(); ++i) {
//         const auto& ch = arr.get_chunk(i);
//         for (auto v : ch) {
//             (void)v;
//         }
//     }
//     std::cout << "  ✓ get_chunk() works\n";
// }

// int main() {
//     std::cout << "=== SmartArray Tests ===\n";

//     test_basic_push_and_access();
//     test_emplace_and_move();
//     test_bounds_and_exceptions();
//     test_growth_models();
//     test_reserve_behavior();
//     test_clear_and_reuse();
//     test_chunk_access();

//     std::cout << "\nAll SmartArray tests passed successfully ✅\n";
//     return 0;
// }
