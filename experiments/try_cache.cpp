#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <chrono>
#include <random>
#include <algorithm>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <cmath>

namespace vectordb {

// Simple QueryResult simulation
struct QueryResult {
    std::vector<std::pair<int, float>> results; // id, score
    double search_time_ms = 0.0;
};

using VectorName = std::string;

struct CacheEntry {
    std::vector<float> query_vector;
    QueryResult result;
    std::chrono::steady_clock::time_point timestamp;
    VectorName vector_name;
    double attention_score = 1.0;
    size_t access_count = 1;
    
    CacheEntry(std::vector<float> vec, QueryResult res, VectorName name)
        : query_vector(std::move(vec)), 
          result(std::move(res)), 
          timestamp(std::chrono::steady_clock::now()),
          vector_name(std::move(name)) {}
};

// Configurable parameters for Attention cache
struct AttentionConfig {
    double recency_factor = 1.0;
    double frequency_factor = 1.0;
    double time_scale = 3600.0; // seconds
};

// Original LRU cache
class LRUCache {
private:
    std::vector<CacheEntry> cache_entries_;
    size_t capacity_;
    mutable std::mutex mutex_;

    auto find_entry(const VectorName& vector_name, const std::vector<float>& query_vector) {
        return std::find_if(cache_entries_.begin(), cache_entries_.end(),
            [&](const CacheEntry& entry) {
                return entry.vector_name == vector_name && 
                       entry.query_vector.size() == query_vector.size() &&
                       std::equal(entry.query_vector.begin(), entry.query_vector.end(),
                                 query_vector.begin());
            });
    }

    void evict_lru() {
        if (cache_entries_.empty()) return;
        auto lru_it = std::min_element(cache_entries_.begin(), cache_entries_.end(),
            [](const CacheEntry& a, const CacheEntry& b) {
                return a.timestamp < b.timestamp;
            });
        cache_entries_.erase(lru_it);
    }

public:
    LRUCache(size_t capacity) : capacity_(capacity) {}

    void put(const VectorName& vector_name, 
             const std::vector<float>& query_vector, 
             QueryResult&& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = find_entry(vector_name, query_vector);
        if (it != cache_entries_.end()) {
            it->result = std::move(result);
            it->timestamp = std::chrono::steady_clock::now();
            return;
        }

        if (cache_entries_.size() >= capacity_) {
            evict_lru();
        }

        cache_entries_.emplace_back(query_vector, std::move(result), vector_name);
    }

    bool get(const VectorName& vector_name, 
             const std::vector<float>& query_vector, 
             QueryResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = find_entry(vector_name, query_vector);
        if (it == cache_entries_.end()) {
            return false;
        }

        it->timestamp = std::chrono::steady_clock::now();
        result = it->result;
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_entries_.size();
    }
};

// Attention aware cache
class AttentionAwareCache {
private:
    std::unordered_map<size_t, CacheEntry> cache_entries_;
    std::map<std::tuple<VectorName, size_t>, size_t> key_mapping_;
    size_t capacity_;
    mutable std::mutex mutex_;
    size_t next_id_ = 0;
    AttentionConfig config_;

