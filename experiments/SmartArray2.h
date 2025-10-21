#pragma once
#include <vector>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <deque>
#include <chrono>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

namespace vectordb {

// ===================== Linear Predictor with persistence =====================
struct LinearPredictor {
    double w0 = 200.0;
    double w1 = 30.0;
    double w2 = 100.0;
    double w3 = -200.0;   // memory-pressure weight: negative to shrink when pressure high
    double lr = 1e-4;     // learning rate

    // Predict next chunk size
    double predict(double x1, double x2, double x3) const {
        return w0 + w1 * x1 + w2 * x2 + w3 * x3;
    }

    // SGD update
    void update(double x1, double x2, double x3, double target) {
        double pred = predict(x1, x2, x3);
        double error = target - pred;
        w0 += lr * error;
        w1 += lr * error * x1;
        w2 += lr * error * x2;
        w3 += lr * error * x3;
    }

    // Persist/load simple space-separated weights file
    bool load(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return false;
        in >> w0 >> w1 >> w2 >> w3 >> lr;
        return in.good();
    }
    bool save(const std::string& path) const {
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) return false;
        out << std::setprecision(17)
            << w0 << " " << w1 << " " << w2 << " " << w3 << " " << lr << "\n";
        return out.good();
    }
};

// ===================== SmartArray (template) =====================
template<typename T>
class SmartArray {
public:
    explicit SmartArray(size_t initial_capacity = 200,
                        size_t min_chunk = 100,
                        size_t max_chunk = 5000,
                        bool enable_feedback = true,
                        const std::string& persist_path = ".smartarray_weights",
                        const std::string& log_path = "smartarray_log.csv")
        : m_total_size{0},
          m_min_chunk{min_chunk},
          m_max_chunk{max_chunk},
          m_feedback_enabled{enable_feedback},
          m_weights_path{persist_path},
          m_log_path{log_path}
    {
        // try load predictor weights if exists
        m_predictor.load(m_weights_path);
        initializeFirstChunk(initial_capacity);

        // open CSV log (append)
        m_log_stream.open(m_log_path, std::ios::app);
        if (m_log_stream.tellp() == 0) {
            // write header if file empty/new
            m_log_stream << "timestamp,chunk_index,chunk_capacity,fill_time_s,avg_fill_ratio,mem_pressure\n";
            m_log_stream.flush();
        }
    }

    ~SmartArray() {
        // persist weights on destruction
        m_predictor.save(m_weights_path);
        if (m_log_stream.is_open()) m_log_stream.close();
    }

    // disable copying for simplicity
    SmartArray(const SmartArray&) = delete;
    SmartArray& operator=(const SmartArray&) = delete;

    void push_back(const T& value) {
        if (m_chunks.empty() || current_chunk_is_full()) {
            addNewChunk();
        }
        m_chunks.back()->push_back(value);
        m_total_size++;
        if (m_chunks.back()->size() >= m_chunks.back()->capacity()) {
            onChunkFilled();
        }
        // update last prefix start (fast)
        if (!m_prefix_sizes.empty()) {
            m_prefix_sizes.back() = m_total_size - m_chunks.back()->size();
        }
    }

    void push_back(T&& value) {
        if (m_chunks.empty() || current_chunk_is_full()) {
            addNewChunk();
        }
        m_chunks.back()->push_back(std::move(value));
        m_total_size++;
        if (m_chunks.back()->size() >= m_chunks.back()->capacity()) {
            onChunkFilled();
        }
        if (!m_prefix_sizes.empty()) {
            m_prefix_sizes.back() = m_total_size - m_chunks.back()->size();
        }
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (m_chunks.empty() || current_chunk_is_full()) {
            addNewChunk();
        }
        m_chunks.back()->emplace_back(std::forward<Args>(args)...);
        m_total_size++;
        if (m_chunks.back()->size() >= m_chunks.back()->capacity()) {
            onChunkFilled();
        }
        if (!m_prefix_sizes.empty()) {
            m_prefix_sizes.back() = m_total_size - m_chunks.back()->size();
        }
    }

    size_t size() const { return m_total_size; }
    bool empty() const { return m_total_size == 0; }

    // index access (O(log chunks))
    const T& operator[](size_t index) const {
        if (m_total_size == 0) throw std::out_of_range("Accessing empty SmartArray");
        if (index >= m_total_size) throw std::out_of_range("SmartArray index out of range");
        auto it = std::upper_bound(m_prefix_sizes.begin(), m_prefix_sizes.end(), index);
        size_t chunk_idx = std::distance(m_prefix_sizes.begin(), it) - 1;
        size_t offset = index - m_prefix_sizes[chunk_idx];
        return (*m_chunks[chunk_idx])[offset];
    }

    T& operator[](size_t index) {
        if (index >= m_total_size) throw std::out_of_range("SmartArray index out of range");
        auto it = std::upper_bound(m_prefix_sizes.begin(), m_prefix_sizes.end(), index);
        size_t chunk_idx = std::distance(m_prefix_sizes.begin(), it) - 1;
        size_t offset = index - m_prefix_sizes[chunk_idx];
        return (*m_chunks[chunk_idx])[offset];
    }

    void clear() {
        m_chunks.clear();
        m_prefix_sizes.clear();
        m_total_size = 0;
        m_recent_fill_rates.clear();
        initializeFirstChunk(m_min_chunk);
    }

    size_t chunk_count() const { return m_chunks.size(); }

