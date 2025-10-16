#pragma once

#include "DataTypes.h"
#include "ActiveSegment.h"
#include "ImmutableSegment.h"

#include <future>
/**
 * @brief 
 * Low-level container: stores and manages access to segments
 * Doesn’t make decisions.
 *      
 * Scope: Manages segments inside a single collection.
 * Purpose: Acts as a manager or container of segments belonging to one collection.
 * 
 * It will hold 1 ActiveSegment and multiple ImmutableSgment(s) per Collection obj.     
 * 
 */
namespace vectordb {

class SegmentHolder {
public:
    SegmentHolder(size_t max_active_capacity, const CollectionInfo& info)
        : m_collection_info{info},
          m_active_segment{max_active_capacity, info} {/*constructor body*/}
    
    ~SegmentHolder() = default;

    Status insertPoint(PointIdType point_id, const DenseVector& vector) {
        auto status = m_active_segment.insertPoint(point_id, vector);
        if (status.ok) {
            // Try to convert if needed
            auto convert_status = convertActiveToImmutable();
            if (!convert_status.ok) {
                return convert_status;
            }
        }
        return status;
    }
    
    Status insertPoint(PointIdType point_id, 
                      const std::map<VectorName, DenseVector>& named_vectors) {
        auto status = m_active_segment.insertPoint(point_id, named_vectors);
        if (status.ok) {
            auto convert_status = convertActiveToImmutable();
            if (!convert_status.ok) {
                return convert_status;
            }
        }
        return status;
    }

    // Convert active to immutable when ready
    Status convertActiveToImmutable() {
        if (!m_active_segment.shouldIndex() && !m_active_segment.isFull()) {
            // std::cout << "Hello From Segmentholder, converActiveToImmutable\n";
            return Status::OK();
        }
        
        auto immutable_segment = m_active_segment.convertToImmutable();
        if (!immutable_segment.ok()) {
            return immutable_segment.status();
        }
        
        //the value() which is from StatusOr, could later add ValueOrDie() method in.
        if (m_collection_info.on_disk == false) {
            //fix: use a smartVector to manage this
            m_immutable_segments.push_back(std::move(immutable_segment.value()));
        } else {
            //add to disk stuff here..
            //do add some cache array here to hold some in memory immutable segments?
            //like immutable_segment.writeToDisk?
        }

        return Status::OK();
    }

    const ActiveSegment& getActiveSegment() const { 
        return m_active_segment; 
    }

    const std::vector<std::unique_ptr<ImmutableSegment>>& getImmutableSegments() const {
        return m_immutable_segments;
    }

    size_t getTotalPointCount() const {
        size_t count = m_active_segment.getPointCount();
        for (const auto& seg : m_immutable_segments) {
            count += seg->getPointCount();
        }
        return count;
    }

    QueryResult searchTopK(
        const std::string& vector_name,
        const std::vector<DenseVector>& query_vectors,
        size_t k) const 
    {
        // auto start_time = std::chrono::high_resolution_clock::now();

        // Collect all results
        std::vector<QueryResult> all_results;

        // Search active segment first
        all_results.push_back(
            m_active_segment.searchTopK(vector_name, query_vectors, k)
        );

        // Multi-threaded search for immutable segments
        const size_t num_threads = std::max(1u, std::thread::hardware_concurrency() / 2);
        std::atomic<size_t> next_index{0};
        std::vector<std::future<std::vector<QueryResult>>> futures;

        // Worker: searches one or more immutable segments
        auto worker = [&]() -> std::vector<QueryResult> {
            std::vector<QueryResult> local_results;
            size_t idx;
            while ((idx = next_index.fetch_add(1)) < m_immutable_segments.size()) {
                auto& seg = *m_immutable_segments[idx];
                local_results.push_back(
                    seg.searchTopK(vector_name, query_vectors, k)
                );
            }
            return local_results;
        };

        // Launch workers
        for (size_t i = 0; i < num_threads; ++i)
            futures.push_back(std::async(std::launch::async, worker));

        // Collect results
        for (auto& f : futures) {
            auto local = f.get();
            for (auto& r : local)
                all_results.push_back(std::move(r));
        }

        // Merge across all segments
        QueryResult merged = mergeBatchResults(all_results, k);

        // auto end_time = std::chrono::high_resolution_clock::now();
        // merged.time_seconds =
        //     std::chrono::duration<double>(end_time - start_time).count();
        merged.status = Status::OK();

        return merged;
    }

    QueryResult mergeBatchResults(
        const std::vector<QueryResult>& results,
        size_t k) const
    {
        QueryResult merged;
        if (results.empty()) return merged;

        size_t num_queries = results.front().results.size();
        merged.results.resize(num_queries);

        // For each query in the batch
        for (size_t qi = 0; qi < num_queries; ++qi) {
            using Item = ScoredId;
            auto cmp = [](const Item& a, const Item& b) {
                return a.score < b.score; // higher score is better
            };
            std::priority_queue<Item, std::vector<Item>, decltype(cmp)> max_heap(cmp);

            // Gather all hits for this query index
            for (const auto& r : results) {
                if (qi >= r.results.size()) continue;
                for (const auto& hit : r.results[qi].hits) {
                    if (max_heap.size() < k) {
                        max_heap.push(hit);
                    } else if (hit.score > max_heap.top().score) {
                        max_heap.pop();
                        max_heap.push(hit);
                    }
                }
            }

            // Extract top-k (highest scores)
            std::vector<ScoredId> topk;
            topk.reserve(max_heap.size());
            while (!max_heap.empty()) {
                topk.push_back(max_heap.top());
                max_heap.pop();
            }
            std::reverse(topk.begin(), topk.end());
            merged.results[qi].hits = std::move(topk);
        }

        return merged;
    }


private:
    const CollectionInfo& m_collection_info;
    ActiveSegment m_active_segment;
    //might implement my own AI driven std::vector for capacity prediction expansion later. Cool stuff
    std::vector<std::unique_ptr<ImmutableSegment>> m_immutable_segments;
    // std::atomic<uint64_t> next_id_{0};//mayeb uuid is better for segment id?
};

} // namespace vectordb