#pragma once

#include "DataTypes.h"
#include "Collection.h"
#include "Point.h"

/**
 * @brief CollectionContainer will be a hashmap for storing the collections. 
 *        The ColllectionId will be a string as key, and collection as value. 
 * 
 *          Scope: Manages collections as a whole.
 *          Purpose: Acts as a top-level registry or container of multiple collections.
 *          Responsibilities:
 *          Add, remove, lookup collections by name or ID.
 *          Manage collection lifecycle (creation, deletion).
 *          Handle concurrency for collection-level operations.
 *          Analogy: Like a library shelf that holds many books (collections).
 */
namespace vectordb {

class CollectionContainer {
public:
    void addCollection(const CollectionId& collection_name, Collection& collection);
    void RemoveCollection(const CollectionId& collection_name);
    void lookupCollection(const CollectionId& collection_name); //if exist print info
    // void listAllCollectionInfo();

private:
    std::unordered_map<CollectionId, std::shared_ptr<Collection>> m_collections;
};

}