/**
 * @brief The goal of impl the SmartArray (predict the dynamic array capacity growth)
 *        is to experiment and learn what can be done to manage memory in 1 dimension.
 *        I am not sure how this will turn out since by the time writing this I have 
 *        zero idea what I am doing; however, I do hope to future explore this idea in
 *        2D memory management with different data structures in the future. We will see.
 * 
 * 
 *        The SmartArray will be predicting array capacity via the S-shape Curve math model.
 *        I will be using linkedlist, where each node of the linkedlist is a predicted size array.
 * 
 *        Why i chose linkedlist and why S-shape Curve?
 *        Ah, so i am kind of annoyed by the fact that whenever we need to resize the array, 
 *        we need to reallocate a new empty array and then copy the elements into the new array.
 *        My ImmutableSegment objs are large, I don't want to copy them or "std::move" them as 
 *        they are meant to be const. We also know linkedlist is good to be used for holding stuff
 *        like data you don't know the sizeof, but the disadvantage is that there will be a lot of 
 *        scattered memory locations, hence i put the array in the nodes for better continuity. 
 * 
 *        The S-shape Curve is not really ideal, but i decided to do that because in economics, 
 *        S-curve graphs helps describe, visualize and predict a business' performance progressively over time.
 *        Since the Database is going to be sort of related to business in software, the usage can be described as such.
 *        Economic growth curve ~= computer memory growth curve, well if memory is limited, we don't want to growth too much.       
 * 
 *        It is good? No. 
 *        But i feel like it is good enough for prototyping my vectordb. 
 *        std::vector is fine, but i want not boring stuff.
 * 
 *        The data structure might look something like this, where each node's array size is predicted via the S-shape Curve.
 * 
 *        [arr{1,2,3,4,5,6}]-->[arr{1,2,3,4,5,6,7,8,9,10}]-->[arr{1,2,3,4,5,6,7,89,0,100,12,12,3,123}]-->[arr{1,2,3,4,5}]
 */
#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace vectordb {

enum class GrowthModel {
    S_CURVE,        // S-shape curve for business-like growth patterns
    EXPONENTIAL,    // Exponential growth for rapid scaling
    FIBONACCI,      // Fibonacci sequence for natural growth patterns
    LINEAR,         // Linear growth for predictable memory usage
    LOGARITHMIC     // Logarithmic growth for conservative memory usage
};

/**
 * @brief Array chunk with predicted capacity using various growth models
 *        Now using std::vector with reserve() for automatic memory management
 */
template<typename T>
class ArrayChunk {
public:
    explicit ArrayChunk(size_t predicted_capacity)
        : m_capacity{predicted_capacity} {
        m_data.reserve(m_capacity); // Pre-allocate fixed capacity
    }

    bool is_full() const { return m_data.size() >= m_capacity; }
    size_t size() const { return m_data.size(); }
    size_t capacity() const { return m_capacity; }
    bool empty() const { return m_data.empty(); }

    void push_back(const T& item) {
        if (is_full()) throw std::runtime_error("ArrayChunk is full");
        m_data.push_back(item);
    }

    void push_back(T&& item) {
        if (is_full()) throw std::runtime_error("ArrayChunk is full");
        m_data.push_back(std::move(item));
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (is_full()) throw std::runtime_error("ArrayChunk is full");
        m_data.emplace_back(std::forward<Args>(args)...);
    }

    const T& operator[](size_t index) const {
        if (index >= m_data.size()) throw std::out_of_range("ArrayChunk index out of range");
        return m_data[index];
    }

    T& operator[](size_t index) {
        if (index >= m_data.size()) throw std::out_of_range("ArrayChunk index out of range");
        return m_data[index];
    }

    // Optional: provide vector-like interface for iteration
    auto begin() const { return m_data.begin(); }
    auto end() const { return m_data.end(); }
    auto begin() { return m_data.begin(); }
    auto end() { return m_data.end(); }

private:
    std::vector<T> m_data;
    size_t m_capacity; // Track fixed capacity separately
};

/**
 * @brief SmartArray with multiple growth models for capacity prediction and
 *        binary search for O(log n) access.
 */
template<typename T>
class SmartArray {
public:
    SmartArray(GrowthModel model = GrowthModel::S_CURVE,
               size_t initial_capacity = 100,
               size_t max_expected_capacity = 40000,
               double growth_rate = 0.001)
        : m_total_size{0},
          m_chunk_count{0},
          m_max_capacity{max_expected_capacity},
          m_growth_rate{growth_rate},
          m_model{model},
          m_inflection_point{max_expected_capacity / 4} {

        size_t first_chunk_size = predictNextChunkSize(0);
        m_head = std::make_unique<ChunkNode>(first_chunk_size);
        m_tail = m_head.get();

        m_prefix_sizes.reserve(64); // Should be enough for prototype
        m_chunk_ptrs.reserve(64);
        m_prefix_sizes.push_back(0);
        m_chunk_ptrs.push_back(m_tail);
    }