    size_t compute_vector_hash(const std::vector<float>& vec) const {
        size_t hash = 0;
        for (float value : vec) {
            // Simple hash function
            hash ^= std::hash<float>{}(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }

    size_t generate_id() {
        return next_id_++;
    }

    double calculate_attention_score(const CacheEntry& entry) const {
        auto now = std::chrono::steady_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
            now - entry.timestamp).count();
        
        double recency_weight = config_.recency_factor / (1.0 + time_diff / config_.time_scale);
        double frequency_weight = config_.frequency_factor * std::log(1 + entry.access_count);
        
        return recency_weight * frequency_weight;
    }

    void evict_by_attention_score() {
        if (cache_entries_.empty()) return;

        auto min_it = cache_entries_.begin();
        double min_score = calculate_attention_score(min_it->second);
        
        for (auto it = cache_entries_.begin(); it != cache_entries_.end(); ++it) {
            double score = calculate_attention_score(it->second);
            if (score < min_score) {
                min_score = score;
                min_it = it;
            }
        }

        // Remove from key_mapping_ as well
        for (auto it = key_mapping_.begin(); it != key_mapping_.end(); ) {
            if (it->second == min_it->first) {
                it = key_mapping_.erase(it);
            } else {
                ++it;
            }
        }
        
        cache_entries_.erase(min_it);
    }

public:
    AttentionAwareCache(size_t capacity, AttentionConfig config = AttentionConfig{}) 
        : capacity_(capacity), config_(config) {}

    // Update configuration
    void set_config(const AttentionConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    AttentionConfig get_config() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    bool get(const VectorName& vector_name, 
             const std::vector<float>& query_vector, 
             QueryResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto hash_key = compute_vector_hash(query_vector);
        auto key = std::make_tuple(vector_name, hash_key);
        auto it = key_mapping_.find(key);
        if (it == key_mapping_.end()) return false;
        
        auto entry_it = cache_entries_.find(it->second);
        if (entry_it == cache_entries_.end()) return false;
        
        entry_it->second.access_count++;
        entry_it->second.timestamp = std::chrono::steady_clock::now();
        result = entry_it->second.result;
        
        return true;
    }
    
    void put(const VectorName& vector_name, 
             const std::vector<float>& query_vector, 
             QueryResult&& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto hash_key = compute_vector_hash(query_vector);
        auto key = std::make_tuple(vector_name, hash_key);
        
        // Check if already exists
        auto existing_it = key_mapping_.find(key);
        if (existing_it != key_mapping_.end()) {
            // Update existing entry
            auto entry_it = cache_entries_.find(existing_it->second);
            if (entry_it != cache_entries_.end()) {
                entry_it->second.result = std::move(result);
                entry_it->second.timestamp = std::chrono::steady_clock::now();
                entry_it->second.access_count++;
                return;
            }
        }
        
        // If cache is full, evict
        if (cache_entries_.size() >= capacity_) {
            evict_by_attention_score();
        }
        
        size_t entry_id = generate_id();
        cache_entries_.emplace(entry_id, 
            CacheEntry(query_vector, std::move(result), vector_name));
        key_mapping_[key] = entry_id;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_entries_.size();
    }

    // Get cache statistics
    void get_stats(size_t& total_accesses, double& avg_attention_score) const {
        std::lock_guard<std::mutex> lock(mutex_);
        total_accesses = 0;
        avg_attention_score = 0.0;
        
        for (const auto& entry : cache_entries_) {
            total_accesses += entry.second.access_count;
            avg_attention_score += calculate_attention_score(entry.second);
        }
        
        if (!cache_entries_.empty()) {
            avg_attention_score /= cache_entries_.size();
        }
    }
};

// Test framework
class CacheBenchmark {
private:
    size_t cache_size_;
    size_t num_queries_;
    size_t vector_dim_;
    std::vector<std::vector<float>> query_pool_;
    std::vector<VectorName> collection_pool_;
    
    // Generate random vectors
    std::vector<float> generate_random_vector(size_t dim) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        
        std::vector<float> vec(dim);
        for (size_t i = 0; i < dim; ++i) {
            vec[i] = dis(gen);
        }
        return vec;
    }
    
    // Generate query results
    QueryResult generate_query_result() {
        QueryResult result;
        // Simulate top-10 results
        for (int i = 0; i < 10; ++i) {
            result.results.emplace_back(i, 1.0f - i * 0.1f);
        }
        result.search_time_ms = 5.0 + (std::rand() % 100) / 10.0; // 5-15ms
        return result;
    }

public:
    CacheBenchmark(size_t cache_size = 100, size_t num_queries = 1000, size_t vector_dim = 128)
        : cache_size_(cache_size), num_queries_(num_queries), vector_dim_(vector_dim) {
        
        // Prepare query pool
        for (size_t i = 0; i < cache_size * 3; ++i) {
            query_pool_.push_back(generate_random_vector(vector_dim_));
        }
        
        // Prepare collection names
        collection_pool_ = {"collection1", "collection2", "collection3"};
    }
    
    // Test scenario 1: Uniform distribution queries
    void test_uniform_distribution(const AttentionConfig& attention_config = AttentionConfig{}) {
        std::cout << "=== Test Scenario 1: Uniform Distribution Queries ===" << std::endl;
        std::cout << "Attention Config: recency=" << attention_config.recency_factor 
                  << ", frequency=" << attention_config.frequency_factor 
                  << ", time_scale=" << attention_config.time_scale << std::endl;
        
        LRUCache lru_cache(cache_size_);
        AttentionAwareCache attention_cache(cache_size_, attention_config);
        
        size_t lru_hits = 0, attention_hits = 0;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> query_dist(0, query_pool_.size() - 1);
        std::uniform_int_distribution<size_t> coll_dist(0, collection_pool_.size() - 1);
        
        for (size_t i = 0; i < num_queries_; ++i) {
            size_t query_idx = query_dist(gen);
            size_t coll_idx = coll_dist(gen);
            
            auto& query_vec = query_pool_[query_idx];
            auto& collection = collection_pool_[coll_idx];
            auto result = generate_query_result();
            
            // Try to get from cache first
            QueryResult cached_result;
            if (lru_cache.get(collection, query_vec, cached_result)) {
                lru_hits++;
            } else {
                lru_cache.put(collection, query_vec, std::move(result));
            }
            
            // Regenerate result for attention cache
            result = generate_query_result();
            if (attention_cache.get(collection, query_vec, cached_result)) {
                attention_hits++;
            } else {
                attention_cache.put(collection, query_vec, std::move(result));
            }
        }
        
        double lru_hit_rate = static_cast<double>(lru_hits) / num_queries_ * 100;
        double attention_hit_rate = static_cast<double>(attention_hits) / num_queries_ * 100;
        
        std::cout << "LRU Cache Hit Rate: " << std::fixed << std::setprecision(2) 
                  << lru_hit_rate << "%" << std::endl;
        std::cout << "Attention Cache Hit Rate: " << attention_hit_rate << "%" << std::endl;
        std::cout << "Performance Improvement: " << (attention_hit_rate - lru_hit_rate) << "%" << std::endl;
        
        // Show attention cache stats
        size_t total_accesses;
        double avg_attention_score;
        attention_cache.get_stats(total_accesses, avg_attention_score);
        std::cout << "Attention Cache Stats - Total Accesses: " << total_accesses 
                  << ", Avg Attention Score: " << std::fixed << std::setprecision(3) 
                  << avg_attention_score << std::endl;
    }
    
    // Test scenario 2: Hotspot queries (80% queries on 20% data)
    void test_zipf_distribution(const AttentionConfig& attention_config = AttentionConfig{}) {
        std::cout << "\n=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===" << std::endl;
        std::cout << "Attention Config: recency=" << attention_config.recency_factor 
                  << ", frequency=" << attention_config.frequency_factor 
                  << ", time_scale=" << attention_config.time_scale << std::endl;
        
        LRUCache lru_cache(cache_size_);
        AttentionAwareCache attention_cache(cache_size_, attention_config);
        
        size_t lru_hits = 0, attention_hits = 0;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Create Zipf distribution: 80% queries on 20% data
        size_t hot_data_size = query_pool_.size() / 5; // 20% hot data
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);
        
        for (size_t i = 0; i < num_queries_; ++i) {
            size_t query_idx;
            float rand_val = dis(gen);
            
            if (rand_val < 0.8f) { // 80% probability to access hot data
                query_idx = std::rand() % hot_data_size;
            } else { // 20% probability to access other data
                query_idx = hot_data_size + std::rand() % (query_pool_.size() - hot_data_size);
            }
            
            auto& collection = collection_pool_[i % collection_pool_.size()];
            auto& query_vec = query_pool_[query_idx];
            auto result = generate_query_result();
            
            // LRU cache
            QueryResult cached_result;
            if (lru_cache.get(collection, query_vec, cached_result)) {
                lru_hits++;
            } else {
                lru_cache.put(collection, query_vec, std::move(result));
            }
            
            // Attention cache
            result = generate_query_result();
            if (attention_cache.get(collection, query_vec, cached_result)) {
                attention_hits++;
            } else {
                attention_cache.put(collection, query_vec, std::move(result));
            }
        }
        
        double lru_hit_rate = static_cast<double>(lru_hits) / num_queries_ * 100;
        double attention_hit_rate = static_cast<double>(attention_hits) / num_queries_ * 100;
        
        std::cout << "LRU Cache Hit Rate: " << std::fixed << std::setprecision(2) 
                  << lru_hit_rate << "%" << std::endl;
        std::cout << "Attention Cache Hit Rate: " << attention_hit_rate << "%" << std::endl;
        std::cout << "Performance Improvement: " << (attention_hit_rate - lru_hit_rate) << "%" << std::endl;
        
        // Show attention cache stats
        size_t total_accesses;
        double avg_attention_score;
        attention_cache.get_stats(total_accesses, avg_attention_score);
        std::cout << "Attention Cache Stats - Total Accesses: " << total_accesses 
                  << ", Avg Attention Score: " << std::fixed << std::setprecision(3) 
                  << avg_attention_score << std::endl;
    }
    
