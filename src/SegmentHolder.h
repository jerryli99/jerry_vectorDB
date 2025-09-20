#pragma once

// #include "DataTypes.h"
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
    SegmentHolder();
    ~SegmentHolder();

    
private:
    std::atomic<uint64_t> next_id_{0};//mayeb uuid is better for segment id?
};

} // namespace vectordb