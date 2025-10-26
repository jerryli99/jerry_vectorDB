#pragma once

#include "DataTypes.h"
#include "Collection.h"
#include "CollectionContainer.h"
#include "Status.h"

/*
I will be using the Singleton Design Pattern for my DB class
*/
namespace vectordb {
class DB {
public:
    // Delete copy constructor and assignment operator to prevent copying
    DB(const DB&) = delete;
    DB& operator=(const DB&) = delete;
    
    // Static method to get the single instance
    static DB& getInstance() {
        static DB instance; //Created only once (thread-safe sice C++11)
        return instance;
    }

    Status addCollection(const CollectionId& collection_name, const json& config_json);
    Status deleteCollection(const CollectionId& collection_name);
    Status upsertPointsToCollection(const CollectionId& collection_name, const json& points_json);
    
    json listCollections();
    json queryCollection(const std::string& collection_name, const json& query_body, 
                         const std::string& using_index, std::size_t top_k);

    // Graph operations, uhm the method names maybe bad for some people hehe, but i think is fine for now.
    Status addGraphRelationship(const std::string& collection_name, 
                                PointIdType from_id, PointIdType to_id,
                                const std::string& relationship, float weight = 1.0f);
    
    json getNodeRelationships(const std::string& collection_name, PointIdType node_id);
    json graphTraversal(const std::string& collection_name, 
                        PointIdType start_id,
                        const std::string& direction = "outwards",
                        int max_hops = 2,
                        float min_weight = 0.0f);

    json findShortestPath(const std::string& collection_name, 
                          PointIdType start_id, PointIdType end_id);
    json findRelatedByWeight(const std::string& collection_name, 
                             PointIdType point_id, float min_weight = 0.7f);

    json getGraphData(const std::string& collection_name);

private:
    //Private constructor - can only be created internally
    //Prevents doing DB db; or auto db = DB(); or auto another_db = new DB();
    DB() = default;
    ~DB() = default;
    
    CollectionContainer container;
    
    std::pair<VectorSpec, Status> parseVectorSpec(const std::string& name, const json& config);
    StatusOr<DenseVector> validateVector(const VectorName& name, const json& jvec, 
                                         const CollectionInfo& collection_info);
    
    // Upsert helpers
    Status upsertPoints(std::shared_ptr<Collection>& collection, 
                        const PointIdType& point_id, 
                        const DenseVector& vector,
                        const Payload& payload);
    
    Status upsertPoints(std::shared_ptr<Collection>& collection, 
                        const PointIdType& point_id, 
                        const std::map<VectorName, DenseVector>& named_vectors,
                        const Payload& payload);
};

} // namespace vectordb