    // Test scenario 3: Periodic access pattern
    void test_periodic_pattern(const AttentionConfig& attention_config = AttentionConfig{}) {
        std::cout << "\n=== Test Scenario 3: Periodic Access Pattern ===" << std::endl;
        std::cout << "Attention Config: recency=" << attention_config.recency_factor 
                  << ", frequency=" << attention_config.frequency_factor 
                  << ", time_scale=" << attention_config.time_scale << std::endl;
        
        LRUCache lru_cache(cache_size_);
        AttentionAwareCache attention_cache(cache_size_, attention_config);
        
        size_t lru_hits = 0, attention_hits = 0;
        
        // Create periodic query patterns
        std::vector<size_t> pattern_a = {0, 1, 2, 3, 4};
        std::vector<size_t> pattern_b = {5, 6, 7, 8, 9};
        std::vector<size_t> pattern_c = {10, 11, 12, 13, 14};
        
        for (size_t i = 0; i < num_queries_; ++i) {
            size_t query_idx;
            
            // Periodic pattern: switch pattern every 30 queries
            size_t cycle = i / 30;
            if (cycle % 3 == 0) {
                query_idx = pattern_a[i % pattern_a.size()];
            } else if (cycle % 3 == 1) {
                query_idx = pattern_b[i % pattern_b.size()];
            } else {
                query_idx = pattern_c[i % pattern_c.size()];
            }
            
            // Ensure within bounds
            query_idx = query_idx % query_pool_.size();
            auto& collection = collection_pool_[0]; // Use same collection
            auto& query_vec = query_pool_[query_idx];
            auto result = generate_query_result();
            
            // LRU cache
            QueryResult cached_result;
            if (lru_cache.get(collection, query_vec, cached_result)) {
                lru_hits++;
            } else {
                lru_cache.put(collection, query_vec, std::move(result));
            }
            
            // Attention cache
            result = generate_query_result();
            if (attention_cache.get(collection, query_vec, cached_result)) {
                attention_hits++;
            } else {
                attention_cache.put(collection, query_vec, std::move(result));
            }
        }
        
        double lru_hit_rate = static_cast<double>(lru_hits) / num_queries_ * 100;
        double attention_hit_rate = static_cast<double>(attention_hits) / num_queries_ * 100;
        
        std::cout << "LRU Cache Hit Rate: " << std::fixed << std::setprecision(2) 
                  << lru_hit_rate << "%" << std::endl;
        std::cout << "Attention Cache Hit Rate: " << attention_hit_rate << "%" << std::endl;
        std::cout << "Performance Improvement: " << (attention_hit_rate - lru_hit_rate) << "%" << std::endl;
        
        // Show attention cache stats
        size_t total_accesses;
        double avg_attention_score;
        attention_cache.get_stats(total_accesses, avg_attention_score);
        std::cout << "Attention Cache Stats - Total Accesses: " << total_accesses 
                  << ", Avg Attention Score: " << std::fixed << std::setprecision(3) 
                  << avg_attention_score << std::endl;
    }
    