    void push_back(const T& value) {
        if (m_tail->chunk.is_full()) addNewChunk();
        m_tail->chunk.push_back(value);
        m_total_size++;
        updatePrefixSizes();
    }

    void push_back(T&& value) {
        if (m_tail->chunk.is_full()) addNewChunk();
        m_tail->chunk.push_back(std::move(value));
        m_total_size++;
        updatePrefixSizes();
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (m_tail->chunk.is_full()) addNewChunk();
        m_tail->chunk.emplace_back(std::forward<Args>(args)...);
        m_total_size++;
        updatePrefixSizes();
    }

    size_t size() const { return m_total_size; }
    bool empty() const { return m_total_size == 0; }

    // Change growth model at runtime
    void setGrowthModel(GrowthModel new_model) {
        m_model = new_model;
    }

    GrowthModel getGrowthModel() const {
        return m_model;
    }

    /**
     * @brief O(log n) access via prefix sums + binary search. Designed for read access only.
     */
    const T& operator[](size_t index) const {
        if (index >= m_total_size)
            throw std::out_of_range("SmartArray index out of range");

        // Locate chunk using binary search
        auto it = std::upper_bound(m_prefix_sizes.begin(), m_prefix_sizes.end(), index);
        size_t chunk_idx = std::distance(m_prefix_sizes.begin(), it) - 1;
        size_t offset = index - m_prefix_sizes[chunk_idx];

        return m_chunk_ptrs[chunk_idx]->chunk[offset];
    }

    // Note: array[10] = obj; is not allowed, enforce immutability
    T& operator[](size_t index) = delete;

    void getMemoryStats() const {
        size_t total_allocated = 0;
        size_t total_used = 0;
        size_t chunk_num = 0;

        ChunkNode* current = m_head.get();
        while (current) {
            total_allocated += current->chunk.capacity();
            total_used += current->chunk.size();
            std::cout << "Chunk " << chunk_num++
                      << ": capacity=" << current->chunk.capacity()
                      << ", size=" << current->chunk.size()
                      << ", usage=" << (current->chunk.size() * 100.0 / current->chunk.capacity()) << "%\n";
            current = current->next.get();
        }

        std::cout << "Total: allocated=" << total_allocated
                  << ", used=" << total_used
                  << ", efficiency=" << (total_used * 100.0 / total_allocated) << "%\n";
        std::cout << "Growth Model: " << growthModelToString(m_model) << std::endl;
    }

private:
    struct ChunkNode {
        ArrayChunk<T> chunk;
        std::unique_ptr<ChunkNode> next;
        explicit ChunkNode(size_t predicted_size) : chunk(predicted_size), next(nullptr) {}
    };

    std::unique_ptr<ChunkNode> m_head;
    ChunkNode* m_tail;
    size_t m_total_size;
    size_t m_chunk_count;
    size_t m_max_capacity;
    double m_growth_rate;
    GrowthModel m_model;
    size_t m_inflection_point;

    std::vector<size_t> m_prefix_sizes; // Prefix cumulative sizes
    std::vector<ChunkNode*> m_chunk_ptrs; // Direct chunk pointers

    size_t predictNextChunkSize(size_t current_total_size) {
        switch (m_model) {
            case GrowthModel::S_CURVE:
                return predictS Curve(current_total_size);
            case GrowthModel::EXPONENTIAL:
                return predictExponential(current_total_size);
            case GrowthModel::FIBONACCI:
                return predictFibonacci(current_total_size);
            case GrowthModel::LINEAR:
                return predictLinear(current_total_size);
            case GrowthModel::LOGARITHMIC:
                return predictLogarithmic(current_total_size);
            default:
                return predictS Curve(current_total_size);
        }
    }

    // Original S-curve prediction
    size_t predictS Curve(size_t current_total_size) {
        double exponent = -m_growth_rate * (current_total_size - m_inflection_point);
        double s_curve_value = 1.0 / (1.0 + std::exp(exponent));

        double min_chunk_ratio = 0.05;
        double max_chunk_ratio = 0.20;
        double chunk_ratio = min_chunk_ratio + s_curve_value * (max_chunk_ratio - min_chunk_ratio);

        size_t predicted_size = static_cast<size_t>(m_max_capacity * chunk_ratio);
        const size_t min_chunk_size = 100;
        const size_t max_chunk_size = 1000;
        return std::clamp(predicted_size, min_chunk_size, max_chunk_size);
    }

