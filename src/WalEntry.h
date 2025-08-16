#pragma once

#include "DataTypes.h"

namespace vectordb {
    struct WalEntry {
        int64_t id;
        //so before toImmutableSegment(), we write the buffered vectors to WAL.
        AppendableStorage vectors;
        uint64_t timestamp;  // std::chrono::system_clock::now().time_since_epoch().count()
        // uint32_t rows;??
        // uint32_t cols;??
        uint32_t checksum;  // Simple XOR checksum (optional)
    };
}