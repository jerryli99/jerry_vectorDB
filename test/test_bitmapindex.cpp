#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "../src/BitmapIndex.h"

TEST_CASE("BitmapIndex Basic Operations", "[bitmap]") {
    vectordb::BitmapIndex bm;
    
    SECTION("Initial state") {
        REQUIRE(bm.size() == 0);
    }
    
    SECTION("Resize and basic set/get") {
        bm.resize(10);
        REQUIRE(bm.size() == 10);
        
        // Initially all bits should be 0
        for (size_t i = 0; i < 10; i++) {
            REQUIRE(bm.get(i) == false);
        }
        
        // Set some bits
        bm.set(3);
        bm.set(7);
        bm.set(9);
        
        REQUIRE(bm.get(3) == true);
        REQUIRE(bm.get(7) == true);
        REQUIRE(bm.get(9) == true);
        REQUIRE(bm.get(0) == false);
        REQUIRE(bm.get(5) == false);
        
        // Test out of bounds
        REQUIRE_THROWS_AS(bm.get(10), std::out_of_range);
        REQUIRE_THROWS_AS(bm.set(10), std::out_of_range);
    }
}

TEST_CASE("BitmapIndex Clear Operation", "[bitmap]") {
    vectordb::BitmapIndex bm;
    bm.resize(20);
    
    // Set some bits
    bm.set(5);
    bm.set(10);
    bm.set(15);
    
    REQUIRE(bm.get(5) == true);
    REQUIRE(bm.get(10) == true);
    REQUIRE(bm.get(15) == true);
    
    bm.clear();
    
    // After clear, all bits should be 0
    for (size_t i = 0; i < 20; i++) {
        REQUIRE(bm.get(i) == false);
    }
}

TEST_CASE("BitmapIndex Bitwise Operations", "[bitmap]") {
    vectordb::BitmapIndex bm1, bm2;
    bm1.resize(16);
    bm2.resize(16);
    
    SECTION("AND operation (intersection)") {
        bm1.set(1);
        bm1.set(3);
        bm1.set(5);
        bm1.set(7);
        
        bm2.set(3);
        bm2.set(5);
        bm2.set(9);
        
        auto result = bm1 & bm2;
        
        REQUIRE(result.get(1) == false);
        REQUIRE(result.get(3) == true);  // Common
        REQUIRE(result.get(5) == true);  // Common
        REQUIRE(result.get(7) == false);
        REQUIRE(result.get(9) == false);
        
        // Test size mismatch
        vectordb::BitmapIndex bm3;
        bm3.resize(8);
        REQUIRE_THROWS_AS(bm1 & bm3, std::invalid_argument);
    }
    
    SECTION("OR operation (union)") {
        bm1.set(2);
        bm1.set(4);
        bm1.set(6);
        
        bm2.set(4);
        bm2.set(6);
        bm2.set(8);
        
        auto result = bm1 | bm2;
        
        REQUIRE(result.get(2) == true);
        REQUIRE(result.get(4) == true);
        REQUIRE(result.get(6) == true);
        REQUIRE(result.get(8) == true);
        REQUIRE(result.get(0) == false);
        REQUIRE(result.get(10) == false);
    }
    
    SECTION("Complex multi-bit operations") {
        // Test with 64-bit boundaries
        vectordb::BitmapIndex large1, large2;
        large1.resize(150);  // Crosses multiple 64-bit blocks
        large2.resize(150);
        
        large1.set(63);   // Last bit of first block
        large1.set(64);   // First bit of second block
        large1.set(100);
        
        large2.set(63);   // Overlap
        large2.set(64);   // Overlap
        large2.set(120);
        
        auto andResult = large1 & large2;
        auto orResult = large1 | large2;
        
        REQUIRE(andResult.get(63) == true);
        REQUIRE(andResult.get(64) == true);
        REQUIRE(andResult.get(100) == false);
        REQUIRE(andResult.get(120) == false);
        
        REQUIRE(orResult.get(63) == true);
        REQUIRE(orResult.get(64) == true);
        REQUIRE(orResult.get(100) == true);
        REQUIRE(orResult.get(120) == true);
    }
}

TEST_CASE("BitmapIndex to_ids Conversion", "[bitmap]") {
    vectordb::BitmapIndex bm;
    bm.resize(32);
    
    SECTION("Empty bitmap") {
        auto ids = bm.to_ids();
        REQUIRE(ids.empty());
    }
    
    SECTION("With set bits") {
        bm.set(0);
        bm.set(5);
        bm.set(10);
        bm.set(31);  // Last bit
        
        auto ids = bm.to_ids();
        REQUIRE(ids.size() == 4);
        REQUIRE(ids[0] == 0);
        REQUIRE(ids[1] == 5);
        REQUIRE(ids[2] == 10);
        REQUIRE(ids[3] == 31);
    }
    
    SECTION("All bits set") {
        for (size_t i = 0; i < 32; i++) {
            bm.set(i);
        }
        
        auto ids = bm.to_ids();
        REQUIRE(ids.size() == 32);
        for (size_t i = 0; i < 32; i++) {
            REQUIRE(ids[i] == i);
        }
    }
}

