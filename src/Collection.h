#pragma once

#include "DataTypes.h"
#include "CollectionInfo.h"
#include "PointPayloadStore.h"
#include "SegmentHolder.h"
#include "VectorGraph.h"

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

    QueryResult searchTopK(const std::string& vector_name,
                           const std::vector<DenseVector>& query_vectors, 
                           size_t k) const;

    const CollectionId& getId() const;
    const CollectionInfo& getInfo() const;
    SegmentHolder& getSegmentHolder();
    const SegmentHolder& getSegmentHolder() const;
    PointPayloadStore& getPayloadStore();
    
    // Graph operations
    Status addGraphNode(PointIdType point_id, const std::string& named_vector = "default");
    Status addGraphRelationship(PointIdType from_id, PointIdType to_id, 
                               const std::string& relationship, float weight = 1.0f);
    
    std::vector<GraphEdge> getNodeRelationships(PointIdType node_id) const;
    std::vector<PointIdType> graphTraversal(PointIdType start_id, 
                                           const std::string& direction = "outwards",
                                           int max_hops = 2, 
                                           float min_weight = 0.0f);

    std::vector<PointIdType> findShortestPath(PointIdType start_id, PointIdType end_id);
    std::vector<PointIdType> findRelatedByWeight(PointIdType point_id, float min_weight = 0.7f);
    VectorGraph& getGraph();//??

    json getGraphData() const;
    bool nodeExistsInGraph(PointIdType point_id) const;

private:
    CollectionId m_collection_id;
    CollectionInfo m_collection_info;
    SegmentHolder m_segment_holder;
    PointPayloadStore m_point_payload;
    VectorGraph m_graph;  // Each collection has its own graph
};

}