    void getMemoryStats() const {
        size_t total_allocated = 0;
        size_t total_used = 0;
        std::cout << "SmartArray Memory Stats:\n";
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

    // Save predictor weights manually at any time
    bool save_weights() const { return m_predictor.save(m_weights_path); }
    bool load_weights() { return m_predictor.load(m_weights_path); }

private:
    // internal storage
    std::vector<std::unique_ptr<std::vector<T>>> m_chunks;
    std::vector<size_t> m_prefix_sizes; // starting index of each chunk
    size_t m_total_size;

    // config
    size_t m_min_chunk;
    size_t m_max_chunk;
    bool m_feedback_enabled;

    // predictor + persistence
    LinearPredictor m_predictor;
    std::string m_weights_path;

    // fill-rate history
    std::deque<double> m_recent_fill_rates; // elements/sec
    size_t m_history_k = 8;
    std::chrono::steady_clock::time_point m_current_chunk_start_time;
    size_t m_last_chunk_capacity;

    // logging
    std::string m_log_path;
    std::ofstream m_log_stream;

    bool current_chunk_is_full() const {
        return !m_chunks.empty() && m_chunks.back()->size() >= m_chunks.back()->capacity();
    }

    void initializeFirstChunk(size_t initial_capacity) {
        size_t first_chunk_size = std::clamp(initial_capacity, m_min_chunk, m_max_chunk);
        auto first_chunk = std::make_unique<std::vector<T>>();
        first_chunk->reserve(first_chunk_size);
        m_chunks.push_back(std::move(first_chunk));
        m_prefix_sizes.push_back(0);
        m_total_size = 0;
        m_current_chunk_start_time = std::chrono::steady_clock::now();
        m_last_chunk_capacity = first_chunk_size;
    }

    // compute mem_pressure (0..1). Returns 0 if cannot read.
    double compute_mem_pressure() const {
#ifdef __linux__
        std::ifstream f("/proc/meminfo");
        if (!f.is_open()) return 0.0;
        std::string line;
        long memAvailableKb = -1;
        long memTotalKb = -1;
        while (std::getline(f, line)) {
            if (line.rfind("MemAvailable:", 0) == 0) {
                std::sscanf(line.c_str(), "MemAvailable: %ld kB", &memAvailableKb);
            } else if (line.rfind("MemTotal:", 0) == 0) {
                std::sscanf(line.c_str(), "MemTotal: %ld kB", &memTotalKb);
            }
            if (memAvailableKb >= 0 && memTotalKb >= 0) break;
        }
        if (memAvailableKb < 0 || memTotalKb <= 0) return 0.0;
        double avail = static_cast<double>(memAvailableKb);
        double total = static_cast<double>(memTotalKb);
        double pressure = 1.0 - (avail / total); // 0 = plenty, 1 = exhausted
        return std::clamp(pressure, 0.0, 1.0);
#else
        return 0.0;
#endif
    }

    double mean_recent_fill_rate() const {
        if (m_recent_fill_rates.empty()) return 0.0;
        double s = 0.0;
        for (double v : m_recent_fill_rates) s += v;
        return s / static_cast<double>(m_recent_fill_rates.size());
    }

    // Predict next chunk size (learning predictor)
    size_t predictNextChunkSize() {
        // features
        double x1 = std::log1p(static_cast<double>(m_total_size));
        double x2 = 0.0;
        // x2 = average utilization across chunks (0..1)
        if (!m_chunks.empty()) {
            double accum = 0.0;
            for (const auto& cptr : m_chunks) {
                double ccap = static_cast<double>(cptr->capacity());
                double csize = static_cast<double>(cptr->size());
                if (ccap > 0.0) accum += (csize / ccap);
            }
            x2 = accum / static_cast<double>(m_chunks.size());
        }
        double x3 = compute_mem_pressure();

        double predicted = m_predictor.predict(x1, x2, x3);
        size_t proposed = static_cast<size_t>(std::llround(predicted));
        proposed = std::clamp(proposed, m_min_chunk, m_max_chunk);
        return proposed;
    }

    void addNewChunk() {
        size_t new_chunk_size = predictNextChunkSize();
        if (new_chunk_size == 0) new_chunk_size = m_min_chunk;
        auto new_chunk = std::make_unique<std::vector<T>>();
        new_chunk->reserve(new_chunk_size);
        m_chunks.push_back(std::move(new_chunk));
        m_prefix_sizes.push_back(m_total_size);
        m_current_chunk_start_time = std::chrono::steady_clock::now();
        m_last_chunk_capacity = new_chunk_size;
    }

    void onChunkFilled() {
        // compute duration of filling last chunk
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> dur = now - m_current_chunk_start_time;
        double seconds = std::max(dur.count(), 1e-9);
        double elements = static_cast<double>(m_last_chunk_capacity);
        double fill_rate = elements / seconds;
        m_recent_fill_rates.push_back(fill_rate);
        if (m_recent_fill_rates.size() > m_history_k) m_recent_fill_rates.pop_front();

        // prepare features as used for the prediction that allocated this chunk
        double x1 = std::log1p(static_cast<double>(m_total_size));
        double x2 = mean_recent_fill_rate();
        double x3 = compute_mem_pressure();

        // target = actual last chunk capacity
        double target = static_cast<double>(m_last_chunk_capacity);

        // update predictor weights (online)
        if (m_feedback_enabled) {
            m_predictor.update(x1, x2, x3, target);
        }

        // log the event
        if (m_log_stream.is_open()) {
            std::ostringstream ss;
            const auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()
                            ).count();
            size_t chunk_idx = (m_chunks.size() >= 1) ? (m_chunks.size() - 1) : 0;
            ss << ts << "," << chunk_idx << "," << m_last_chunk_capacity << ","
               << seconds << "," << x2 << "," << x3 << "\n";
            m_log_stream << ss.str();
            m_log_stream.flush();
        }
    }
};

} // namespace vectordb
