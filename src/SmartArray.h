/**
 * @brief The goal of impl the SmartArray (predict the dynamic array capacity growth)
 *        is to experiment and learn what can be done to manage memory in 1 dimension.
 *        I am not sure how this will turn out since by the time writing this I have 
 *        zero idea what I am doing; however, I do hope to future explore this idea in
 *        2D memory management with different data structures in the future. We will see.
 * 
 * 
 *        The SmartArray will be predicting array capacity via the S-shape Curve math model.
 *        I will be using an std::vector of pointers pointing to the predicted std::vector chunk.
 * 
 *        Why S-shape Curve?
 *        Ah, so i am kind of annoyed by the fact that whenever we need to resize the array, 
 *        we need to reallocate a new empty array and then copy the elements into the new array.
 *        My ImmutableSegment objs are large, I don't want to copy them so "std::move" them. 
 * 
 *        The S-shape Curve is not really ideal, but i decided to do that because in economics, 
 *        S-curve graphs helps describe, visualize and predict a business' performance progressively over time.
 *        Since the Database is going to be sort of related to business in software, the usage can be described as such.
 *        Economic growth curve ~= computer memory growth curve, well if memory is limited, we don't want to growth too much.       
 */

/**
 * @brief Simplified SmartArray using vector of vectors with growth model prediction
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
    LINEAR,         // Linear growth for predictable memory usage
    LOGARITHMIC     // Logarithmic growth for conservative memory usage
};

/**
 * @brief SmartArray with multiple growth models for capacity prediction
 *        Uses vector of vectors for simpler memory management
 *        Maintains O(log n) access via prefix sums + binary search
 */
template<typename T>
class SmartArray {
public:
    // Constructor-only growth model
    explicit SmartArray(GrowthModel model = GrowthModel::S_CURVE,
                       size_t initial_capacity = 100,
                       size_t max_expected_capacity = 40000,
                       double growth_rate = 0.001)
        : m_total_size{0}, 
          m_max_capacity{max_expected_capacity}, 
          m_growth_model{model}, 
          m_growth_rate{growth_rate},
          m_inflection_point{max_expected_capacity / 2} 
    {
        initializeFirstChunk(initial_capacity);
    }

    //default copy/move operations (vector handles these automatically)
    SmartArray(const SmartArray&) = default;
    SmartArray& operator=(const SmartArray&) = default;
    SmartArray(SmartArray&&) = default;
    SmartArray& operator=(SmartArray&&) = default;

    //standard vector-like interface
    void push_back(const T& value) {
        if (m_chunks.empty() || current_chunk_is_full()) {
            addNewChunk();
        }
        m_chunks.back()->push_back(value);
        m_total_size++;
        updatePrefixSizes();
    }

    void push_back(T&& value) {
        if (m_chunks.empty() || current_chunk_is_full()) {
            addNewChunk();
        }
        m_chunks.back()->push_back(std::move(value));
        m_total_size++;
        updatePrefixSizes();
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (m_chunks.empty() || current_chunk_is_full()) {
            addNewChunk();
        }
        m_chunks.back()->emplace_back(std::forward<Args>(args)...);
        m_total_size++;
        updatePrefixSizes();
    }

    size_t size() const { return m_total_size; }
    bool empty() const { return m_total_size == 0; }

    // Growth model is immutable after construction
    GrowthModel getGrowthModel() const { return m_growth_model; }

    /**
     * @brief O(log n) access via prefix sums + binary search
     */
    const T& operator[](size_t index) const {
        if (m_total_size == 0) 
            throw std::out_of_range("Accessing empty SmartArray");

        if (index >= m_total_size)
            throw std::out_of_range("SmartArray index out of range");

        // Binary search to find the right chunk
        auto it = std::upper_bound(m_prefix_sizes.begin(), m_prefix_sizes.end(), index);
        size_t chunk_idx = std::distance(m_prefix_sizes.begin(), it) - 1;
        size_t offset = index - m_prefix_sizes[chunk_idx];

        return (*m_chunks[chunk_idx])[offset];
    }

