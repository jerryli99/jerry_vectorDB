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
    //this function is temporary, i will use uuid lib later
    std::string generate_new_key() {
        //generate key, and if key exists in the holder, regenerate. I will do this later
        next_id_ = next_id_ + 1;
        return ("seg_" + std::to_string(next_id_));
    }

    //The segment gets assigned a new unique ID.
    void addSegment(std::shared_ptr<Segment> segment, SegmentType segment_type) {
        //generate new segment id, add segment to the holder
        SegmentEntry segment_entry {.segment_=segment, .type_=segment_type};
        SegmentIdType segment_id = generate_new_key();
        m_segments[segment_id] = segment_entry; //potential overwrite if segment id is messed up? or do i want to allow that?
    }

    std::optional<std::shared_ptr<Segment>> getSegment(SegmentIdType id) const {
        auto it = m_segments.find(id);
        if (it != m_segments.end()) {
            return it->second.segment_;
        } else {
            return std::nullopt;
        }
    }

    const std::unordered_map<SegmentIdType, SegmentEntry> getAllSegments() const {
        return m_segments;
    }

    //delete segment

    //merge segment: find the least amount of vectors among the segments, and then merge them together to a new one

private:
    mutable std::shared_mutex map_mutex_;//use this later i guess for now ignore this.
    std::unordered_map<SegmentIdType, SegmentEntry> m_segments;//not thread safe yet
    std::atomic<uint64_t> next_id_{0};//mayeb uuid is better for segment id?
};

} // namespace vectordb

/*
SegmentHolder seg_holder(...);
seg_holder.addSegment(..);


*/