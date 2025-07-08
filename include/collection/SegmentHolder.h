#pragma once

#include "DataTypes.h"
#include "Segment.h"

/**
 * @brief 
 *      
 *      Scope: Manages segments inside a single collection.
 *      Purpose: Acts as a manager or container of segments belonging to one collection.
 *      Responsibilities:
 *      Add, remove, access segments by segment ID.
 *      Coordinate segment lifecycle within that collection.
 *      Provide thread-safe access to the collection’s segments.
 *      Analogy: Like a book’s chapter list holding multiple chapters (segments).
 * 
 */
namespace vectordb {
    
    struct SegmentHolder {
        std::unordered_map<SegmentId, Segment> segmentholder_map;
        //this holder class should be like routing vectors to segments?
        
        //add concurrent map read/write access?
        //add minheap to store each segment's top_k score (which is the top_k smallest distance)
        //so that we can just get the first k iterations of the min heap
        
    };
}