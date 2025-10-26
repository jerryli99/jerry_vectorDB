#include "Collection.h"

namespace vectordb {

    Collection::Collection(const CollectionId& id, const CollectionInfo& info) 
        : m_collection_info {info}, 
          m_segment_holder(/*max_points*/MAX_MEMORYPOOL_POINTS, /*collectionInfo*/info),
          m_point_payload("./vectordb/" + info.name + "/payload_" + id, CACHE_SIZE) //i might just add a base file path here instead of a hard coded one
    {}
    
    Status Collection::insertPoint(PointIdType point_id, const DenseVector& vector, const Payload& payload) 
    {
        auto status = m_segment_holder.insertPoint(point_id, vector);
        if (status.ok && !payload.empty()) {
            m_point_payload.putPayload(point_id, payload);//ignore payload now
        }
        return status;
    }

    Status Collection::insertPoint(PointIdType point_id, 
                                  const std::map<VectorName, DenseVector>& named_vectors,
                                  const Payload& payload) 
    {
        auto status = m_segment_holder.insertPoint(point_id, named_vectors);
        if (status.ok && !payload.empty()) {
            m_point_payload.putPayload(point_id, payload);//ignore payload now
        }
        return status;
    }

    QueryResult Collection::searchTopK(const std::string& vector_name,
                                       const std::vector<DenseVector>& query_vectors,
                                       size_t k) const 
    {
        return m_segment_holder.searchTopK(vector_name, query_vectors, k);
    }

    const CollectionId& Collection::getId() const { return m_collection_id; }
    const CollectionInfo& Collection::getInfo() const { return m_collection_info; }
    SegmentHolder& Collection::getSegmentHolder() { return m_segment_holder; }
    const SegmentHolder& Collection::getSegmentHolder() const { return m_segment_holder; }
    PointPayloadStore& Collection::getPayloadStore() { return m_point_payload; }

    // Graph operations
    Status Collection::addGraphNode(PointIdType point_id, const std::string& named_vector) {
        // Verify point exists in vector store first
        // if (!m_segment_holder.pointExists(point_id)) {
        //     return {false, "Point does not exist in collection"};
        // }

        m_graph.add_node(point_id, named_vector);
        return {true, ""};
    }

    Status Collection::addGraphRelationship(PointIdType from_id, PointIdType to_id, 
                                        const std::string& relationship, float weight) {
        // Verify both points exist
        // if (!m_segment_holder.pointExists(from_id)) {
        //     return {false, "Source point does not exist in collection"};
        // }
        // if (!m_segment_holder.pointExists(to_id)) {
        //     return {false, "Target point does not exist in collection"};
        // }
        
        // Ensure nodes exist in graph (they might not if only added via relationships)
        if (!m_graph.node_exists(from_id)) {
            m_graph.add_node(from_id, "auto_added");
        }
        if (!m_graph.node_exists(to_id)) {
            m_graph.add_node(to_id, "auto_added");
        }
        
        m_graph.add_relationship(from_id, to_id, relationship, weight);
        return {true, ""};
    }

    std::vector<GraphEdge> Collection::getNodeRelationships(PointIdType node_id) const {
        if (!m_graph.node_exists(node_id)) {
            return {};
        }
        return m_graph.get_node_relationships(node_id);
    }

    std::vector<PointIdType> Collection::graphTraversal(PointIdType start_id, 
                                                    const std::string& direction,
                                                    int max_hops, 
                                                    float min_weight){
        if (!m_graph.node_exists(start_id)) {
            return {};
        }
        
        if (direction == "outwards") {
            return m_graph.traverse_outwards(start_id, max_hops);
        } else if (direction == "inwards") {
            return m_graph.traverse_inwards(start_id, max_hops);
        } else {
            // For "both" direction, combine both traversals
            auto outwards = m_graph.traverse_outwards(start_id, max_hops);
            auto inwards = m_graph.traverse_inwards(start_id, max_hops);
            
            std::unordered_set<PointIdType> combined;
            combined.insert(outwards.begin(), outwards.end());
            combined.insert(inwards.begin(), inwards.end());
            
            return std::vector<PointIdType>(combined.begin(), combined.end());
        }
    }

    std::vector<PointIdType> Collection::findShortestPath(PointIdType start_id, PointIdType end_id){
        if (!m_graph.node_exists(start_id) || !m_graph.node_exists(end_id)) {
            return {};
        }
        return m_graph.find_shortest_path(start_id, end_id);
    }

    std::vector<PointIdType> Collection::findRelatedByWeight(PointIdType point_id, float min_weight){
        if (!m_graph.node_exists(point_id)) {
            return {};
        }
        return m_graph.get_strongly_connected(point_id, min_weight);
    }

    json Collection::getGraphData() const {
        return m_graph.to_json();
    }

    bool Collection::nodeExistsInGraph(PointIdType point_id) const {
        return m_graph.node_exists(point_id);
    }

}