#pragma once

#include "DataTypes.h"

namespace vectordb {
    struct WalEntry {
        int64_t id;
        //so before toImmutableSegment(), we write the buffered vectors to WAL.
        AppendableStorage vectors;
        // std::vector<float> vector;
    };
}