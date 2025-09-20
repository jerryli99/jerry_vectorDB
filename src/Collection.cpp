#include "Collection.h"

namespace vectordb {

    Collection::Collection(const CollectionId& id, const CollectionInfo& info) 
        : m_collectionid {id}, 
          m_collection_info {info}, 
          m_point_payload(PAYLOAD_DIR, CACHE_SIZE) 
    {}

    void Collection::addSegments(...) {
        //todo
        //create segment obj
        
             
    }

    void Collection::searchSegments(...) {
        //todo
    }

    void Collection::printInfo() {
        //print the m_collection_info here
    }

}