// #include <iostream>
// #include <vector>
// #include <thread>
// #include <mutex>
// #include <random>
// #include <cassert>
// #include <algorithm>
// #include <atomic>
// #include <chrono>
// #include "MinHeap.h"  // Include your MinHeap with std::greater<T> as default

// void singleThreadTest() {
//     MinHeap<int> heap;
//     heap.push(5);
//     heap.push(1);
//     heap.push(3);
//     heap.push(2);
//     heap.push(4);

//     std::cout << "Single-thread heap contents: ";
//     heap.print();

//     assert(heap.size() == 5);
//     assert(heap.top() == 1);

//     std::vector<int> result;
//     while (!heap.empty()) {
//         result.push_back(heap.pop());
//     }

//     assert((result == std::vector<int>{1, 2, 3, 4, 5}));
//     std::cout << "✅ Single-thread test passed.\n";
// }


// void multiThreadInsertTest(MinHeap<int>& heap, int start, int end, int threadId) {
//     for (int i = start; i <= end; ++i) {
//         heap.push(i);
//         std::cout << "[Producer " << threadId << "] pushed " << i << "\n";
//     }
// }

// void multiThreadExtractTest(MinHeap<int>& heap, std::vector<int>& out, std::mutex& out_mutex, std::atomic<int>& doneFlag, int threadId) {
//     while (true) {
//         if (heap.empty()) {
//             if (doneFlag.load()) break;
//             std::this_thread::sleep_for(std::chrono::milliseconds(1));
//             continue;
//         }

//         try {
//             int val = heap.pop();
//             {
//                 std::lock_guard<std::mutex> lock(out_mutex);
//                 out.push_back(val);
//                 std::cout << "  [Consumer " << threadId << "] popped " << val << "\n";
//             }
//         } catch (...) {
//             // Heap might be empty between check and pop
//         }
//     }
// }

// void concurrentProducerConsumerTest() {
//     MinHeap<int> heap;

//     std::vector<std::thread> producers;
//     std::vector<std::thread> consumers;

//     std::vector<int> results;
//     std::mutex results_mutex;
//     std::atomic<int> doneProducing = 0;

//     // Start 4 producer threads
//     for (int i = 0; i < 4; ++i) {
//         int start = i * 250 + 1;
//         int end = start + 249;
//         producers.emplace_back(multiThreadInsertTest, std::ref(heap), start, end, i);
//     }
//     heap.print();
//     // Start 3 consumer threads
//     for (int i = 0; i < 3; ++i) {
//         consumers.emplace_back(multiThreadExtractTest, std::ref(heap), std::ref(results), std::ref(results_mutex), std::ref(doneProducing), i);
//     }

//     for (auto& p : producers) p.join();
//     doneProducing = 1;
//     for (auto& c : consumers) c.join();

//     std::cout << "Concurrent test finished. Extracted count: " << results.size() << "\n";

//     // Sort-check
//     std::vector<int> sorted = results;
//     std::sort(sorted.begin(), sorted.end());

//     // Validate 1~1000
//     for (int i = 0; i < 1000; ++i) {
//         assert(sorted[i] == i + 1);
//     }
//     heap.print();
//     std::cout << "Extracted elements are in sorted order.\n";
// }


// int main() {
//     singleThreadTest();
//     concurrentProducerConsumerTest();
//     return 0;
// }
