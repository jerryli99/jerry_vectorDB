/*
use bitmapindex.h as reference for query filtering
we just have a list of bitmapindex, which looks like a matrix

i might treat this BitmapIndex like a matrix (but i use map keyword) for each segment.
...or you call it bit matrix.

pointid  0   1   2   3   4   5   6   ...
filter1 [1 , 0 , 0 , 0 , 0 , 0 , 0 , ...]
filter2 [1 , 0 , 0 , 0 , 0 , 0 , 1 , ...]
filter3 [1 , 0 , 0 , 0 , 1 , 0 , 0 , ...]

so something like this (read it as row by row by row...):

Point IDs:   0   1   2   3   4   5   6   7   8   9
tenant_123  [1 , 0 , 1 , 0 , 1 , 0 , 1 , 0 , 0 , 1]
category_img[0 , 1 , 1 , 0 , 0 , 1 , 0 , 0 , 1 , 0]
price_high  [1 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 1]

*/

#pragma once

#include "DataTypes.h"
#include "BitmapIndex.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

namespace vectordb {

class FilterMatrix {
public:
    FilterMatrix() : total_points{0} {/*constructor body*/}
    ~FilterMatrix() = default;

    void resize(size_t new_size) {
        total_points = new_size;
        // Resize all existing bitmaps
        for (auto& [name, bitmap] : filters) {
            bitmap.resize(new_size);
        }
    }

    void add_filter(const std::string& name, const BitmapIndex& bitmap) {
        if (bitmap.size() != total_points && total_points > 0) {
            throw std::invalid_argument("Bitmap size must match total_points");
        }
        filters[name] = bitmap;
        if (total_points == 0) {
            total_points = bitmap.size();
        }
    }
    
    BitmapIndex query(const std::vector<std::string>& filter_names) const {
        if (filter_names.empty()) {
            // Return all ones bitmap if no filters
            BitmapIndex all_ones;
            all_ones.resize(total_points);
            for (size_t i = 0; i < total_points; i++) {
                all_ones.set(i, true);
            }
            return all_ones;
        }
        
        BitmapIndex result;
        bool first = true;
        
        for (const auto& name : filter_names) {
            auto it = filters.find(name);
            if (it == filters.end()) {
                // Filter not found - return empty result
                BitmapIndex empty;
                empty.resize(total_points);
                return empty;
            }
            
            if (first) {
                result = it->second;
                first = false;
            } else {
                result = result & it->second;
            }
        }
        
        return result;
    }

    // Get list of available filter names
    std::vector<std::string> get_filter_names() const {
        std::vector<std::string> names;
        names.reserve(filters.size());
        for (const auto& [name, _] : filters) {
            names.push_back(name);
        }
        return names;
    }

    // Check if a filter exists
    bool has_filter(const std::string& name) const {
        return filters.find(name) != filters.end();
    }

    size_t get_total_points() const { return total_points; }
    size_t get_filter_count() const { return filters.size(); }

private:
    std::unordered_map<std::string, BitmapIndex> filters;
    size_t total_points;
};

} // namespace vectordb

/*

// Setup
FilterMatrix matrix;
matrix.resize(10);  // We have 10 points

// Create some filters
BitmapIndex tenant_A, category_image, price_high;

// tenant_A: points 0, 2, 4, 6, 9 belong to tenant_A
tenant_A.resize(10);
tenant_A.set(0, true); tenant_A.set(2, true); tenant_A.set(4, true);
tenant_A.set(6, true); tenant_A.set(9, true);

// category_image: points 1, 2, 5, 8 are images
category_image.resize(10);
category_image.set(1, true); category_image.set(2, true);
category_image.set(5, true); category_image.set(8, true);

// price_high: points 0, 3, 4, 7, 9 have high price
price_high.resize(10);
price_high.set(0, true); price_high.set(3, true); price_high.set(4, true);
price_high.set(7, true); price_high.set(9, true);

// Add filters to matrix
matrix.add_filter("tenant_A", tenant_A);
matrix.add_filter("category_image", category_image);
matrix.add_filter("price_high", price_high);

//Query
auto result1 = matrix.query({"tenant_A", "category_image"});
// tenant_A:      [1, 0, 1, 0, 1, 0, 1, 0, 0, 1]
// category_image:[0, 1, 1, 0, 0, 1, 0, 0, 1, 0]
// BITWISE AND:   [0, 0, 1, 0, 0, 0, 0, 0, 0, 0] → Only point 2 matches!
auto point_ids = result1.to_ids();  // Returns [2]

auto result2 = matrix.query({"tenant_A", "price_high"});
// tenant_A: [1, 0, 1, 0, 1, 0, 1, 0, 0, 1]
// price_high:[1, 0, 0, 1, 1, 0, 0, 1, 0, 1]
// AND result:[1, 0, 0, 0, 1, 0, 0, 0, 0, 1] → Points 0, 4, 9
auto point_ids = result2.to_ids();  // Returns [0, 4, 9]

auto result3 = matrix.query({});
// Returns all ones: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
// All 10 points included
*/