TEST_CASE("BitmapIndex Debug String", "[bitmap]") {
    vectordb::BitmapIndex bm;
    bm.resize(10);
    
    bm.set(1);
    bm.set(3);
    bm.set(6);
    bm.set(9);
    
    std::string debug = bm.debugString();
    // Expected: "0 1 0 1 0 0 1 0 0 1 "
    REQUIRE(debug.find("0 1 0 1 0 0 1 0 0 1") != std::string::npos);
    
    // Test with limit
    bm.resize(100);
    bm.set(99);
    std::string limited = bm.debugString(20);
    // Should only show first 20 bits
    REQUIRE(limited.length() < 60);  // Approximate check
}

TEST_CASE("Multi-tenant filtering example", "[bitmap][multitenant]") {
    // Simulate: 10 vectors, some belong to user1, some to user2
    vectordb::BitmapIndex user1_index;
    vectordb::BitmapIndex category_image_index;
    
    user1_index.resize(10);
    category_image_index.resize(10);
    
    // User1's vectors: IDs 1, 3, 6, 9
    user1_index.set(1);
    user1_index.set(3);
    user1_index.set(6);
    user1_index.set(9);
    
    // Vectors with category="image": IDs 1, 2, 6
    category_image_index.set(1);
    category_image_index.set(2);
    category_image_index.set(6);
    
    SECTION("Filter by user only") {
        auto user1_vectors = user1_index.to_ids();
        REQUIRE(user1_vectors == std::vector<size_t>{1, 3, 6, 9});
    }
    
    SECTION("Filter by category only") {
        auto image_vectors = category_image_index.to_ids();
        REQUIRE(image_vectors == std::vector<size_t>{1, 2, 6});
    }
    
    SECTION("Combine filters (user AND category)") {
        auto combined = user1_index & category_image_index;
        auto result_ids = combined.to_ids();
        
        // Only IDs that are both: user1 AND category=image
        REQUIRE(result_ids.size() == 2);
        REQUIRE(result_ids[0] == 1);
        REQUIRE(result_ids[1] == 6);
    }
    
    SECTION("Mark delete example") {
        // Simulate marking ID 3 as deleted
        vectordb::BitmapIndex deleted_index;
        deleted_index.resize(10);
        deleted_index.set(3);
        
        // Filter out deleted items
        auto active_user1 = user1_index & deleted_index;  // Actually, we want NOT deleted
        // In real usage, you'd need a NOT operation or difference
        // For now, let's show the issue:
        auto deleted_for_user1 = user1_index & deleted_index;
        REQUIRE(deleted_for_user1.get(3) == true);  // ID 3 is both user1 and deleted
    }
}

TEST_CASE("Performance characteristics", "[bitmap][performance]") {
    const size_t LARGE_SIZE = 1000000;  // 1 million items
    
    vectordb::BitmapIndex large_bm;
    large_bm.resize(LARGE_SIZE);
    
    BENCHMARK("Set random bits in large bitmap") {
        for (size_t i = 0; i < 10000; i++) {
            large_bm.set(i * 97 % LARGE_SIZE);
        }
    };
    
    BENCHMARK("AND operation on large bitmaps") {
        vectordb::BitmapIndex other;
        other.resize(LARGE_SIZE);
        for (size_t i = 0; i < 5000; i++) {
            other.set(i * 103 % LARGE_SIZE);
        }
        auto result = large_bm & other;
        return result.size();  // Use result to prevent optimization
    };
}

TEST_CASE("Edge Cases", "[bitmap][edge]") {
    SECTION("Size 0 bitmap") {
        vectordb::BitmapIndex bm;
        bm.resize(0);
        REQUIRE(bm.size() == 0);
        REQUIRE_THROWS(bm.get(0));
        REQUIRE(bm.to_ids().empty());
    }
    
    SECTION("Size 1 bitmap") {
        vectordb::BitmapIndex bm;
        bm.resize(1);
        REQUIRE(bm.get(0) == false);
        bm.set(0);
        REQUIRE(bm.get(0) == true);
        REQUIRE(bm.to_ids() == std::vector<size_t>{0});
    }
    
    SECTION("Exact 64-bit boundary") {
        vectordb::BitmapIndex bm;
        bm.resize(64);
        bm.set(0);
        bm.set(63);
        
        REQUIRE(bm.get(0) == true);
        REQUIRE(bm.get(63) == true);
        REQUIRE(bm.get(32) == false);
        
        auto ids = bm.to_ids();
        REQUIRE(ids.size() == 2);
        // REQUIRE((ids[0] == 0 && ids[1] == 63) || (ids[0] == 63 && ids[1] == 0));
        bool is_valid_order = (ids[0] == 0 && ids[1] == 63) || (ids[0] == 63 && ids[1] == 0);
        REQUIRE(is_valid_order);
    }
    
    SECTION("Crossing 64-bit boundary") {
        vectordb::BitmapIndex bm;
        bm.resize(65);  // Needs 2 uint64_t blocks
        bm.set(0);
        bm.set(63);
        bm.set(64);
        
        REQUIRE(bm.get(0) == true);
        REQUIRE(bm.get(63) == true);
        REQUIRE(bm.get(64) == true);
        REQUIRE(bm.get(1) == false);
    }
}