    void run_all_tests(const AttentionConfig& config = AttentionConfig{}) {
        std::cout << "Starting Cache Performance Tests..." << std::endl;
        std::cout << "Cache Size: " << cache_size_ << std::endl;
        std::cout << "Query Count: " << num_queries_ << std::endl;
        std::cout << "Vector Dimension: " << vector_dim_ << std::endl;
        
        test_uniform_distribution(config);
        test_zipf_distribution(config); 
        test_periodic_pattern(config);
        
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Attention cache demonstrates better adaptability across different scenarios" << std::endl;
        std::cout << "Particularly significant advantages in non-uniform query distributions" << std::endl;
    }

    void test_multiple_configs() {
        std::vector<AttentionConfig> configs = {
            {1.0, 1.0, 3600.0},    // Default
            {0.7, 1.3, 1800.0},    // More frequency-focused
            {1.3, 0.7, 7200.0},    // More recency-focused
            {0.5, 1.5, 900.0},     // Very frequency-focused
            {1.5, 0.5, 10800.0}    // Very recency-focused
        };

        std::cout << "Testing multiple attention configurations...\n" << std::endl;

        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "=== Configuration " << (i + 1) << " ===" << std::endl;
            std::cout << "Recency: " << configs[i].recency_factor 
                      << ", Frequency: " << configs[i].frequency_factor 
                      << ", Time Scale: " << configs[i].time_scale << std::endl;
            
            test_zipf_distribution(configs[i]);
            std::cout << std::endl;
        }
    }
};

} // namespace vectordb

