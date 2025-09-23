#pragma once

#include "DataTypes.h"
#include "ActiveSegment.h"
#include "ImmutableSegment.h"

/**
 * @brief 
 * Low-level container: stores and manages access to segments
 * Doesn’t make decisions.
 *      
 * Scope: Manages segments inside a single collection.
 * Purpose: Acts as a manager or container of segments belonging to one collection.
 * 
 * It will hold 1 ActiveSegment and multiple ImmutableSgment(s) per Collection obj.     
 * 
 */
namespace vectordb {

class SegmentHolder {
public:
    SegmentHolder(size_t max_active_capacity, const IndexSpec& index_spec)
        : m_active_segment{max_active_capacity, index_spec} {/*constructor body*/}
    
    ~SegmentHolder() = default;

    Status insertPoint(PointIdType point_id, const DenseVector& vector) {
        auto status = m_active_segment.insertPoint(point_id, vector);
        if (status.ok) {
            // Try to convert if needed
            auto convert_status = convertActiveToImmutable();
            if (!convert_status.ok) {
                return convert_status;
            }
        }
        return status;
    }
    
    Status insertPoint(PointIdType point_id, 
                      const std::map<VectorName, DenseVector>& named_vectors) {
        auto status = m_active_segment.insertPoint(point_id, named_vectors);
        if (status.ok) {
            auto convert_status = convertActiveToImmutable();
            if (!convert_status.ok) {
                return convert_status;
            }
        }
        return status;
    }

    // Convert active to immutable when ready
    Status convertActiveToImmutable() {
        if (!m_active_segment.shouldIndex() && !m_active_segment.isFull()) {
            std::cout << "Hello From Segmentholder, converActiveToImmutable\n";
            return Status::OK();
        }
        
        auto immutable_segment = m_active_segment.convertToImmutable();
        if (!immutable_segment.ok()) {
            return immutable_segment.status();
        }
        
        //the value() which is from StatusOr, could later add ValueOrDie() method in.
        m_immutable_segments.push_back(std::move(immutable_segment.value()));
        
        return Status::OK();
    }

    const ActiveSegment& getActiveSegment() const { 
        return m_active_segment; 
    }

    const std::vector<std::unique_ptr<ImmutableSegment>>& getImmutableSegments() const {
        return m_immutable_segments;
    }

    size_t getTotalPointCount() const {
        size_t count = m_active_segment.getPointCount();
        for (const auto& seg : m_immutable_segments) {
            count += seg->getPointCount();
        }
        return count;
    }

private:
    ActiveSegment m_active_segment;
    //might implement my own AI driven std::vector for capacity prediction expansion later. Cool stuff
    std::vector<std::unique_ptr<ImmutableSegment>> m_immutable_segments;
    // std::atomic<uint64_t> next_id_{0};//mayeb uuid is better for segment id?
};

} // namespace vectordb