#pragma once

#include "DataTypes.h"
#include "Collection.h"
#include "CollectionContainer.h"
#include "Status.h"

// #include <memory>

namespace vectordb{
class DB {
    public:
        DB() = default;
        ~DB() = default; //?? ehm, we will see about this part. 
        
        //helper for addCollection
        std::pair<VectorSpec, Status> parseVectorSpec(const std::string& name, const json& config);

        //config collection obj
        Status addCollection(const CollectionId& collection_name, const json& config_json);

        json listCollections();
        
        Status deleteCollection(const CollectionId& collection_name);
        
        //helper for upsertPointsToCollection
        StatusOr<DenseVector> validateVector(const VectorName& name, const json& jvec, 
                                             const CollectionInfo& collection_info);
        
        Status upsertPointsToCollection(const CollectionId& collection_name, const json& points_json);
        
        //upsert for single vector 
        Status upsertPoints(std::shared_ptr<Collection>& collection, 
                            const PointIdType& point_id, 
                            const DenseVector& vector,
                            const Payload& payload);
        
        //upsert for multiple named vectors, overload the member function
        Status upsertPoints(std::shared_ptr<Collection>& collection, 
                            const PointIdType& point_id, 
                            const std::map<VectorName, DenseVector>& named_vectors,
                            const Payload& payload);
        
        //size_t topK, collectioName
        void searchTopKInCollection(...);

    private:
        CollectionContainer container;
};

}