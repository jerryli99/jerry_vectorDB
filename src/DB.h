#pragma once

#include "DataTypes.h"
#include "Collection.h"
#include "CollectionContainer.h"
#include "Status.h"

namespace vectordb{
class DB {
    public:
        DB() = default;
        ~DB() = default; //?? ehm, we will see about this part. 
        //config collection obj
        Status addCollection(const CollectionId& collection_name, const json& config_json);
        json listCollections();
        Status deleteCollection(const CollectionId& collection_name);
        Status upsertPointToCollection(const CollectionId& collection_name, const json& points_json);
        
        Status upsertPoints(const CollectionId& collection_name, const PointIdType& point_id, 
                            const DenseVector& vector, const json& payload);
        //just overload the member function so i don't need to create a seperate one from scratch.
        Status upsertPoints(const CollectionId& collection_name, const PointIdType& point_id, 
                            const std::map<VectorName, DenseVector>& named_vectors, const json& payload);
        //size_t topK, collectioName
        void searchTopKInCollection(...);

    private:
        CollectionContainer container;
};

}