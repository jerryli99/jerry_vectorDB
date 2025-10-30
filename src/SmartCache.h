/*
The idea is that i am tired of rigid data structures as they are predesigned to serve a specific use case, 
and i think that all the data structures we created works because we kind of have to adapt to what the computer is capable of doing, 
but now how about we turn the table and let the computer adapt to what we want to do?

In order to make the computer adapt to what we want, in this case, cache the results and have the hit rate as much as possible so we don't waste 
the time querying the same vector, we will need the computer to "figure out, make a decision" in the limited memory space to say "oh, it is likely
the user of this vectordb collection accesses this xyz vector, i need to cache it longer and evict the least useful one for the user, etc". 

LRU and LFU if i am correct are trying to solve this but seems like the "really hot data" always keeps the advantage and occupy precious memory space for blocking
new data to come in and have a seat. 

I learned in school that LLMs have self-attention mechanism. I reviewed some ideas from the paper "Attention is all you need" and was excited. 
So what if we can do some calculations to see if this xyz cached data is worth paying attention to keep it in the cache or not?

Haha, so maybe this is not a big deal to some people, but i also was reminded that using log() and multipliers can help 
reduce the "hotness weight" and amplify the "significance weight" of data staying in the cache memory. 

So, attention aware cache is thus created. 

*/

#pragma once
#include "QueryResult.h"
#include "Status.h"
#include "DataTypes.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <chrono>
#include <atomic>
#include <mutex>
#include <cmath>

namespace vectordb {

using VectorName = std::string;

struct CacheEntry {
    std::vector<float> query_vector;
    QueryResult result;
    std::chrono::steady_clock::time_point timestamp;
    VectorName vector_name;
    size_t access_count = 1;
    
    CacheEntry(std::vector<float> vec, QueryResult res, VectorName name)
        : query_vector(std::move(vec)), 
          result(std::move(res)), 
          timestamp(std::chrono::steady_clock::now()),
          vector_name(std::move(name)) {}
};

// Configurable parameters for Attention cache, might need macros for this?
struct AttentionConfig {
    double recency_factor = 1.0;
    double frequency_factor = 1.0;
    double time_scale = 900.0; // seconds
    
    AttentionConfig(double recency = 1.0, double frequency = 1.0, double scale = 3600.0)
        : recency_factor(recency), frequency_factor(frequency), time_scale(scale) {}
};

// Thread-safe Attention aware cache
class AttentionAwareCache {
public:
    AttentionAwareCache(size_t capacity, AttentionConfig config = AttentionConfig{}) 
        : capacity_{capacity}, config_{config} {
        if (capacity == 0) {
            throw std::invalid_argument("Cache capacity must be greater than 0");
        }
    }

    ~AttentionAwareCache() = default;

    // Delete copy constructor and assignment operator
    AttentionAwareCache(const AttentionAwareCache&) = delete;
    AttentionAwareCache& operator=(const AttentionAwareCache&) = delete;

    // Thread-safe put operation
    void put(const VectorName& vector_name, 
             const std::vector<float>& query_vector, 
             QueryResult result) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if entry already exists
        auto existing_it = find_existing_entry(vector_name, query_vector);
        if (existing_it != cache_entries_.end()) {
            // Update existing entry
            existing_it->second.result = std::move(result);
            existing_it->second.timestamp = std::chrono::steady_clock::now();
            existing_it->second.access_count++;
            return;
        }
        
        // If cache is full, evict least important entry
        if (cache_entries_.size() >= capacity_) {
            evict_by_attention_score();
        }
        
        // Add new entry
        size_t entry_id = generate_id();
        cache_entries_.emplace(entry_id, 
            CacheEntry(query_vector, std::move(result), vector_name));
        
        auto hash_key = compute_vector_hash(query_vector);
        auto key = std::make_tuple(vector_name, hash_key);
        key_mapping_[key] = entry_id;
    }

    // Thread-safe get operation
    bool get(const VectorName& vector_name, 
             const std::vector<float>& query_vector, 
             QueryResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto existing_it = find_existing_entry(vector_name, query_vector);
        if (existing_it == cache_entries_.end()) {
            return false;
        }
        
        // Update access information and return result
        existing_it->second.access_count++;
        existing_it->second.timestamp = std::chrono::steady_clock::now();
        result = existing_it->second.result;
        
        return true;
    }

    // Check if query exists in cache (thread-safe)
    bool contains(const VectorName& vector_name, const std::vector<float>& query_vector) {
        std::lock_guard<std::mutex> lock(mutex_);
        return find_existing_entry(vector_name, query_vector) != cache_entries_.end();
    }

