#include <iostream>
#include <set>
#include "../include/segment/IdTracker.h"

using namespace vectordb;

void print_test_result(bool passed, const std::string& test_name) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << test_name << std::endl;
}

void test_basic_operations() {
    vectordb::IdTracker tracker;
    bool all_passed = true;

    // Test empty state
    all_passed &= tracker.empty();
    all_passed &= (tracker.size() == 0);

    // Test insertion
    auto id1 = tracker.insert(100);
    all_passed &= (id1 == 0);
    all_passed &= !tracker.empty();
    all_passed &= (tracker.size() == 1);

    // Test lookup
    all_passed &= (tracker.get_internal_id(100) == 0);
    all_passed &= (tracker.get_external_id(0) == 100);

    // Test duplicate insertion
    auto id2 = tracker.insert(100);
    all_passed &= (id1 == id2);
    std::cout << tracker.size();

    // Test second insertion
    auto id3 = tracker.insert(200);
    all_passed &= (id3 == 1);
    all_passed &= (tracker.size() == 2);

    // Test removal
    tracker.remove(100);
    all_passed &= (tracker.size() == 1);
    all_passed &= !tracker.get_internal_id(100).has_value();
    all_passed &= !tracker.get_external_id(0).has_value();

    // Test slot reuse
    auto id4 = tracker.insert(300);
    all_passed &= (id4 == 0);  // Should reuse the freed slot

    print_test_result(all_passed, "Basic Operations");
}

void test_iteration() {
    vectordb::IdTracker tracker;
    tracker.insert(100);
    tracker.insert(200);
    tracker.insert(300);
    bool all_passed = true;

    auto internal_ids = tracker.iter_internal_ids();
    auto external_ids = tracker.iter_external_ids();

    all_passed &= (internal_ids.size() == 3);
    all_passed &= (external_ids.size() == 3);

    // Check all external IDs are present
    std::set<vectordb::PointIdType> expected{100, 200, 300};
    std::set<vectordb::PointIdType> actual(external_ids.begin(), external_ids.end());
    all_passed &= (actual == expected);

    // Check internal/external id mapping consistency
    for (auto internal_id : internal_ids) {
        auto external_id = tracker.get_external_id(internal_id);
        all_passed &= external_id.has_value();
        all_passed &= (tracker.get_internal_id(*external_id) == internal_id);
    }

    print_test_result(all_passed, "Iteration");
}

void test_boundary_conditions() {
    vectordb::IdTracker tracker;
    bool all_passed = true;

    // Test invalid lookups
    all_passed &= !tracker.get_internal_id(999).has_value();
    all_passed &= !tracker.get_external_id(999).has_value();

    // Test remove non-existent
    tracker.remove(999);  // Should not crash
    all_passed &= true;

    // Test large sequence
    for (vectordb::PointIdType i = 0; i < 1000; i++) {
        all_passed &= (tracker.insert(i) == i);
    }
    all_passed &= (tracker.size() == 1000);

    // Remove every other item
    for (vectordb::PointIdType i = 0; i < 1000; i += 2) {
        tracker.remove(i);
    }
    all_passed &= (tracker.size() == 500);

    // Check slot reuse
    for (vectordb::PointIdType i = 0; i < 500; i++) {
        auto new_id = 1000 + i;
        auto internal_id = tracker.insert(new_id);
        all_passed &= (internal_id < 1000);  // Should reuse freed slots
    }
    all_passed &= (tracker.size() == 1000);

    print_test_result(all_passed, "Boundary Conditions");
}



void test_sequential_internal_ids() {
    vectordb::IdTracker tracker;
    bool all_passed = true;

    // Insert unordered external IDs
    auto id1 = tracker.insert(500);
    auto id2 = tracker.insert(200);
    auto id3 = tracker.insert(800);
    auto id4 = tracker.insert(100);

    // Verify internal IDs are sequential (0, 1, 2, 3)
    all_passed &= (id1 == 0);
    all_passed &= (id2 == 1);
    all_passed &= (id3 == 2);
    all_passed &= (id4 == 3);
    all_passed &= (tracker.size() == 4);

    // Remove some IDs
    tracker.remove(200);
    tracker.remove(500);

    // Verify internal IDs remain consistent
    all_passed &= (tracker.get_internal_id(800) == 2);
    all_passed &= (tracker.get_internal_id(100) == 3);
    all_passed &= (tracker.size() == 2);

    // Insert new IDs and verify slot reuse
    auto id5 = tracker.insert(300);
    auto id6 = tracker.insert(400);

    // Should reuse the smallest available slots (0 and 1)
    all_passed &= (id5 == 0);
    all_passed &= (id6 == 1);
    all_passed &= (tracker.size() == 4);

    // Verify all internal IDs are sequential and dense (0, 1, 2, 3)
    auto internal_ids = tracker.iter_internal_ids();
    std::set<vectordb::PointOffSetType> expected_internal{0, 1, 2, 3};
    std::set<vectordb::PointOffSetType> actual_internal(internal_ids.begin(), internal_ids.end());
    all_passed &= (actual_internal == expected_internal);

    print_test_result(all_passed, "Sequential Internal IDs");
}

