/*
Ehm this file will be tossed into the inifite space deep deep down...
*/
























// /**
//  * @brief 
//  * The LRUCache class is indented to be used for query. 
//  * But i can modify it for the query vector.
//  * So query#1 might be similiar to query#2 vector, like 0.1 difference kind,
//  * then why not just do a cache quick query vector difference check to see if
//  * the 2 queries or more stored in the LRU cache are similiar, and if so, then
//  * simply retrieve it instead of adding a new one to the list?
//  * 
//  * [
//  *    Update (10/16/2025): I feel like this will be fine for low to mid rate queries.
//  *    But for large and frequent queries, this might be a bad idea as we can expect the
//  *    cache size to be big, perhaps this vector query cache might even deserve its own server for it. LOL.
//  *    So i will still keep this as an option though.
//  * 
//  *    std::unorderedmap<VectorName, LRUCache> cache;
//  *    another cache will be hash based cache. I will implement it later.
//  *    Maybe borrow from LSH (Locality Sensitive Hashing) and Product Quantization?
//  * ]
//  * 
//  * 
//  */
// #pragma once
// #include "DataTypes.h"
// #include "QueryResult.h"
// #include "CollectionInfo.h"
// #include "Distance.h"

// #include <vector>
// #include <string>
// #include <map>
// #include <chrono>
// #include <algorithm>
// #include <mutex>
// namespace vectordb {

// struct CacheEntry {
//     std::vector<float> query_vector;
//     QueryResult result;
//     std::chrono::steady_clock::time_point timestamp;
//     VectorName vector_name;
    
//     CacheEntry(std::vector<float> vec, QueryResult res, VectorName name)
//         : query_vector(std::move(vec)), 
//           result(std::move(res)), 
//           timestamp(std::chrono::steady_clock::now()),
//           vector_name(std::move(name)) {}
// };

// class LRUCache {
// private:
//     std::vector<CacheEntry> cache_entries_;
//     size_t capacity_;
//     mutable std::mutex mutex_;

//     // Helper to find entry by vector name and query vector
//     auto find_entry(const VectorName& vector_name, const std::vector<float>& query_vector) {
//         return std::find_if(cache_entries_.begin(), cache_entries_.end(),
//             [&](const CacheEntry& entry) {
//                 return entry.vector_name == vector_name && 
//                        entry.query_vector.size() == query_vector.size() &&
//                        std::equal(entry.query_vector.begin(), entry.query_vector.end(),
//                                  query_vector.begin());
//             });
//     }

// public:
//     LRUCache(size_t capacity) : capacity_(capacity) {}

//     // Put a query result into cache
//     void put(const VectorName& vector_name, 
//              const std::vector<float>& query_vector, 
//              QueryResult&& result) {
//         std::lock_guard<std::mutex> lock(mutex_);
        
//         // Check if entry already exists
//         auto it = find_entry(vector_name, query_vector);
//         if (it != cache_entries_.end()) {
//             // Update existing entry
//             it->result = std::move(result);
//             it->timestamp = std::chrono::steady_clock::now();
//             return;
//         }

//         // If cache is full, remove LRU entry
//         if (cache_entries_.size() >= capacity_) {
//             evict_lru();
//         }

//         // Add new entry
//         cache_entries_.emplace_back(query_vector, std::move(result), vector_name);
//     }

//     // Get cached result
//     bool get(const VectorName& vector_name, 
//              const std::vector<float>& query_vector, 
//              QueryResult& result) {
//         std::lock_guard<std::mutex> lock(mutex_);
        
//         auto it = find_entry(vector_name, query_vector);
//         if (it == cache_entries_.end()) {
//             return false;
//         }

//         // Update timestamp and return result
//         it->timestamp = std::chrono::steady_clock::now();
//         result = it->result;
//         return true;
//     }

//     // Check if query exists in cache
//     bool contains(const VectorName& vector_name, const std::vector<float>& query_vector) {
//         std::lock_guard<std::mutex> lock(mutex_);
//         return find_entry(vector_name, query_vector) != cache_entries_.end();
//     }

//     // Get current size
//     size_t size() const {
//         std::lock_guard<std::mutex> lock(mutex_);
//         return cache_entries_.size();
//     }

//     // Clear cache
//     void clear() {
//         std::lock_guard<std::mutex> lock(mutex_);
//         cache_entries_.clear();
//     }

//     // Clear cache for specific vector name
//     void clear(const VectorName& vector_name) {
//         std::lock_guard<std::mutex> lock(mutex_);
//         cache_entries_.erase(
//             std::remove_if(cache_entries_.begin(), cache_entries_.end(),
//                 [&](const CacheEntry& entry) {
//                     return entry.vector_name == vector_name;
//                 }),
//             cache_entries_.end());
//     }

//     // Get capacity
//     size_t capacity() const { return capacity_; }

// private:
//     void evict_lru() {
//         if (cache_entries_.empty()) return;

//         // Find the entry with the oldest timestamp
//         auto lru_it = cache_entries_.begin();
//         auto oldest_time = lru_it->timestamp;
        
//         for (auto it = cache_entries_.begin() + 1; it != cache_entries_.end(); ++it) {
//             if (it->timestamp < oldest_time) {
//                 oldest_time = it->timestamp;
//                 lru_it = it;
//             }
//         }

//         // Remove the LRU entry
//         cache_entries_.erase(lru_it);
//     }
// };

// } // namespace vectordb