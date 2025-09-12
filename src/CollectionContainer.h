#pragma once

#include "DataTypes.h"
#include "Collection.h"
#include "Status.h"
#include <shared_mutex>
#include <mutex>
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
    std::shared_ptr<Collection> collection;
    json config;
    mutable std::shared_mutex mutex;
    
    // Delete copy operations
    CollectionEntry(const CollectionEntry&) = delete;
    CollectionEntry& operator=(const CollectionEntry&) = delete;
    
    // Custom move operations (don't try to move the mutex)
    CollectionEntry(CollectionEntry&& other) noexcept
        : collection(std::move(other.collection))
        , config(std::move(other.config))
        // mutex is not moved - it remains default-constructed
    {}
    
    CollectionEntry& operator=(CollectionEntry&& other) noexcept {
        if (this != &other) {
            collection = std::move(other.collection);
            config = std::move(other.config);
            // mutex is not moved
        }
        return *this;
    }
    
    CollectionEntry() = default;
};

class CollectionContainer {
public:
    using ReadAccess = std::pair<const CollectionEntry*, std::shared_lock<std::shared_mutex>>;
    using WriteAccess = std::pair<CollectionEntry*, std::unique_lock<std::shared_mutex>>;

    CollectionContainer() = default;
    ~CollectionContainer() = default;

    Status addCollection(const CollectionId& name, CollectionEntry entry);
    
    std::optional<ReadAccess> getCollectionForRead(const CollectionId& name) const;
    std::optional<WriteAccess> getCollectionForWrite(const CollectionId& name);
    
    // Helper to get just the collection with proper locking
    std::shared_ptr<Collection> getCollectionPtr(const CollectionId& name) const;

    // CollectionEntry* getCollection(const CollectionId& name);
    // const CollectionEntry* getCollection(const CollectionId& name) const;

    std::vector<CollectionId> getCollectionNames() const;

    bool contains(const CollectionId& name) const;
    bool removeCollection(const CollectionId& name);

    // Get a collection's specific mutex for fine-grained locking
    // std::shared_mutex& getCollectionMutex(const CollectionId& name);

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
private:       
    mutable std::shared_mutex m_mutex; //for read-write lock, multiple reads (shared lock), 1 write (unique lock),
    // mutable std::unordered_map<CollectionId, std::unique_ptr<std::shared_mutex>> m_collection_mutexes; //mutable allows const methods to lock
    //well since i am already using unordered_map, i think add, remove, lookup is handled?
    std::unordered_map<CollectionId, CollectionEntry> m_collections;
};

}