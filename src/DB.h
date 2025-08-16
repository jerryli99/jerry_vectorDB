#pragma once

#include "DataTypes.h"
#include "CollectionContainer.h"

namespace vectordb{
    class DB {
        public:
            DB() = default;
            ~DB() = default;
            //config collection obj
            void createCollection(...);
            void listCollections(...);
            void deleteCollection(...);
            void upsertToCollection(...);

            //size_t topK, collectioName
            void searchTopKInCollection(...);

        private:
            CollectionContainer m_collection_container;
    };

}