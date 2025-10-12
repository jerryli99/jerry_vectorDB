#pragma once
#include "Status.h"
#include "DataTypes.h"
#include <vector>
#include <string>
#include <chrono>

namespace vectordb {

struct ScoredId {
    PointIdType id;
    float score;
};

struct QueryBatchResult {
    std::vector<ScoredId> hits;
};

struct QueryResult {
    std::vector<QueryBatchResult> results;  // multiple queries
    Status status;
    double time_seconds = 0.0;
};

} // namespace vectordb
