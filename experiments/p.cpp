// #include <iostream>
// #include <vector>
// #include <string>
// #include "DataTypes.h"
// #include "Point.h"
// #include "PointMemoryPool.h"

// using namespace vectordb;

// void testPointMemoryPool() {
//     std::cout << "=== Testing PointMemoryPool with default size (5000) ===\n\n";
    
//     // Create pool with default size (5000 points)
//     PointMemoryPool<8> pool;
//     std::cout << "Created pool with capacity: " << pool.getMaxCapacity() << " points\n";
//     std::cout << "Initial allocated: " << pool.getTotalAllocated() << " points\n";
//     std::cout << "Free slots: " << pool.getFreeSlots() << "\n\n";

//     // Test 1: Allocate points up to capacity
//     std::cout << "=== Test 1: Allocating points up to capacity ===\n";
//     std::vector<Point<8>*> points;
    
//     for (size_t i = 0; i < pool.getMaxCapacity(); ++i) {
//         std::string point_id = "point_" + std::to_string(i);
//         Point<8>* point = pool.allocatePoint(point_id);  // Direct pointer return
        
//         if (point) {
//             point->addVector("embedding", {0.1f, 0.2f, 0.3f, 0.4f});
//             point->addVector("text", {0.5f, 0.6f, 0.7f, 0.8f});
//             points.push_back(point);
//         } else {
//             std::cout << "Error allocating point " << i << " (should not happen here!)\n";
//             break;
//         }
//     }
    
//     std::cout << "Allocated " << points.size() << " points\n";
//     std::cout << "Pool usage: " << pool.getTotalAllocated() << "/" << pool.getMaxCapacity() << "\n";
//     std::cout << "Free slots: " << pool.getFreeSlots() << "\n\n";

//     // Test 2: Try to allocate one more point (should return nullptr)
//     std::cout << "=== Test 2: Trying to allocate beyond capacity ===\n";
//     Point<8>* overflow_point = pool.allocatePoint("overflow_point");
//     if (!overflow_point) {
//         std::cout << "Successfully caught overflow: returned nullptr\n";
//     } else {
//         std::cout << "ERROR: Should have returned nullptr but didn't!\n";
//         pool.deallocatePoint(overflow_point);
//     }
//     std::cout << "\n";

//     // Test 3: Deallocate some points and reuse
//     std::cout << "=== Test 3: Deallocating and reusing points ===\n";
//     size_t points_to_remove = 1000;
//     for (size_t i = 0; i < points_to_remove; ++i) {
//         pool.deallocatePoint(points[i]);
//     }
//     points.erase(points.begin(), points.begin() + points_to_remove);
    
//     std::cout << "Deallocated " << points_to_remove << " points\n";
//     std::cout << "Pool usage after deallocation: " << pool.getTotalAllocated() << "/" << pool.getMaxCapacity() << "\n";
//     std::cout << "Free slots: " << pool.getFreeSlots() << "\n\n";

//     // Test 4: Reallocate the freed points
//     std::cout << "=== Test 4: Reallocating freed points ===\n";
//     for (size_t i = 0; i < points_to_remove; ++i) {
//         std::string point_id = "reused_point_" + std::to_string(i);
//         Point<8>* point = pool.allocatePoint(point_id);
        
//         if (point) {
//             point->addVector("new_embedding", {0.9f, 1.0f, 1.1f, 1.2f});
//             points.push_back(point);
//         } else {
//             std::cout << "Error reallocating point (should not happen!)\n";
//             break;
//         }
//     }
    
//     std::cout << "Total points after reuse: " << points.size() << "\n";
//     std::cout << "Pool usage: " << pool.getTotalAllocated() << "/" << pool.getMaxCapacity() << "\n";
//     std::cout << "Free slots: " << pool.getFreeSlots() << "\n\n";

//     // Test 5: Test getAllPoints()
//     std::cout << "=== Test 5: Testing getAllPoints() ===\n";
//     auto all_points = pool.getAllPoints();
//     std::cout << "getAllPoints() returned: " << all_points.size() << " points\n";
    
//     // Verify some points
//     if (!all_points.empty()) {
//         auto first_point = all_points[0];
//         auto vec = first_point->getVector("embedding");
//         if (vec) {
//             std::cout << "First point has embedding vector with size: " << vec->size() << "\n";
//         }
//     }
//     std::cout << "\n";

//     // Test 6: Test containsPoint()
//     std::cout << "=== Test 6: Testing containsPoint() ===\n";
//     if (!all_points.empty()) {
//         bool contains = pool.containsPoint(all_points[0]);
//         std::cout << "containsPoint(first_point): " << (contains ? "true" : "false") << "\n";
//     }
    
//     // Test with invalid pointer (create a stack-allocated point)
//     Point<8> invalid_point("invalid_id");
//     bool contains_invalid = pool.containsPoint(&invalid_point);
//     std::cout << "containsPoint(invalid_point): " << (contains_invalid ? "true" : "false") << "\n";
//     std::cout << "\n";

//     // Test 7: Clear the pool
//     std::cout << "=== Test 7: Testing clear() ===\n";
//     pool.clear();
//     std::cout << "After clear - Pool usage: " << pool.getTotalAllocated() << "/" << pool.getMaxCapacity() << "\n";
//     std::cout << "Free slots: " << pool.getFreeSlots() << "\n";
    
//     // Test 8: Try to allocate after clear
//     std::cout << "=== Test 8: Allocating after clear ===\n";
//     Point<8>* new_point = pool.allocatePoint("new_after_clear");
//     if (new_point) {
//         std::cout << "Successfully allocated after clear\n";
//         pool.deallocatePoint(new_point);
//     } else {
//         std::cout << "Failed to allocate after clear (should not happen!)\n";
//     }

//     std::cout << "\n=== Test completed successfully! ===\n";
// }

// int main() {
//     try {
//         testPointMemoryPool();
//         return 0;
//     } catch (const std::exception& e) {
//         std::cerr << "Exception caught: " << e.what() << std::endl;
//         return 1;
//     }
// }