    // Thread-safe size check
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_entries_.size();
    }

    // Get capacity
    size_t capacity() const {
        return capacity_;
    }

    // Thread-safe configuration update
    void set_config(const AttentionConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    // Thread-safe configuration getter
    AttentionConfig get_config() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    // Clear all cache entries (thread-safe)
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_entries_.clear();
        key_mapping_.clear();
    }

    // Clear cache for specific vector name (thread-safe)
    void clear(const VectorName& vector_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto it = cache_entries_.begin(); it != cache_entries_.end(); ) {
            if (it->second.vector_name == vector_name) {
                // Remove from key_mapping_
                for (auto map_it = key_mapping_.begin(); map_it != key_mapping_.end(); ) {
                    if (map_it->second == it->first) {
                        map_it = key_mapping_.erase(map_it);
                        break;
                    } else {
                        ++map_it;
                    }
                }
                it = cache_entries_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Get cache statistics (thread-safe)
    struct CacheStats {
        size_t total_entries;
        size_t total_accesses;
        double avg_attention_score;
        double min_attention_score;
        double max_attention_score;
    };

    CacheStats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        CacheStats stats{};
        
        if (cache_entries_.empty()) {
            return stats;
        }
        
        stats.total_entries = cache_entries_.size();
        stats.min_attention_score = std::numeric_limits<double>::max();
        stats.max_attention_score = std::numeric_limits<double>::lowest();
        
        for (const auto& entry : cache_entries_) {
            stats.total_accesses += entry.second.access_count;
            double score = calculate_attention_score(entry.second);
            stats.avg_attention_score += score;
            
            if (score < stats.min_attention_score) {
                stats.min_attention_score = score;
            }
            if (score > stats.max_attention_score) {
                stats.max_attention_score = score;
            }
        }
        
        stats.avg_attention_score /= cache_entries_.size();
        return stats;
    }

private:
    std::unordered_map<size_t, CacheEntry> cache_entries_;//entry_id -> CacheEntry struct
    std::map<std::tuple<VectorName, size_t>, size_t> key_mapping_;//(vector_name, hash_key)-> entry_id
    size_t capacity_;
    mutable std::mutex mutex_;
    std::atomic<size_t> next_id_{0};
    AttentionConfig config_;

    //thread-safe vector hash computation
    size_t compute_vector_hash(const std::vector<float>& vec) const {
        size_t hash = 0;
        for (float value : vec) {
            hash ^= std::hash<float>{}(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);//this is magic boy
        }
        return hash;
    }

    // Thread-safe ID generation
    size_t generate_id() {
        return next_id_.fetch_add(1, std::memory_order_relaxed);
    }

    // Calculate attention score for an entry
    double calculate_attention_score(const CacheEntry& entry) const {
        auto now = std::chrono::steady_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
            now - entry.timestamp).count();
        
        double recency_weight = config_.recency_factor / (1.0 + time_diff / config_.time_scale);
        double frequency_weight = config_.frequency_factor * std::log(1 + entry.access_count);
        
        return recency_weight * frequency_weight;
    }

    // Find entry with minimum attention score (assumes lock is held)
    typename std::unordered_map<size_t, CacheEntry>::iterator find_min_attention_entry() {
        auto min_it = cache_entries_.begin();
        double min_score = calculate_attention_score(min_it->second);
        
        for (auto it = cache_entries_.begin(); it != cache_entries_.end(); ++it) {
            double score = calculate_attention_score(it->second);
            if (score < min_score) {
                min_score = score;
                min_it = it;
            }
        }
        return min_it;
    }

    // Evict entry with minimum attention score (assumes lock is held)
    void evict_by_attention_score() {
        if (cache_entries_.empty()) return;

        auto min_it = find_min_attention_entry();
        
        // Remove from key_mapping_
        for (auto it = key_mapping_.begin(); it != key_mapping_.end(); ) {
            if (it->second == min_it->first) {
                it = key_mapping_.erase(it);
                break;
            } else {
                ++it;
            }
        }
        
        cache_entries_.erase(min_it);
    }

    // Find existing entry (assumes lock is held)
    typename std::unordered_map<size_t, CacheEntry>::iterator find_existing_entry(
        const VectorName& vector_name, 
        const std::vector<float>& query_vector) {
        
        auto hash_key = compute_vector_hash(query_vector);
        auto key = std::make_tuple(vector_name, hash_key);
        auto mapping_it = key_mapping_.find(key);
        
        if (mapping_it != key_mapping_.end()) {
            return cache_entries_.find(mapping_it->second);
        }
        return cache_entries_.end();
    }
};

} // namespace vectordb

/*
Attention Score Calculation
recency_weight = recency_factor / (1 + Δt / time_scale)
frequency_weight = frequency_factor * log(1 + access_count)
attention_score = recency_weight * frequency_weight

*/
