#pragma once

#include "BitmapIndex.h"
#include "DataTypes.h"
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>

//this idtracker will be used to do mapping between faiss index and point ids.
//maybe add reset(), integrityCheck(), thread safety, persistentSerialization() methods in future?
namespace vectordb {

class IdTracker {
public:
    IdTracker() = default;
    ~IdTracker() = default;

    // Initialize per vector name with expected size
    void init(const std::vector<VectorName>& vector_names, size_t expected_size) noexcept {
        for (const auto& name : vector_names) {
            m_tables[name].offset_to_pointid.resize(expected_size);
            m_tables[name].bitmap.resize(expected_size);
            m_tables[name].next_free_offset = 0;
        }
    }

    std::optional<PointOffSetType> getInternalId(const VectorName& name, PointIdType point_id) const {
        auto it_table = m_tables.find(name);
        if (it_table == m_tables.end()) return std::nullopt;

        const auto& tbl = it_table->second;
        auto it = tbl.point_id_to_offset.find(point_id);
        return (it != tbl.point_id_to_offset.end()) ? std::make_optional(it->second) : std::nullopt;
    }

    std::optional<PointIdType> getExternalId(const VectorName& name, PointOffSetType offset) const {
        auto it_table = m_tables.find(name);
        if (it_table == m_tables.end()) return std::nullopt;

        const auto& tbl = it_table->second;
        if (offset < tbl.offset_to_pointid.size() && tbl.bitmap.get(offset)) {
            return tbl.offset_to_pointid[offset];
        }
        return std::nullopt;
    }

    PointOffSetType insert(const VectorName& name, PointIdType point_id) {
        auto& tbl = m_tables[name]; // creates if not exists

        if (auto existing = getInternalId(name, point_id))
            return *existing;

        PointOffSetType offset;
        if (tbl.next_free_offset < tbl.offset_to_pointid.size()) {
            offset = tbl.next_free_offset;
            findNextFree(tbl);
            tbl.offset_to_pointid[offset] = point_id;
        } else {
            offset = tbl.offset_to_pointid.size();
            tbl.offset_to_pointid.push_back(point_id);
            tbl.bitmap.resize(tbl.offset_to_pointid.size());
        }

        tbl.bitmap.set(offset, true);
        tbl.point_id_to_offset[point_id] = offset;
        return offset;
    }

    void remove(const VectorName& name, PointIdType point_id) {
        auto it_table = m_tables.find(name);
        if (it_table == m_tables.end()) return;

        auto& tbl = it_table->second;
        auto it = tbl.point_id_to_offset.find(point_id);
        if (it == tbl.point_id_to_offset.end()) return;

        PointOffSetType offset = it->second;
        tbl.bitmap.set(offset, false);
        tbl.offset_to_pointid[offset] = std::nullopt;
        tbl.point_id_to_offset.erase(it);

        if (offset < tbl.next_free_offset) tbl.next_free_offset = offset;
    }

    std::vector<PointOffSetType> iterInternalIds(const VectorName& name) const {
        std::vector<PointOffSetType> ids;
        auto it_table = m_tables.find(name);
        if (it_table == m_tables.end()) return ids;

        const auto& tbl = it_table->second;
        for (size_t i = 0; i < tbl.offset_to_pointid.size(); ++i) {
            if (tbl.bitmap.get(i)) {
                ids.push_back(i);
            }
        }
        return ids;
    }

    std::vector<PointIdType> iterExternalIds(const VectorName& name) const {
        std::vector<PointIdType> ids;
        auto it_table = m_tables.find(name);
        if (it_table == m_tables.end()) return ids;

        const auto& tbl = it_table->second;
        ids.reserve(tbl.point_id_to_offset.size());
        for (const auto& [pid, _] : tbl.point_id_to_offset) {
            ids.push_back(pid);
        }
        return ids;
    }

    size_t size(const VectorName& name) const { 
        auto it_table = m_tables.find(name);
        return (it_table != m_tables.end()) ? it_table->second.point_id_to_offset.size() : 0;
    }

    bool empty(const VectorName& name) const { 
        return size(name) == 0; 
    }

