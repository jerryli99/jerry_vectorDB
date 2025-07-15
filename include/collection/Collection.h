#pragma once

#include "DataTypes.h"
#include "CollectionInfo.h"
#include "SegmentRegistry.h"
#include "Point.h" //not sure about this here...

namespace vectordb {

class Collection {
public:
    CollectionInfo info_; //include collection_id in this struct
    std::unique_ptr<SegmentRegistry> segment_register_;  // Or ShardMap in future

    void insert(...);
    std::vector<Point> search(...);
};

}

