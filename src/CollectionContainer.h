#pragma once

#include "DataTypes.h"
#include "Collection.h"
// #include "Point.h"

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

struct CollectionEntry {
    std::unique_ptr<Collection> collection;
    json config; //collection's json config

    // Useful metadata
    // size_t num_points = 0;
    // Vector clock for causal reasoning and conflict resolution
    // VectorClock version;
};

class CollectionContainer {
public:
    CollectionContainer() = default;
    ~CollectionContainer() = default;
    //well since i am already using unordered_map, i think add, remove, lookup is handled?
    std::unordered_map<CollectionId, CollectionEntry> m_collections;
    size_t size() const;
    //maybe have something else here...
    // json get_stats() const 
    // size_t estimate_memory_usage() const
    // cleanup_expired_collections()
    // bool can_create_more_collections() const
    // void restore_from_snapshot(const json& snapshot)
    // json snapshot() const //well this guy will be in snapshot class i think, i just put it here...
    // bool update_collection_config(const CollectionId& id, const json& new_config, 
    //                              const VectorClock& incoming_version)       
};

}