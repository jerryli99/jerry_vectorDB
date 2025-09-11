#include "CollectionContainer.h"
#include <mutex>

namespace vectordb {
    Status CollectionContainer::addCollection(const CollectionId& name, CollectionEntry entry) {
        std::unique_lock lock(m_mutex);
        auto [it, inserted] = m_collections.try_emplace(name, std::move(entry));
        return inserted ? Status::OK() : Status::Error("Collection exists");
    }

    CollectionEntry* CollectionContainer::getCollection(const CollectionId& name) {
        std::shared_lock lock(m_mutex);
        auto it = m_collections.find(name);
        return (it != m_collections.end()) ? &it->second : nullptr;
    }

    const CollectionEntry* CollectionContainer::getCollection(const CollectionId& name) const {
        std::shared_lock lock(m_mutex);
        auto it = m_collections.find(name);
        return (it != m_collections.end()) ? &it->second : nullptr;
    }

    std::vector<CollectionId> CollectionContainer::getCollectionNames() const {
        std::shared_lock lock(m_mutex);
        std::vector<CollectionId> names;
        names.reserve(m_collections.size());
        for (const auto& [name, _] : m_collections) {
            names.push_back(name);
        }
        return names;
    }

    bool CollectionContainer::contains(const CollectionId& name) const {
        std::shared_lock lock(m_mutex);
        return m_collections.find(name) != m_collections.end();
    }

    bool CollectionContainer::removeCollection(const CollectionId& name) {
        std::unique_lock lock(m_mutex);
        bool removed = m_collections.erase(name) > 0;
        if (removed) {
            m_collection_mutexes.erase(name); // Clean up the mutex too
        }
        return removed;
    }
    // Get a collection's specific mutex for fine-grained locking
    std::shared_mutex& CollectionContainer::getCollectionMutex(const CollectionId& name) {
        std::shared_lock read_lock(m_mutex);
        auto it = m_collection_mutexes.find(name);
        if (it != m_collection_mutexes.end()) {
            return it->second;
        }
        
        // Upgrade to write lock if mutex doesn't exist
        read_lock.unlock();
        std::unique_lock write_lock(m_mutex);
        
        // Check again after acquiring write lock (double-check pattern)
        it = m_collection_mutexes.find(name);
        if (it == m_collection_mutexes.end()) {
            it = m_collection_mutexes.emplace(name, std::shared_mutex{}).first;
        }
        return it->second;
    }

    size_t CollectionContainer::size() const {
        std::shared_lock lock(m_mutex);
        return m_collections.size();
    }
}