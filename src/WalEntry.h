#pragma once

#include "DataTypes.h"
#include "Point.h"

namespace vectordb {
    struct WalEntry {
        std::atomic<int64_t> id;
        //so before toImmutableSegment(), we write the buffered vectors to WAL.
        std::vector<Point<MAX_ENTRIES_TINYMAP>> vectors;
        uint64_t timestamp;  // std::chrono::system_clock::now().time_since_epoch().count()
        // uint32_t rows;??
        // uint32_t cols;??
        uint32_t checksum;  // Simple XOR checksum (optional)
    };
}