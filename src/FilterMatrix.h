/*
use bitmapindex.h as reference for query filtering
we just have a list of bitmapindex, which looks like a matrix

i might treat this BitmapIndex like a matrix (but i use map keyword) for each segment.
...or you call it bit matrix.

pointid  0   1   2   3   4   5   6   ...
filter1 [1 , 0 , 0 , 0 , 0 , 0 , 0 , ...]
filter2 [1 , 0 , 0 , 0 , 0 , 0 , 1 , ...]
filter3 [1 , 0 , 0 , 0 , 1 , 0 , 0 , ...]
*/

#pragma once

#include "DataTypes.h"
#include "BitmapIndex.h"



namespace vectordb {

class FilterMatrix {

public:
    FilterMatrix() = default;
    ~FilterMatrix() = default;

    void add_filter(const std::string& name, const BitmapIndex& bitmap) noexcept {
        filters[name] = bitmap;
    }
    
    BitmapIndex query(const std::vector<std::string>& filter_names) noexcept {
        BitmapIndex result;
        result.resize(total_points);

        // Initialize with all ones
        for (size_t i = 0; i < total_points; i++) {
            result.set(i, true);
        }
        
        for (const auto& name : filter_names) {
            result = result & filters.at(name);
        }
        return result;
    }
    
private:
    std::unordered_map<std::string, BitmapIndex> filters;
    size_t total_points;
};

}// namespace vectordb