/*
High-level orchestration: 
where to insert, 
when to flush, 
when to create or replace segments, 
searching in parallel?
*/
#pragma once

#include "DataTypes.h"
#include "SegmentHolder.h"
#include "AppendableSegment.h"
#include "ImmutableSegment.h"
// #include "SegmentUtils.h"

#include <memory>
#include <vector>
#include <utility>

namespace vectordb {

class SegmentRegistry {
public:
    SegmentRegistry(size_t dim, DistanceMetric metric);

    void insert(const PointIdType& id, const DenseVector& vec);

    void insertBatch(const std::vector<std::pair<PointIdType, DenseVector>>& points);

    std::vector<std::pair<size_t, float>> search(const DenseVector& query, int top_k) const;

    void flushToImmutable();  // Perform conversion and replacement

private:
    SegmentHolder holder_;
    size_t dim_;
    DistanceMetric metric_;
    SegmentIdType next_segment_id_ = 0;

    std::shared_ptr<AppendableSegment> current_appendable_;
    SegmentIdType current_appendable_id_ = -1;
};

}