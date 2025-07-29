/*
High-level orchestration: 
Meta data and control tower
where to insert, 
when to flush, 
when to create or replace segments, 
searching in parallel?
*/
#pragma once

#include "DataTypes.h"
#include "SegmentHolder.h"
// // #include "SegmentUtils.h"
#include <memory>
#include <vector>
#include <utility>
#include <atomic>

namespace vectordb 
{

    class SegmentRegister {
    public:
        void register_segment(const SegmentInfo& info);
        std::optional<SegmentInfo> get_segment_info(const std::string& id);
        const std::vector<SegmentInfo> list_all();
        void mark_deleted(const std::string& id);
        void update_status(const std::string& id, SegmentStatus new_status);

    private:
        std::unordered_map<std::string, SegmentInfo> registry;
        std::mutex mutex;
    };

}


// namespace vectordb {

// class SegmentRegistry {
// public:
//     // Constructor with dimension and metric
//     SegmentRegistry(size_t dim, DistanceMetric metric);
    
//     // Insert single vector with ID
//     void insert(PointIdType id, const DenseVector& vec);
    
//     // Batch insert multiple vectors
//     void insertBatch(const std::vector<std::pair<PointIdType, DenseVector>>& points);
    
//     // Search for similar vectors
//     std::vector<std::pair<PointIdType, float>> search(const DenseVector& query, int top_k) const;

// private:
//     void createNewAppendableSegment();
//     void flushToImmutable();
//     size_t calculateVectorSize(const DenseVector& vec) const;

//     // Configuration
//     const size_t dim_;
//     const DistanceMetric metric_;
//     const size_t max_appendable_size_bytes_ = 20 * 1024 * 1024; // 20MB threshold

//     // Current appendable segment state
//     std::shared_ptr<AppendableSegment> current_appendable_;
//     std::atomic<size_t> current_size_bytes_;

//     // All segments
//     SegmentHolder holder_;
// };

// } // namespace vectordb