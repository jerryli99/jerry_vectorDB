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

