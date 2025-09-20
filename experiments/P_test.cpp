// #include <iostream>
// #include <vector>
// #include <chrono>
// #include <memory>
// #include <thread>
// #include <atomic>
// #include <mutex>
// #include <functional>
// #include <limits>
// #include "DataTypes.h"
// #include "Point.h"
// #include "PointMemoryPool.h"

// using namespace vectordb;

// // Custom vector class to track reallocations
// template<typename T>
// class TrackingVector {
// private:
//     std::vector<T> data;
//     size_t realloc_count = 0;
//     size_t initial_capacity;

// public:
//     TrackingVector(size_t initial_capacity = 0) : initial_capacity(initial_capacity) {
//         if (initial_capacity > 0) {
//             data.reserve(initial_capacity);
//         }
//     }

//     void push_back(const T& value) {
//         size_t old_capacity = data.capacity();
//         data.push_back(value);
//         if (data.capacity() > old_capacity) {
//             realloc_count++;
//         }
//     }

//     void push_back(T&& value) {
//         size_t old_capacity = data.capacity();
//         data.push_back(std::move(value));
//         if (data.capacity() > old_capacity) {
//             realloc_count++;
//         }
//     }

//     size_t getReallocCount() const { return realloc_count; }
//     size_t capacity() const { return data.capacity(); }
//     size_t size() const { return data.size(); }
    
//     // Forward other necessary methods
//     auto begin() { return data.begin(); }
//     auto end() { return data.end(); }
//     T& operator[](size_t index) { return data[index]; }
//     const T& operator[](size_t index) const { return data[index]; }
// };

// // Test without memory pool - with reallocation tracking
// void testWithoutMemoryPool(size_t num_points) {
//     std::cout << "Testing WITHOUT memory pool (" << num_points << " points)...\n";
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     TrackingVector<std::unique_ptr<Point<8>>> points;
    