    // Exponential growth prediction - aggressive scaling
    size_t predictExponential(size_t current_total_size) {
        double base_growth = 1.5; // 50% growth factor
        double exponent = m_growth_rate * current_total_size;
        
        size_t base_size = 100;
        size_t predicted_size = static_cast<size_t>(base_size * std::pow(base_growth, exponent));
        
        const size_t min_chunk_size = 100;
        const size_t max_chunk_size = 5000; // Higher max for exponential
        return std::clamp(predicted_size, min_chunk_size, max_chunk_size);
    }

    // Fibonacci-based growth - natural progression
    size_t predictFibonacci(size_t current_total_size) {
        static size_t fib_prev = 100;
        static size_t fib_curr = 200;
        
        // Use Fibonacci sequence but scale based on current size
        size_t next_fib = fib_prev + fib_curr;
        fib_prev = fib_curr;
        fib_curr = next_fib;
        
        // Scale Fibonacci numbers to reasonable chunk sizes
        double scale_factor = 1.0 + (static_cast<double>(current_total_size) / m_max_capacity);
        size_t predicted_size = static_cast<size_t>(next_fib * scale_factor / 10.0); // Scale down
        
        const size_t min_chunk_size = 100;
        const size_t max_chunk_size = 3000;
        return std::clamp(predicted_size, min_chunk_size, max_chunk_size);
    }

    // Linear growth - predictable and steady
    size_t predictLinear(size_t current_total_size) {
        double base_size = 200.0;
        double growth_per_chunk = 50.0; // Fixed growth per chunk
        
        size_t predicted_size = static_cast<size_t>(base_size + (growth_per_chunk * m_chunk_count));
        
        const size_t min_chunk_size = 100;
        const size_t max_chunk_size = 2000;
        return std::clamp(predicted_size, min_chunk_size, max_chunk_size);
    }

    // Logarithmic growth - conservative, good for memory-constrained environments
    size_t predictLogarithmic(size_t current_total_size) {
        double base_size = 300.0;
        double log_factor = std::log1p(current_total_size); // log(1 + x)
        
        size_t predicted_size = static_cast<size_t>(base_size + (50.0 * log_factor));
        
        const size_t min_chunk_size = 100;
        const size_t max_chunk_size = 1500;
        return std::clamp(predicted_size, min_chunk_size, max_chunk_size);
    }

    void addNewChunk() {
        size_t new_chunk_size = predictNextChunkSize(m_total_size);
        auto new_chunk = std::make_unique<ChunkNode>(new_chunk_size);
        m_tail->next = std::move(new_chunk);
        m_tail = m_tail->next.get();

        m_chunk_count++;
        m_chunk_ptrs.push_back(m_tail);
        m_prefix_sizes.push_back(m_prefix_sizes.back()); // Initialize with previous value
    }

    void updatePrefixSizes() {
        // Update the last prefix size to reflect current total
        if (!m_prefix_sizes.empty()) {
            m_prefix_sizes.back() = m_total_size;
        }
    }

    std::string growthModelToString(GrowthModel model) const {
        switch (model) {
            case GrowthModel::S_CURVE: return "S_CURVE";
            case GrowthModel::EXPONENTIAL: return "EXPONENTIAL";
            case GrowthModel::FIBONACCI: return "FIBONACCI";
            case GrowthModel::LINEAR: return "LINEAR";
            case GrowthModel::LOGARITHMIC: return "LOGARITHMIC";
            default: return "UNKNOWN";
        }
    }
};

} // namespace vectordb

/*
Usage examples:

// Different growth models for different use cases
vectordb::SmartArray<int> business_data(vectordb::GrowthModel::S_CURVE);
vectordb::SmartArray<int> rapid_growth_data(vectordb::GrowthModel::EXPONENTIAL);
vectordb::SmartArray<int> natural_data(vectordb::GrowthModel::FIBONACCI);
vectordb::SmartArray<int> predictable_data(vectordb::GrowthModel::LINEAR);
vectordb::SmartArray<int> memory_constrained_data(vectordb::GrowthModel::LOGARITHMIC);

// Change model at runtime if needed
business_data.setGrowthModel(vectordb::GrowthModel::EXPONENTIAL);

for (int i = 0; i < 5000; ++i) {
    business_data.push_back(i);
}
business_data.getMemoryStats();
*/

/*
m_prefix_sizes = [0, 100, 230, 400, 640]
                 |    |    |    |    |
 index           0   100  230  400  640

Chunk 0 covers indices [0, 99]
Chunk 1 covers [100, 229]
Chunk 2 covers [230, 399]
Chunk 3 covers [400, 639]

 upper_bound(250) -> points to 400
 position = 3
 chunk_idx = 3 - 1 = 2

 Usage:
 vectordb::SmartArray<int> arr;
for (int i = 0; i < 5000; ++i) arr.push_back(i);
arr.getMemoryStats();
std::cout << "arr[1234] = " << arr[1234] << "\n";


*/