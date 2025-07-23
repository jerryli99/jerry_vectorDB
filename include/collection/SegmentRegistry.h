// /*
// High-level orchestration: 
// where to insert, 
// when to flush, 
// when to create or replace segments, 
// searching in parallel?
// */
#pragma once

#include "DataTypes.h"
#include "SegmentHolder.h"
#include "AppendableSegment.h"
#include "ImmutableSegment.h"
// // #include "SegmentUtils.h"
#include <memory>
#include <vector>
#include <utility>
#include <atomic>

namespace vectordb {

class SegmentRegistry {
public:
    // Constructor with dimension and metric
    SegmentRegistry(size_t dim, DistanceMetric metric);
    
    // Insert single vector with ID
    void insert(PointIdType id, const DenseVector& vec);
    
    // Batch insert multiple vectors
    void insertBatch(const std::vector<std::pair<PointIdType, DenseVector>>& points);
    
    // Search for similar vectors
    std::vector<std::pair<PointIdType, float>> search(const DenseVector& query, int top_k) const;

private:
    void createNewAppendableSegment();
    void flushToImmutable();
    size_t calculateVectorSize(const DenseVector& vec) const;

    // Configuration
    const size_t dim_;
    const DistanceMetric metric_;
    const size_t max_appendable_size_bytes_ = 20 * 1024 * 1024; // 20MB threshold

    // Current appendable segment state
    std::shared_ptr<AppendableSegment> current_appendable_;
    std::atomic<size_t> current_size_bytes_;

    // All segments
    SegmentHolder holder_;
};

} // namespace vectordb