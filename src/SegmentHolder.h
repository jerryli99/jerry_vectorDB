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
    SegmentHolder();
    ~SegmentHolder();

    std::string generate_new_key();
    void addSegment(std::shared_ptr<Segment> segment);
    std::optional<std::shared_ptr<Segment>> getSegment(SegmentIdType id) const;
    const std::unordered_map<SegmentIdType, SegmentEntry> getAllSegments() const;

    //delete segment

    //merge segment: find the least amount of vectors among the segments, and then merge them together to a new one

private:
    mutable std::shared_mutex map_mutex_;//use this later i guess for now ignore this.
    std::unordered_map<SegmentIdType, SegmentEntry> m_segments;//not thread safe yet or perhaps do object pool?
    std::atomic<uint64_t> next_id_{0};//mayeb uuid is better for segment id?
};

} // namespace vectordb

/*
SegmentHolder seg_holder(...);
seg_holder.addSegment(..);


*/