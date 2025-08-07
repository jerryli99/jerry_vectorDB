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
    // std::optional<std::shared_ptr<Segment>> getSegment(SegmentIdType id) const;
    void searchSegmentsTopK(...);
    const std::vector<std::pair<SegmentIdType, SegmentEntry>> getAllSegments() const;

    //delete segment?

    //merge segment: find the least amount of vectors among the segments, and then merge them together to a new one

    
private:
    mutable std::shared_mutex map_mutex_;//use this later i guess for now ignore this.

    //I was going to use a hashmap for this, but then thought space might be not efficient because
    //I will need to search through each segment though. Of course, if my indexing method changed, this will change..
    std::vector<std::pair<SegmentIdType, SegmentEntry>> m_segments;//not thread safe yet
    
    //uhmm, if i want to lookup the segment id in the future, I could try to maintain a hashtable 
    //where the segment id is the key and the value is the index of std::vector where the segment is located 
    
    std::atomic<uint64_t> next_id_{0};//mayeb uuid is better for segment id?
};

} // namespace vectordb

/*
SegmentHolder seg_holder(...);
seg_holder.addSegment(..);


*/