    /**
     * @brief Non-const access for modification
     */
    T& operator[](size_t index) {
        if (index >= m_total_size)
            throw std::out_of_range("SmartArray index out of range");

        auto it = std::upper_bound(m_prefix_sizes.begin(), m_prefix_sizes.end(), index);
        size_t chunk_idx = std::distance(m_prefix_sizes.begin(), it) - 1;
        size_t offset = index - m_prefix_sizes[chunk_idx];

        return (*m_chunks[chunk_idx])[offset];
    }

    /**
     * @brief Get element with bounds checking
     */
    const T& at(size_t index) const {
        if (index >= m_total_size)
            throw std::out_of_range("SmartArray index out of range");
        return (*this)[index];
    }

    T& at(size_t index) {
        if (index >= m_total_size)
            throw std::out_of_range("SmartArray index out of range");
        return (*this)[index];
    }

    /**
     * @brief Clear all data but keep growth model settings
     */
    void clear() {
        m_chunks.clear();
        m_prefix_sizes.clear();
        m_total_size = 0;
        initializeFirstChunk(100); // Reset with default initial capacity
    }

    /**
     * @brief Reserve capacity by pre-allocating chunks
     */
    void reserve(size_t new_capacity) {
        size_t reserved = capacity();
        while (reserved < new_capacity) {
            size_t chunk_size = predictNextChunkSize(reserved);
            reserved += chunk_size;
            auto new_chunk = std::make_unique<std::vector<T>>();
            new_chunk->reserve(chunk_size);
            m_chunks.push_back(std::move(new_chunk));
            m_prefix_sizes.push_back(m_total_size); // starting offset stays the same
        }
    }

    /**
     * @brief Total reserved capacity across all chunks
     */
    size_t capacity() const {
        size_t c = 0;
        for (const auto& ch : m_chunks) {
            if (ch) c += ch->capacity();
        }
        return c;
    }

    /**
     * @brief Memory usage statistics
     */
    void getMemoryStats() const {
        size_t total_allocated = 0;
        size_t total_used = 0;

        std::cout << "SmartArray Memory Stats:\n";
        std::cout << "Growth Model: " << growthModelToString(m_growth_model) << "\n";
        std::cout << "Total elements: " << m_total_size << "\n";
        std::cout << "Number of chunks: " << m_chunks.size() << "\n\n";

        for (size_t i = 0; i < m_chunks.size(); ++i) {
            size_t chunk_capacity = m_chunks[i]->capacity();
            size_t chunk_size = m_chunks[i]->size();
            total_allocated += chunk_capacity;
            total_used += chunk_size;

            std::cout << "Chunk " << i 
                      << ": capacity=" << chunk_capacity
                      << ", size=" << chunk_size
                      << ", usage=" << (chunk_size * 100.0 / chunk_capacity) << "%\n";
        }

        std::cout << "\nTotal: allocated=" << total_allocated
                  << ", used=" << total_used
                  << ", efficiency=" << (total_used * 100.0 / total_allocated) << "%\n";
    }

    /**
     * @brief Get chunk information for advanced usage
     */
    size_t chunk_count() const { return m_chunks.size(); }
    
    const std::vector<T>& get_chunk(size_t chunk_index) const {
        if (chunk_index >= m_chunks.size())
            throw std::out_of_range("Chunk index out of range");
        return *m_chunks[chunk_index];
    }

private:
    std::vector<std::unique_ptr<std::vector<T>>> m_chunks;
    std::vector<size_t> m_prefix_sizes; // Cumulative sizes for binary search
    size_t m_total_size;
    size_t m_max_capacity;
    GrowthModel m_growth_model;
    double m_growth_rate;
    size_t m_inflection_point;

