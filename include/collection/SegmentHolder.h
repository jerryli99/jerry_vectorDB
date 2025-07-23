#pragma once

#include "DataTypes.h"
#include "Segment.h"
#include "SegmentEntry.h"

/**
 * @brief 
 *      Low-level container: stores and manages access to segments (like a thread-safe map). 
 *      Doesn’t make decisions.
 *      
 *      Scope: Manages segments inside a single collection.
 *      Purpose: Acts as a manager or container of segments belonging to one collection.
 *      
 *      Responsibilities:
 *      Add, remove, access segments by segment ID.
 *      Coordinate segment lifecycle within that collection.
 *      Provide thread-safe access to the collection’s segments.
 *      Analogy: Like a book’s chapter list holding multiple chapters (segments).
 * 
 */
namespace vectordb {

class SegmentHolder {
public:
    void addSegment(SegmentIdType id, std::shared_ptr<Segment> segment) {
        SegmentType type = segment->isAppendable()
                           ? SegmentType::Appendable
                           : SegmentType::Immutable;
        segments_[id] = SegmentEntry{segment, type};
    }

    std::shared_ptr<Segment> getSegment(SegmentIdType id) const {
        auto it = segments_.find(id);
        return it != segments_.end() ? it->second.segment : nullptr;
    }

    //??? UHm maybe not this guy...
    std::shared_ptr<Segment> getFirstAppendableSegment() const {
        for (const auto& [id, entry] : segments_) {
            if (entry.type == SegmentType::Appendable)
                return entry.segment;
        }
        return nullptr;
    }

    std::unordered_map<SegmentIdType, SegmentEntry> getAllSegments() const {
        return segments_;
    }

private:
    std::unordered_map<SegmentIdType, SegmentEntry> segments_;
};

} // namespace vectordb
