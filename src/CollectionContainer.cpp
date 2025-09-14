#include "CollectionContainer.h"
#include <mutex>

namespace vectordb {
    Status CollectionContainer::addCollection(const CollectionId& name, CollectionEntry entry) {
        std::unique_lock lock(m_mutex);
        auto [it, inserted] = m_collections.try_emplace(name, std::move(entry));
        return inserted ? Status::OK() : Status::Error("Collection exists");
    }

    std::optional<CollectionContainer::ReadAccess> 
    CollectionContainer::getCollectionForRead(const CollectionId& name) const {
        std::shared_lock read_lock(m_mutex);
        auto it = m_collections.find(name);
        if (it == m_collections.end()) {
            return std::nullopt;
        }
        
        // Create the lock first, then construct the pair
        std::shared_lock<std::shared_mutex> collection_lock(it->second.mutex);
        read_lock.unlock();
        
        return std::make_pair(&it->second, std::move(collection_lock));
    }

    std::optional<CollectionContainer::WriteAccess> 
    CollectionContainer::getCollectionForWrite(const CollectionId& name) {
        std::unique_lock write_lock(m_mutex);
        auto it = m_collections.find(name);
        if (it == m_collections.end()) {
            return std::nullopt;
        }
        
        std::unique_lock<std::shared_mutex> collection_lock(it->second.mutex);
        write_lock.unlock();
        
        return std::make_pair(&it->second, std::move(collection_lock));
    }

    std::shared_ptr<Collection> CollectionContainer::getCollectionPtr(const CollectionId& name) const {
        std::shared_lock lock(m_mutex);
        auto it = m_collections.find(name);
        if (it == m_collections.end()) {
            return nullptr;
        }
        return it->second.collection;
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
        return removed;
    }

    size_t CollectionContainer::size() const {
        std::shared_lock lock(m_mutex);
        return m_collections.size();
    }
}