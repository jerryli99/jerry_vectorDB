#pragma once

#include "DataTypes.h"
#include "CollectionInfo.h"
#include "PointPayloadStore.h"

// #include "SegmentRegistry.h"//later add this
// #include "SegmentHolder.h"

// #include "Point.h" //not sure about this here...

/*
In the collection you will have a collection of segments where the points are stored.
*/
namespace vectordb {

    class Collection {
        public:
            Collection(const CollectionId& id, const CollectionInfo& info);
            ~Collection() = default;

            //need to figure out the params here to interact with seg_holder.
            //maybe pass in segment object
            void addSegments(...);
            void searchSegments(...);
            void printInfo();

        public: //not sure if this is a good idea, but i could fix this later... after prototype works
            CollectionId m_collectionid;
            CollectionInfo m_collection_info; //include collection_id in this struct
            PointPayloadStore m_point_payload;

            //later use segment_registry, but for now just segmentholder
            //shouldn’t go into CollectionInfo because you don’t want metadata to depend on runtime memory objects.
            // SegmentHolder m_seg_holder;
    };

}

