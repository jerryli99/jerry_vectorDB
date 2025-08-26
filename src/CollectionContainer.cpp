#include "CollectionContainer.h"

namespace vectordb {
    void CollectionContainer::addCollection(const CollectionId& collection_name, std::unique_ptr<Collection> collection) {
        if (m_collections.find(collection_name) != m_collections.end()) {
            throw std::runtime_error("Collection \"" + collection_name + "\" already exists.");
        }
        m_collections[collection_name] = std::move(collection);//transfer ownership 

    }

    void CollectionContainer::RemoveCollection(const CollectionId& collection_name) {
        auto it = m_collections.find(collection_name);
        if (it != m_collections.end()) {
            m_collections.erase(it);
        } else {
            throw std::runtime_error("Collection \"" + collection_name + "\" cannot be found to be deleted.");
        }
    }

    void CollectionContainer::lookupCollection(const CollectionId& collection_name) {
        auto it = m_collections.find(collection_name);
        if (it != m_collections.end()) {
            // std::cout << "Collection \"" << collection_name << "\" found:\n";
            it->second->printInfo();
        } else {
            throw std::runtime_error("Collection \"" + collection_name + "\" cannot be found.");
        }
    }

    // void listAllCollectionInfo() {
    //     if (m_collections.empty()) {
    //         std::cout << "No collections available.\n";
    //         return;
    //     }

    //     std::cout << "Listing all collections:\n";
    //     for (const auto& [name, collection] : m_collections) {
    //         std::cout << "- " << name << ": ";
    //         collection->printInfo();
    //     }
    // }
}