    bool current_chunk_is_full() const {
        return !m_chunks.empty() && 
               m_chunks.back()->size() >= m_chunks.back()->capacity();
    }

    void initializeFirstChunk(size_t initial_capacity) {
        size_t first_chunk_size = predictNextChunkSize(0);
        auto first_chunk = std::make_unique<std::vector<T>>();
        first_chunk->reserve(first_chunk_size);
        m_chunks.push_back(std::move(first_chunk));
        m_prefix_sizes.push_back(0);
    }

    size_t predictNextChunkSize(size_t current_total_size) {
        switch (m_growth_model) {
            case GrowthModel::S_CURVE:
                return predictSCurve(current_total_size);
            case GrowthModel::EXPONENTIAL:
                return predictExponential(current_total_size);
            case GrowthModel::LINEAR:
                return predictLinear(current_total_size);
            case GrowthModel::LOGARITHMIC:
                return predictLogarithmic(current_total_size);
            default:
                return predictSCurve(current_total_size);
        }
    }

    //f(x) = 1 / (1 + e^(-k(x - x0)))
    size_t predictSCurve(size_t current_total_size) {
        double exponent = -0.00005 * (current_total_size - 50000.0);
        double s_curve_value = 1.0 / (1.0 + std::exp(exponent));

        double min_chunk_ratio = 0.02;
        double max_chunk_ratio = 0.25;
        double chunk_ratio = min_chunk_ratio + s_curve_value * (max_chunk_ratio - min_chunk_ratio);

        size_t predicted_size = static_cast<size_t>(20000.0 * chunk_ratio); // 20000 = m_max_capacity example
        return std::clamp(predicted_size, size_t(100), size_t(5000));
    }


    size_t predictExponential(size_t current_total_size) {
        double base_growth = 1.5;
        double exponent = m_growth_rate * current_total_size;
        
        size_t base_size = 100;
        size_t predicted_size = static_cast<size_t>(base_size * std::pow(base_growth, exponent));
        
        return std::clamp(predicted_size, size_t(100), size_t(5000));
    }

    size_t predictLinear(size_t current_total_size) {
        double base_size = 100.0;
        double growth_per_chunk = 50.0;
        
        // Use current_total_size to determine growth
        size_t chunk_count_estimate = current_total_size / 100; // rough estimate
        size_t predicted_size = static_cast<size_t>(base_size + (growth_per_chunk * chunk_count_estimate));
        
        return std::clamp(predicted_size, size_t(100), size_t(2000));
    }

    //f(x) = a * log(bx + 1)
    size_t predictLogarithmic(size_t current_total_size) {
        double base_size = 300.0;
        double log_factor = std::log1p(current_total_size);
        
        size_t predicted_size = static_cast<size_t>(base_size + (50.0 * log_factor));
        
        return std::clamp(predicted_size, size_t(100), size_t(1500));
    }

    void addNewChunk() {
        size_t new_chunk_size = predictNextChunkSize(m_total_size);
        auto new_chunk = std::make_unique<std::vector<T>>();
        new_chunk->reserve(new_chunk_size);
        m_chunks.push_back(std::move(new_chunk));
        m_prefix_sizes.push_back(m_total_size);
    }

    void updatePrefixSizes() {
        m_prefix_sizes.clear();
        size_t cumulative = 0;
        for (const auto& chunk : m_chunks) {
            m_prefix_sizes.push_back(cumulative);
            cumulative += chunk->size();
        }
    }

    std::string growthModelToString(GrowthModel model) const {
        switch (model) {
            case GrowthModel::S_CURVE: return "S_CURVE";
            case GrowthModel::EXPONENTIAL: return "EXPONENTIAL";
            case GrowthModel::LINEAR: return "LINEAR";
            case GrowthModel::LOGARITHMIC: return "LOGARITHMIC";
            default: return "UNKNOWN";
        }
    }
};

} // namespace vectordb