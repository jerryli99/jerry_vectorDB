#pragma once

#include "DataTypes.h"
#include "CollectionContainer.h"

namespace vectordb{
class DB {
    public:
        DB() = default;
        ~DB() = default; //?? ehm, we will see about this part. 
        //config collection obj
        void addCollection(...);
        void listCollections(...);
        void deleteCollection(...);
        void upsertPointToCollection(...);

        //size_t topK, collectioName
        void searchTopKInCollection(...);

    private:
        CollectionContainer m_collection_container;
};

}