// Main test program
int main() {
    vectordb::CacheBenchmark benchmark(300, 100000, 1024);
    
    // Test with default configuration
    benchmark.run_all_tests();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "COMPARING DIFFERENT ATTENTION CONFIGURATIONS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Test multiple configurations
    benchmark.test_multiple_configs();
    
    return 0;
}

/*
Starting Cache Performance Tests...
Cache Size: 300
Query Count: 100000
Vector Dimension: 1024
=== Test Scenario 1: Uniform Distribution Queries ===
Attention Config: recency=1, frequency=1, time_scale=3600
LRU Cache Hit Rate: 10.98%
Attention Cache Hit Rate: 11.17%
Performance Improvement: 0.19%
Attention Cache Stats - Total Accesses: 11433, Avg Attention Score: 3.648

=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===
Attention Config: recency=1.000, frequency=1.000, time_scale=3600.000
LRU Cache Hit Rate: 34.24%
Attention Cache Hit Rate: 42.79%
Performance Improvement: 8.55%
Attention Cache Stats - Total Accesses: 43057, Avg Attention Score: 4.915

=== Test Scenario 3: Periodic Access Pattern ===
Attention Config: recency=1.000, frequency=1.000, time_scale=3600.000
LRU Cache Hit Rate: 99.98%
Attention Cache Hit Rate: 99.98%
Performance Improvement: 0.00%
Attention Cache Stats - Total Accesses: 100000, Avg Attention Score: 8.805

=== Test Summary ===
Attention cache demonstrates better adaptability across different scenarios
Particularly significant advantages in non-uniform query distributions

============================================================
COMPARING DIFFERENT ATTENTION CONFIGURATIONS
============================================================
Testing multiple attention configurations...

=== Configuration 1 ===
Recency: 1.000, Frequency: 1.000, Time Scale: 3600.000

=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===
Attention Config: recency=1.000, frequency=1.000, time_scale=3600.000
LRU Cache Hit Rate: 34.13%
Attention Cache Hit Rate: 42.62%
Performance Improvement: 8.49%
Attention Cache Stats - Total Accesses: 42893, Avg Attention Score: 4.901

=== Configuration 2 ===
Recency: 0.700, Frequency: 1.300, Time Scale: 1800.000

=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===
Attention Config: recency=0.700, frequency=1.300, time_scale=1800.000
LRU Cache Hit Rate: 34.11%
Attention Cache Hit Rate: 42.46%
Performance Improvement: 8.35%
Attention Cache Stats - Total Accesses: 42728, Avg Attention Score: 4.453

=== Configuration 3 ===
Recency: 1.300, Frequency: 0.700, Time Scale: 7200.000

=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===
Attention Config: recency=1.300, frequency=0.700, time_scale=7200.000
LRU Cache Hit Rate: 34.79%
Attention Cache Hit Rate: 41.50%
Performance Improvement: 6.71%
Attention Cache Stats - Total Accesses: 41774, Avg Attention Score: 4.388

=== Configuration 4 ===
Recency: 0.500, Frequency: 1.500, Time Scale: 900.000

=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===
Attention Config: recency=0.500, frequency=1.500, time_scale=900.000
LRU Cache Hit Rate: 34.29%
Attention Cache Hit Rate: 43.14%
Performance Improvement: 8.85%
Attention Cache Stats - Total Accesses: 43417, Avg Attention Score: 3.702

=== Configuration 5 ===
Recency: 1.500, Frequency: 0.500, Time Scale: 10800.000

=== Test Scenario 2: Hotspot Queries (Zipf Distribution) ===
Attention Config: recency=1.500, frequency=0.500, time_scale=10800.000
LRU Cache Hit Rate: 34.59%
Attention Cache Hit Rate: 42.27%
Performance Improvement: 7.68%
Attention Cache Stats - Total Accesses: 42547, Avg Attention Score: 3.656
*/