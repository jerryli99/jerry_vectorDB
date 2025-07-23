#pragma once

#include "DataTypes.h"
#include "Segment.h"
#include "SegmentEntry.h"

/**
 * @brief 
 *      Low-level container: stores and manages access to segments (like a thread-safe map)...maybe 
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
    void addSegment(SegmentIdType id, SegmentEntry segment_entry) {
        m_segments.insert({id, segment_entry});
    }

    std::shared_ptr<Segment> getSegment(SegmentIdType id) const {
        auto it = m_segments.find(id);
        return (it != m_segments.end() ? it->second.segment : nullptr);
    }

    std::unordered_map<SegmentIdType, SegmentEntry> getAllSegments() const {
        return m_segments;
    }

private:
    std::unordered_map<SegmentIdType, SegmentEntry> m_segments;//not thread safe yet
};

} // namespace vectordb
