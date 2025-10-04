#pragma once

#include "DataTypes.h"
#include "Point.h"

namespace vectordb {
enum class WalEntryType : uint8_t {
    Insert = 0,
    Delete = 1,
    Update = 2,
};

struct WalEntry {
    WalEntryType type;
    std::string collection_name;
    PointIdType point_id;
    std::map<VectorName, DenseVector> vectors;
};


}