    const BitmapIndex& bitmap(const VectorName& name) const { 
        return m_tables.at(name).bitmap; 
    }

private:
    struct Table {
        std::map<PointIdType, PointOffSetType> point_id_to_offset;
        std::vector<std::optional<PointIdType>> offset_to_pointid;
        BitmapIndex bitmap;
        size_t next_free_offset = 0;
    };

    std::unordered_map<VectorName, Table> m_tables;

    static void findNextFree(Table& tbl) {
        for (; tbl.next_free_offset < tbl.bitmap.size(); ++tbl.next_free_offset) {
            if (!tbl.bitmap.get(tbl.next_free_offset))
                return;
        }
    }
};

} // namespace vectordb



// #pragma once

// #include "BitmapIndex.h"
// #include "DataTypes.h"
// #include <map>
// #include <optional>
// #include <vector>

// namespace vectordb {

// class IdTracker {
// public:
//     IdTracker() = default;
//     ~IdTracker() = default;

//     void init(size_t expected_size) noexcept {
//         m_offset_to_pointid.resize(expected_size);
//         m_bitmap.resize(expected_size);
//         m_next_free_offset = 0;
//     }

//     std::optional<PointOffSetType> get_internal_id(PointIdType point_id) const {
//         auto it = m_point_id_to_offset.find(point_id);
//         return (it != m_point_id_to_offset.end()) ? std::make_optional(it->second) : std::nullopt;
//     }

//     std::optional<PointIdType> get_external_id(PointOffSetType offset) const {
//         if (offset < m_offset_to_pointid.size() && m_bitmap.get(offset)) {
//             return m_offset_to_pointid[offset];
//         }
//         return std::nullopt;
//     }

//     PointOffSetType insert(PointIdType point_id) {
//         if (auto existing = get_internal_id(point_id))
//             return *existing;

//         PointOffSetType offset;
//         if (m_next_free_offset < m_offset_to_pointid.size()) {
//             //there’s space (deleted slot)
//             offset = m_next_free_offset;
//             find_next_free(); // move to next
//             m_offset_to_pointid[offset] = point_id;
//         } else {
//             offset = m_offset_to_pointid.size();
//             m_offset_to_pointid.push_back(point_id);
//             m_bitmap.resize(m_offset_to_pointid.size());
//         }

//         m_bitmap.set(offset, true);
//         m_point_id_to_offset[point_id] = offset;
//         return offset;
//     }

//     void remove(PointIdType point_id) {
//         auto it = m_point_id_to_offset.find(point_id);
//         if (it == m_point_id_to_offset.end()) return;

//         PointOffSetType offset = it->second;
//         m_bitmap.set(offset, false);
//         m_offset_to_pointid[offset] = std::nullopt;
//         m_point_id_to_offset.erase(it);

//         // Optionally track earliest free offset
//         if (offset < m_next_free_offset) m_next_free_offset = offset;
//     }

//     std::vector<PointOffSetType> iter_internal_ids() const {
//         std::vector<PointOffSetType> ids;
//         for (size_t i = 0; i < m_offset_to_pointid.size(); ++i) {
//             if (m_bitmap.get(i)) {
//                 ids.push_back(i);
//             }
//         }
//         return ids;
//     }

//     std::vector<PointIdType> iter_external_ids() const {
//         std::vector<PointIdType> ids;
//         ids.reserve(m_point_id_to_offset.size());
//         for (const auto& [pid, _] : m_point_id_to_offset) {
//             ids.push_back(pid);
//         }
//         return ids;
//     }

//     size_t size() const { 
//         return m_point_id_to_offset.size(); 
//     }

//     bool empty() const { 
//         return m_point_id_to_offset.empty(); 
//     }

//     const BitmapIndex& bitmap() const { 
//         return m_bitmap; 
//     }

// private:
//     void find_next_free() {
//         for (; m_next_free_offset < m_bitmap.size(); ++m_next_free_offset) {
//             if (!m_bitmap.get(m_next_free_offset))
//                 return;
//         }
//     }

// private:
//     std::map<PointIdType, PointOffSetType> m_point_id_to_offset;
//     std::vector<std::optional<PointIdType>> m_offset_to_pointid;
//     BitmapIndex m_bitmap;
//     size_t m_next_free_offset = 0; // track where to start searching for free space
// };

// } // namespace vectordb
