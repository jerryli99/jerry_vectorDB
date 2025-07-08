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

int main() {
    std::cout << "Running IdTracker tests...\n";
    
    test_basic_operations();
    test_iteration();
    test_boundary_conditions();

    std::cout << "Tests completed.\n";
    return 0;
}