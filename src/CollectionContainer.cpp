#include "CollectionContainer.h"

namespace vectordb {
    size_t CollectionContainer::size() const {
        // std::shared_lock lock(mutex_);
        return m_collections.size();
    }
}