void test_dense_internal_ids_after_removal() {
    vectordb::IdTracker tracker;
    bool all_passed = true;

    // Fill with 10 items
    for (vectordb::PointIdType i = 0; i < 10; i++) {
        tracker.insert(1000 + i);
    }

    // Remove every other item
    for (vectordb::PointIdType i = 0; i < 10; i += 2) {
        tracker.remove(1000 + i);
    }

    // Add new items - should fill the gaps
    for (vectordb::PointIdType i = 0; i < 5; i++) {
        auto internal_id = tracker.insert(2000 + i);
        all_passed &= (internal_id < 10);  // Should reuse freed slots
    }

    // Verify we have dense internal IDs (0-9)
    auto internal_ids = tracker.iter_internal_ids();
    all_passed &= (internal_ids.size() == 10);
    for (size_t i = 0; i < 10; i++) {
        all_passed &= (std::find(internal_ids.begin(), internal_ids.end(), i) != internal_ids.end());
    }

    print_test_result(all_passed, "Dense Internal IDs After Removal");
}

void test_large_scale_operations() {
    vectordb::IdTracker tracker;
    bool all_passed = true;
    const size_t N = 10000;

    // Insert N items
    for (vectordb::PointIdType i = 0; i < N; i++) {
        auto internal_id = tracker.insert(i * 2 + 1);  // Odd numbers
        all_passed &= (internal_id == i);
    }

    // Remove half of them
    for (vectordb::PointIdType i = 0; i < N; i += 2) {
        tracker.remove(i * 2 + 1);
    }

    // Insert new items - should reuse slots
    for (vectordb::PointIdType i = 0; i < N/2; i++) {
        auto internal_id = tracker.insert(i * 2 + 2);  // Even numbers
        all_passed &= (internal_id < N);
    }

    // Verify we have exactly N items with dense internal IDs
    all_passed &= (tracker.size() == N);
    auto internal_ids = tracker.iter_internal_ids();
    all_passed &= (internal_ids.size() == N);
    for (size_t i = 0; i < N; i++) {
        all_passed &= (std::find(internal_ids.begin(), internal_ids.end(), i) != internal_ids.end());
    }

    print_test_result(all_passed, "Large Scale Operations");
}

void test_external_id_unordered() {
    vectordb::IdTracker tracker;
    bool all_passed = true;

    // Insert completely random external IDs
    std::vector<vectordb::PointIdType> external_ids = {
        4298, 12, 999999, 0, 42, 987654, 3, 777777
    };

    for (auto id : external_ids) {
        tracker.insert(id);
    }

    // Verify internal IDs are sequential regardless of external ID order
    auto internal_ids = tracker.iter_internal_ids();
    std::set<vectordb::PointOffSetType> expected_internal;
    for (size_t i = 0; i < external_ids.size(); i++) {
        expected_internal.insert(i);
    }
    std::set<vectordb::PointOffSetType> actual_internal(internal_ids.begin(), internal_ids.end());
    all_passed &= (actual_internal == expected_internal);

    // Verify we can retrieve all external IDs
    std::set<vectordb::PointIdType> expected_external(external_ids.begin(), external_ids.end());
    auto retrieved_external = tracker.iter_external_ids();
    std::set<vectordb::PointIdType> actual_external(retrieved_external.begin(), retrieved_external.end());
    all_passed &= (actual_external == expected_external);

    print_test_result(all_passed, "Unordered External IDs");
}

int main() {
    std::cout << "Running IdTracker tests...\n";
    
    test_basic_operations();
    test_iteration();
    test_boundary_conditions();
    test_sequential_internal_ids();
    test_dense_internal_ids_after_removal();
    test_large_scale_operations();
    test_external_id_unordered();

    std::cout << "Tests completed.\n";
    return 0;
}

// int main() {
//     std::cout << "Running IdTracker tests...\n";
    
//     test_basic_operations();
//     test_iteration();
//     test_boundary_conditions();

//     std::cout << "Tests completed.\n";
//     return 0;
// }