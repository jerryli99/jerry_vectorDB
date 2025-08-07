#pragma once

#include "DataTypes.h"
#include "CollectionInfo.h"
#include "SegmentRegistry.h"
#include "SegmentHolder.h"

// #include "Point.h" //not sure about this here...

/*
In the collection you will have a collection of segments where the points are stored.

Since we have named vectors

*/

namespace vectordb {

class Collection {
public:
    Collection();
    ~Collection();

    //need to figure out the params here to interact with seg_holder.
    //maybe pass in segment object
    void addSegments(...);
    void searchSegments(...);
    void printInfo();

private:
    CollectionId m_collectionid;
    CollectionInfo m_collection_info; //include collection_id in this struct
    //later use segment_registry, but for now just segmentholder
    SegmentHolder m_seg_holder;
    //since i allow namedvectors, then a collection can have multiple dims of the same data
    size_t collection_dim_list[MAX_ENTRIES_TINYMAP];
};

}

