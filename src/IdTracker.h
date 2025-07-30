#pragma once

#include "DataTypes.h"

#include <unordered_map>
#include <vector>
#include <optional>
#include <cstddef>
#include <stack>

/*
Used for converting external index (PointIDType) to internal index (sequential) hnsw indexing

If we have 1000 segment objects, we have 1000 IdTracker objects
*/

namespace vectordb {

class IdTracker {
public:
    IdTracker() = default;
    ~IdTracker();

    std::optional<PointOffSetType> get_internal_id(PointIdType point_id) const;
    std::optional<PointIdType> get_external_id(PointOffSetType offset) const;

    PointOffSetType insert(PointIdType point_id);
    void remove(PointIdType point_id);

    std::vector<PointOffSetType> iter_internal_ids() const;
    std::vector<PointIdType> iter_external_ids() const;

    size_t size() const;
    bool empty() const;

private:
    std::map<PointIdType, PointOffSetType> point_id_to_offset_;
    std::vector<std::optional<PointIdType>> offset_to_point_id_;
    std::stack<PointOffSetType> free_slots_;
};

} // namespace vectordb