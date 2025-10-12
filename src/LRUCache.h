/**
 * @brief 
 * The LRUCache class is indented to be used for query. 
 * But i can modify it for the query vector.
 * So query#1 might be similiar to query#2 vector, like 0.1 difference kind,
 * then why not just do a cache quick query vector difference check to see if
 * the 2 queries or more stored in the LRU cache are similiar, and if so, then
 * simply retrieve it instead of adding a new one to the list?
 * 
 */
#pragma once

#include "Distance.h"
#include <list>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <vector>
#include <string>

namespace vectordb {
using QueryResult = json;

// Represents one cached query entry
struct QueryCacheEntry {
    std::vector<float> query_vector;
    DistanceMetric metric;
    float similarity_threshold; //configurable per entry (e.g. 0.98 cosine or small L2 diff)
    QueryResult result;         //whatever type
};

// LRU cache with approximate vector lookup
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : m_capacity(capacity) {}

    /**
     * Try to find a similar query in cache.
     * For COSINE: higher is more similar.
     * For L2: smaller is more similar.
     */
    std::optional<QueryResult> getApproximate(
        const std::vector<float>& query_vec,
        DistanceMetric metric,
        float similarity_threshold = 0.98f)  // e.g., for cosine
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto it = m_list.begin(); it != m_list.end(); ++it) {
            const auto& entry = *it;
            if (entry.metric != metric || entry.query_vector.size() != query_vec.size())
                continue;

            float sim = compute_distance(metric, entry.query_vector, query_vec);

            bool is_similar = false;
            if (metric == DistanceMetric::COSINE) {
                is_similar = sim >= similarity_threshold;
            } else if (metric == DistanceMetric::L2) {
                is_similar = sim <= (1.0f - similarity_threshold); // or some absolute threshold
            } else if (metric == DistanceMetric::DOT) {
                is_similar = sim >= similarity_threshold;
            }

            if (is_similar) {
                // LRU update: move to front
                m_list.splice(m_list.begin(), m_list, it);
                return entry.result;
            }
        }
        return std::nullopt; // cache miss
    }

    /**
     * Insert new query result or update an existing similar one.
     */
    void put(const std::vector<float>& query_vec,
             DistanceMetric metric,
             const QueryResult& result,
             float similarity_threshold = 0.98f)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // If similar entry exists, update it
        for (auto it = m_list.begin(); it != m_list.end(); ++it) {
            auto& entry = *it;
            if (entry.metric != metric || entry.query_vector.size() != query_vec.size())
                continue;

            float sim = compute_distance(metric, entry.query_vector, query_vec);
            bool is_similar = (metric == DistanceMetric::COSINE && sim >= similarity_threshold) ||
                              (metric == DistanceMetric::L2 && sim <= (1.0f - similarity_threshold)) ||
                              (metric == DistanceMetric::DOT && sim >= similarity_threshold);

            if (is_similar) {
                // Update result and move to front
                entry.result = result;
                m_list.splice(m_list.begin(), m_list, it);
                return;
            }
        }

        // Otherwise, insert new entry
        m_list.emplace_front(QueryCacheEntry{query_vec, metric, similarity_threshold, result});

        // Evict least recently used if over capacity
        if (m_list.size() > m_capacity)
            m_list.pop_back();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_list.size();
    }

private:
    mutable std::mutex m_mutex;
    size_t m_capacity;
    std::list<QueryCacheEntry> m_list; // front = most recently used
};

} // namespace vectordb


// #pragma once

// #include <list>
// #include <unordered_map>
// #include <mutex>
// #include <optional>

// template <typename Key, typename Value>
// class LRUCache {
// public:
//     explicit LRUCache(size_t capacity) : m_capacity(capacity) {}

//     //Insert or update a key-value pair, and make it the most recently used.
//     void put(const Key& key, Value value) {
//         std::lock_guard<std::mutex> lock(m_mutex);

//         auto it = m_map.find(key);
//         if (it != m_map.end()) {
//             m_list.erase(it->second);
//             m_map.erase(it);
//         }

//         m_list.emplace_front(key, std::move(value));
//         m_map[key] = m_list.begin();//Update the hash map to point to that new list node.

//         //If we exceed capacity: Find the last element (end() - 1 = least recently used). 
//         //Remove it from both m_map and m_list.
//         if (m_map.size() > m_capacity) {
//             auto last = m_list.end();
//             --last;
//             m_map.erase(last->first);
//             m_list.pop_back();
//         }
//     }

//     //Retrieve value if present, and mark it as most recently used.
//     std::optional<Value> get(const Key& key) {
//         std::lock_guard<std::mutex> lock(m_mutex);

//         auto it = m_map.find(key);
//         if (it == m_map.end()) return std::nullopt;

//         // Move to front (most recently used)
//         // splice is O(1) since it just changes pointers in the linked list.
//         m_list.splice(m_list.begin(), m_list, it->second);
//         return it->second->second;
//     }

//     bool contains(const Key& key) {
//         std::lock_guard<std::mutex> lock(m_mutex);
//         return m_map.find(key) != m_map.end();
//     }

//     size_t size() const {
//         std::lock_guard<std::mutex> lock(m_mutex);
//         return m_map.size();
//     }

// private:
//     mutable std::mutex m_mutex;
//     size_t m_capacity;
//     std::list<std::pair<Key, Value>> m_list;  // front = most recent
//     // a hash map that maps from a Key to an iterator pointing to a node in the linked list
//     std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> m_map;
// };
