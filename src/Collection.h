#pragma once

#include "DataTypes.h"
#include "CollectionInfo.h"
#include "PointPayloadStore.h"
#include "SegmentHolder.h"

// #include "SegmentRegistry.h"//later add this

//In the collection you will have a collection of segments where the points are stored.

namespace vectordb {

class Collection {
public:
    Collection(const CollectionId& id, const CollectionInfo& info);
    ~Collection() = default;

    Status insertPoint(PointIdType point_id, const DenseVector& vector, const Payload& payload);

    Status insertPoint(PointIdType point_id, 
                    const std::map<VectorName, DenseVector>& named_vectors,
                    const Payload& payload);

    const CollectionId& getId() const;
    const CollectionInfo& getInfo() const;
    SegmentHolder& getSegmentHolder();
    const SegmentHolder& getSegmentHolder() const;
    PointPayloadStore& getPayloadStore();

private:
    CollectionId m_collection_id;
    CollectionInfo m_collection_info;
    SegmentHolder m_segment_holder;
    PointPayloadStore m_point_payload;
};

}

