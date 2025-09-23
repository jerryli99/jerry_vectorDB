#include "Collection.h"

namespace vectordb {

    Collection::Collection(const CollectionId& id, const CollectionInfo& info) 
        : m_collection_info {info}, 
          m_segment_holder(/*max_points*/5000, /*indexSpec*/info.index_specs),
          m_point_payload("./vectordb/payload_" + id, CACHE_SIZE) //i might just add a base file path here instead of a hard coded one
    {}
    
    Status Collection::insertPoint(PointIdType point_id, const DenseVector& vector, const Payload& payload) 
    {
        auto status = m_segment_holder.insertPoint(point_id, vector);
        if (status.ok && !payload.empty()) {
            m_point_payload.putPayload(point_id, payload);
        }
        return status;
    }

    Status Collection::insertPoint(PointIdType point_id, 
                                  const std::map<VectorName, DenseVector>& named_vectors,
                                  const Payload& payload) 
    {
        auto status = m_segment_holder.insertPoint(point_id, named_vectors);
        if (status.ok && !payload.empty()) {
            m_point_payload.putPayload(point_id, payload);
        }
        return status;
    }

    const CollectionId& Collection::getId() const { return m_collection_id; }
    const CollectionInfo& Collection::getInfo() const { return m_collection_info; }
    SegmentHolder& Collection::getSegmentHolder() { return m_segment_holder; }
    const SegmentHolder& Collection::getSegmentHolder() const { return m_segment_holder; }
    PointPayloadStore& Collection::getPayloadStore() { return m_point_payload; }

}