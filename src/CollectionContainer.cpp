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
        std::unique_lock lock(m_mutex); // Exclusive access for removal
        return m_collections.erase(name) > 0;
    }
    // Get a collection's specific mutex for fine-grained locking
    std::shared_mutex& CollectionContainer::getCollectionMutex(const CollectionId& name) {
        std::shared_lock lock(m_mutex);
        return m_collection_mutexes[name]; // Creates if doesn't exist
    }

    size_t CollectionContainer::size() const {
        std::shared_lock lock(m_mutex);
        return m_collections.size();
    }
}