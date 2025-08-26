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
        Status upsertPointToCollection(const CollectionId& collection_name, ...);

        //size_t topK, collectioName
        void searchTopKInCollection(...);

    private:
        CollectionContainer container;
};

}