//     for (size_t i = 0; i < num_points; ++i) {
//         std::string point_id = "point_" + std::to_string(i);
//         auto point = std::make_unique<Point<8>>(point_id);
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("te", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("t", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//         points.push_back(std::move(point));
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     std::cout << "Time taken: " << duration.count() << " microseconds\n";
//     std::cout << "Reallocations: " << points.getReallocCount() << " times\n";
//     std::cout << "Final capacity: " << points.capacity() << " points\n";
//     std::cout << "Memory fragmentation: HIGH\n\n";
// }

// // Test with reserve - to show the difference
// void testWithoutMemoryPoolWithReserve(size_t num_points) {
//     std::cout << "Testing WITHOUT memory pool WITH reserve (" << num_points << " points)...\n";
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     TrackingVector<std::unique_ptr<Point<8>>> points(num_points);
    
//     for (size_t i = 0; i < num_points; ++i) {
//         std::string point_id = "point_" + std::to_string(i);
//         auto point = std::make_unique<Point<8>>(point_id);
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("te", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("t", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//         points.push_back(std::move(point));
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     std::cout << "Time taken: " << duration.count() << " microseconds\n";
//     std::cout << "Reallocations: " << points.getReallocCount() << " times\n";
//     std::cout << "Final capacity: " << points.capacity() << " points\n";
//     std::cout << "Memory fragmentation: MEDIUM\n\n";
// }

// // Test with simplified memory pool
// void testWithSimpleMemoryPool(size_t num_points) {
//     std::cout << "Testing WITH simple memory pool (" << num_points << " points)...\n";
    
//     PointMemoryPool pool(num_points);
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     TrackingVector<Point<8>*> points(num_points);
    
//     for (size_t i = 0; i < num_points; ++i) {
//         std::string point_id = "point_" + std::to_string(i);
//         auto* point = pool.allocatePoint<8>(point_id);
//         if (point) {
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("te", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("t", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//             points.push_back(point);
//         }
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     // Cleanup
//     for (auto* point : points) {
//         pool.deallocatePoint(point);
//     }
    
//     std::cout << "Time taken: " << duration.count() << " microseconds\n";
//     std::cout << "Reallocations: " << points.getReallocCount() << " times\n";
//     std::cout << "Final capacity: " << points.capacity() << " points\n";
//     std::cout << "Pool usage: " << pool.getTotalAllocated() << "/" 
//               << pool.getMaxCapacity() << " points\n\n";
// }

// // Test fast batch allocation
// void testFastBatchAllocation(size_t num_points) {
//     std::cout << "Testing FAST batch allocation (" << num_points << " points)...\n";
    
//     PointMemoryPool pool(num_points);
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     // Use fast batch allocation
//     auto points_vector = pool.allocatePointsBatch<8>(num_points, "batch_");
    
//     // Track reallocations for the outer vector
//     TrackingVector<Point<8>*> points(num_points);
//     for (auto* point : points_vector) {
//         points.push_back(point);
//     }
    
//     // Initialize vectors
//     for (auto* point : points) {
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("te", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("t", {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f});
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     // Cleanup
//     for (auto* point : points) {
//         pool.deallocatePoint(point);
//     }
    
//     std::cout << "Time taken: " << duration.count() << " microseconds\n";
//     std::cout << "Reallocations: " << points.getReallocCount() << " times\n";
//     std::cout << "Final capacity: " << points.capacity() << " points\n";
//     std::cout << "Batch allocated: " << points_vector.size() << " points\n\n";
// }


// void testAccessPerformance(size_t num_points) {
//     std::cout << "Testing ACCESS performance (" << num_points << " points)...\n";
    
//     // Test 1: Without pool
//     std::vector<std::unique_ptr<Point<8>>> points_no_pool;
//     points_no_pool.reserve(num_points);
//     for (size_t i = 0; i < num_points; ++i) {
//         auto point = std::make_unique<Point<8>>("point_" + std::to_string(i));
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         points_no_pool.push_back(std::move(point));
//     }
    
//     // Test 2: With pool
//     PointMemoryPool pool(num_points);
//     std::vector<Point<8>*> points_with_pool;
//     points_with_pool.reserve(num_points);
//     for (size_t i = 0; i < num_points; ++i) {
//         auto point = pool.allocatePoint<8>("point_" + std::to_string(i));
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         points_with_pool.push_back(point);
//     }
    
//     // Measure access time
//     auto start = std::chrono::high_resolution_clock::now();
//     float sum_no_pool = 0;
//     for (auto& point : points_no_pool) {
//         auto vec = point->getVector("text");
//         if (vec) sum_no_pool += vec.value()[0];
//     }
//     auto mid = std::chrono::high_resolution_clock::now();
    
//     float sum_with_pool = 0;
//     for (auto* point : points_with_pool) {
//         auto vec = point->getVector("text");
//         if (vec) sum_with_pool += vec.value()[0];
//     }
//     auto end = std::chrono::high_resolution_clock::now();
    
//     auto duration_no_pool = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
//     auto duration_with_pool = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
//     std::cout << "Access time without pool: " << duration_no_pool.count() << " μs\n";
//     std::cout << "Access time with pool: " << duration_with_pool.count() << " μs\n";
//     std::cout << "Access speedup: " << (duration_no_pool.count() - duration_with_pool.count()) * 100.0 / duration_no_pool.count() << "%\n";
    
//     // Cleanup
//     for (auto* point : points_with_pool) {
//         pool.deallocatePoint(point);
//     }
// }


// //===================

// // Test multi-threaded access with memory pool
// void testMultiThreadedAccessWithPool(size_t num_points, int num_threads) {
//     std::cout << "Testing MULTI-THREADED ACCESS with memory pool (" 
//               << num_points << " points, " << num_threads << " threads)...\n";
    
//     // Setup memory pool with points
//     PointMemoryPool pool(num_points);
//     std::vector<Point<8>*> points;
//     points.reserve(num_points);
    
//     for (size_t i = 0; i < num_points; ++i) {
//         std::string point_id = "point_" + std::to_string(i);
//         auto* point = pool.allocatePoint<8>(point_id);
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         point->addVector("image", {0.5f, 0.6f, 0.7f, 0.8f});
//         points.push_back(point);
//     }
    
//     std::atomic<size_t> total_accessed{0};
//     std::atomic<size_t> read_operations{0};
//     std::mutex cout_mutex;
    
//     auto worker = [&](int thread_id, size_t start, size_t end) {
//         float local_sum = 0;
//         size_t local_ops = 0;
        
//         for (size_t i = start; i < end; ++i) {
//             if (auto vec = points[i]->getVector("text")) {
//                 local_sum += vec.value()[0];
//                 local_ops++;
//             }
//             if (auto vec = points[i]->getVector("image")) {
//                 local_sum += vec.value()[0];
//                 local_ops++;
//             }
//         }
        
//         total_accessed += local_ops;
//         read_operations += local_ops;
        
//         std::lock_guard<std::mutex> lock(cout_mutex);
//         std::cout << "Thread " << thread_id << " processed " << local_ops << " operations\n";
//     };
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     std::vector<std::thread> threads;
//     size_t points_per_thread = num_points / num_threads;
    
//     for (int i = 0; i < num_threads; ++i) {
//         size_t start_idx = i * points_per_thread;
//         size_t end_idx = (i == num_threads - 1) ? num_points : start_idx + points_per_thread;
//         threads.emplace_back(worker, i, start_idx, end_idx);
//     }
    
//     for (auto& t : threads) {
//         t.join();
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     std::cout << "Total time: " << duration.count() << " μs\n";
//     std::cout << "Total read operations: " << read_operations.load() << "\n";
//     std::cout << "Throughput: " << (read_operations.load() * 1000000.0 / duration.count()) 
//               << " ops/sec\n";
//     std::cout << "Time per operation: " << (duration.count() * 1000.0 / read_operations.load()) 
//               << " ns/op\n\n";
    
//     // Cleanup
//     for (auto* point : points) {
//         pool.deallocatePoint(point);
//     }
// }

// // Test multi-threaded read/write mix with memory pool
// void testMultiThreadedReadWriteWithPool(size_t num_points, int num_threads) {
//     std::cout << "Testing MULTI-THREADED READ/WRITE with memory pool (" 
//               << num_points << " points, " << num_threads << " threads)...\n";
    
//     PointMemoryPool pool(num_points * 2); // Extra capacity for writes
//     std::vector<Point<8>*> points;
//     points.reserve(num_points);
    
//     // Initial setup
//     for (size_t i = 0; i < num_points; ++i) {
//         std::string point_id = "point_" + std::to_string(i);
//         auto* point = pool.allocatePoint<8>(point_id);
//         point->addVector("text", {0.1f, 0.2f, 0.3f, 0.4f});
//         points.push_back(point);
//     }
    
//     std::atomic<size_t> read_operations{0};
//     std::atomic<size_t> write_operations{0};
//     std::atomic<size_t> next_point_id{num_points};
//     std::mutex cout_mutex;
    
//     auto worker = [&](int thread_id) {
//         size_t local_reads = 0;
//         size_t local_writes = 0;
        
//         // Mix of reads and writes
//         for (int i = 0; i < 1000; ++i) {
//             if (rand() % 100 < 70) { // 70% reads, 30% writes
//                 // Read operation
//                 size_t index = rand() % points.size();
//                 if (auto vec = points[index]->getVector("text")) {
//                     local_reads++;
//                 }
//             } else {
//                 // Write operation - add new point
//                 size_t new_id = next_point_id++;
//                 std::string point_id = "point_" + std::to_string(new_id);
//                 auto* new_point = pool.allocatePoint<8>(point_id);
//                 if (new_point) {
//                     new_point->addVector("text", {float(rand()) / RAND_MAX, 
//                                                  float(rand()) / RAND_MAX,
//                                                  float(rand()) / RAND_MAX,
//                                                  float(rand()) / RAND_MAX});
//                     std::lock_guard<std::mutex> lock(cout_mutex);
//                     points.push_back(new_point);
//                     local_writes++;
//                 }
//             }
//         }
        
//         read_operations += local_reads;
//         write_operations += local_writes;
        
//         std::lock_guard<std::mutex> lock(cout_mutex);
//         std::cout << "Thread " << thread_id << ": " << local_reads << " reads, " 
//                   << local_writes << " writes\n";
//     };
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     std::vector<std::thread> threads;
//     for (int i = 0; i < num_threads; ++i) {
//         threads.emplace_back(worker, i);
//     }
    
//     for (auto& t : threads) {
//         t.join();
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     std::cout << "Total time: " << duration.count() << " μs\n";
//     std::cout << "Total reads: " << read_operations.load() << ", writes: " 
//               << write_operations.load() << "\n";
//     std::cout << "Throughput: " << ((read_operations + write_operations) * 1000000.0 / duration.count()) 
//               << " ops/sec\n";
//     std::cout << "Final points: " << points.size() << "/" << pool.getMaxCapacity() << "\n\n";
    
//     // Cleanup
//     for (auto* point : points) {
//         pool.deallocatePoint(point);
//     }
// }

// // Test concurrent allocation performance
// void testMultiThreadedAllocation(int num_threads, size_t points_per_thread) {
//     std::cout << "Testing MULTI-THREADED ALLOCATION (" 
//               << num_threads << " threads, " << points_per_thread << " points/thread)...\n";
    
//     PointMemoryPool pool(num_threads * points_per_thread);
//     std::atomic<size_t> total_allocated{0};
//     std::mutex cout_mutex;
    
//     auto allocator_worker = [&](int thread_id) {
//         std::vector<Point<8>*> thread_points;
//         thread_points.reserve(points_per_thread);
        
//         for (size_t i = 0; i < points_per_thread; ++i) {
//             std::string point_id = "thread_" + std::to_string(thread_id) + "_point_" + std::to_string(i);
//             auto* point = pool.allocatePoint<8>(point_id);
//             if (point) {
//                 point->addVector("text", {float(thread_id) / num_threads, 
//                                          float(i) / points_per_thread,
//                                          0.3f, 0.4f});
//                 thread_points.push_back(point);
//             }
//         }
        
//         total_allocated += thread_points.size();
        
//         // Deallocate some points to test reuse
//         for (size_t i = 0; i < thread_points.size() / 2; ++i) {
//             pool.deallocatePoint(thread_points[i]);
//         }
        
//         std::lock_guard<std::mutex> lock(cout_mutex);
//         std::cout << "Thread " << thread_id << " allocated " << thread_points.size() 
//                   << " points, deallocated " << (thread_points.size() / 2) << "\n";
//     };
    
//     auto start = std::chrono::high_resolution_clock::now();
    
//     std::vector<std::thread> threads;
//     for (int i = 0; i < num_threads; ++i) {
//         threads.emplace_back(allocator_worker, i);
//     }
    
//     for (auto& t : threads) {
//         t.join();
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
//     std::cout << "Total time: " << duration.count() << " μs\n";
//     std::cout << "Total allocated: " << total_allocated.load() << " points\n";
//     std::cout << "Final pool usage: " << pool.getTotalAllocated() << "/" 
//               << pool.getMaxCapacity() << " points\n";
//     std::cout << "Allocation rate: " << (total_allocated * 1000000.0 / duration.count()) 
//               << " points/sec\n\n";
// }


// int main() {
//     const size_t SMALL_TEST = 2000;
//     const size_t LARGE_TEST = 70000;
    
//     // std::cout << "=== Memory Pool Test with Reallocation Tracking ===\n\n";
    
//     // std::cout << "=== Small Test (" << SMALL_TEST << " points) ===\n";
//     // testWithoutMemoryPool(SMALL_TEST);
//     // testWithoutMemoryPoolWithReserve(SMALL_TEST);
//     // testWithSimpleMemoryPool(SMALL_TEST);
//     // testFastBatchAllocation(SMALL_TEST);
    
//     // std::cout << "=== Large Test (" << LARGE_TEST << " points) ===\n";
//     // testWithoutMemoryPool(LARGE_TEST);
//     // testWithoutMemoryPoolWithReserve(LARGE_TEST);
//     // testWithSimpleMemoryPool(LARGE_TEST);
//     // testFastBatchAllocation(LARGE_TEST);

//     std::cout << "--- Memory Pool Test with Access ---\n\n";
//     std::cout << "=== Small test ===\n";
//     testAccessPerformance(SMALL_TEST);

//     std::cout << "=== Large test ===\n";
//     testAccessPerformance(LARGE_TEST);

//     std::cout << "=== Multi-threaded Memory Pool Tests ===\n\n";
//     // Test 1: Pure read access
//     testMultiThreadedAccessWithPool(2000, 4);
//     testMultiThreadedAccessWithPool(70000, 4);
    
//     // Test 2: Mixed read/write access
//     testMultiThreadedReadWriteWithPool(2000, 4);
//     testMultiThreadedReadWriteWithPool(70000, 4);
    
//     return 0;
// }