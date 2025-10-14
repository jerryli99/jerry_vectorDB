#pragma once

#include "DataTypes.h"
#include "Status.h"
#include "Point.h"
#include "PointMemoryPool.h"
#include "CollectionInfo.h"
#include "ImmutableSegment.h"
#include "QueryResult.h"

#include <memory>
#include <mutex>
#include <functional>
#include <map>

namespace vectordb {

class ActiveSegment {
public:
    ActiveSegment(size_t max_capacity, const CollectionInfo& info)
        : m_pool{std::make_unique<PointMemoryPool>(max_capacity)}
        , m_info{info}
        , m_index_spec{info.index_specs}
        , m_max_capacity{max_capacity}
    {}

    ~ActiveSegment() = default;

    //prevent accidental copies
    ActiveSegment(const ActiveSegment&) = delete;
    ActiveSegment& operator=(const ActiveSegment&) = delete;

    //do allow moves though
    ActiveSegment(ActiveSegment&&) noexcept = default;
    ActiveSegment& operator=(ActiveSegment&&) noexcept = default;

    //insert single unnamed vector
    Status insertPoint(PointIdType point_id, const DenseVector& vector) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // std::cout << "Hello from activeSegment inserting 1 vector\n";
        auto* point = m_pool->allocatePoint(point_id);
        if (!point) {
            return Status::Error("Active segment is full");
        }

        if (!point->addVector("default", vector)) {
            m_pool->deallocatePoint(point);
            return Status::Error("Failed to add vector to point");
        }

        return Status::OK();
    }

    //insert multiple named vectors
    Status insertPoint(PointIdType point_id,
                       const std::map<VectorName, DenseVector>& named_vectors) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // std::cout << "Hello from activeSegment inserting multi namedvectors\n";
        auto* point = m_pool->allocatePoint(point_id);
        if (!point) {
            return Status::Error("Active segment is full");
        }

        for (const auto& [name, vec] : named_vectors) {
            if (!point->addVector(name, vec)) {
                m_pool->deallocatePoint(point);
                return Status::Error("Too many named vectors for TinyMap capacity");
            }
        }

        return Status::OK();
    }

    //Check if indexing threshold is reached
    bool shouldIndex() const {
        return getPointCount() >= m_index_spec.index_threshold;
    }

    //Check if segment is full
    bool isFull() const {
        return getPointCount() >= m_max_capacity;
    }

    //Get a copy of the data from this activeSegment obj and put them in immutableSegment for index building, then clearPool 
    //for the next incoming vector data. I choose to copy to keep it safe and simple...i will optimize in future.
    StatusOr<std::unique_ptr<ImmutableSegment>> convertToImmutable() {
        std::lock_guard<std::mutex> lock(m_mutex);

        //Extract data safely by copying
        SegPointData point_data;
        // Lock only as long as needed for extracting data, then unlock before doing heavier stuff
        auto points = m_pool->getAllPoints();
        if (points.empty()) {
            return Status::Error("No points to convert");
        }

        point_data.reserve(points.size());
        for (const auto* point : points) {
            point_data.emplace_back(point->getId(), point->getAllVectors());
        }

        try {
            auto immutable_segment = std::make_unique<ImmutableSegment>(point_data, m_info);
            
            //CLEAR THE POOL AFTER SUCCESSFUL CONVERSION
            m_pool->clearPool();

            return immutable_segment;

        } catch (const std::exception& e) {
            return Status::Error(std::string("ImmutableSegment creation failed: ") + e.what());
        }
    }


    // Get all points for conversion to immutable segment
    std::vector<Point*> getAllPoints() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool->getAllPoints();
    }

    // Get statistics about the segment
    size_t getPointCount() const {
        return m_pool->getTotalAllocated();
    }

    size_t getMaxCapacity() const {
        return m_max_capacity;
    }

    const IndexSpec& getIndexSpec() const {
        return m_index_spec;
    }

    // size_t getTinyMapCapacity() const {
    //     return TinyMapCapacity;
    // }
    QueryResult searchTopK(const std::string& vector_name,
                           const std::vector<DenseVector>& query_vectors,
                           size_t k) const 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        QueryResult query_result;
        // auto start_time = std::chrono::high_resolution_clock::now();

        try {
            //find the vector specification for the given vector name
            auto vec_spec_it = m_info.vec_specs.find(vector_name);
            if (vec_spec_it == m_info.vec_specs.end()) {
                query_result.status = Status::Error("Vector name '" + vector_name + "' not found in collection");
                // Return empty results for each query
                for (size_t i = 0; i < query_vectors.size(); ++i) {
                    query_result.results.push_back(QueryBatchResult{});
                }
                return query_result;
            }

            const VectorSpec& vec_spec = vec_spec_it->second;
            DistanceMetric metric = vec_spec.metric;
            size_t expected_dim = vec_spec.dim;

            // Validate query vector dimensions 
            for (size_t i = 0; i < query_vectors.size(); ++i) {
                if (query_vectors[i].size() != expected_dim) {
                    query_result.status = Status::Error(
                        "Query vector " + std::to_string(i) + " has dimension " + 
                        std::to_string(query_vectors[i].size()) + ", expected " + 
                        std::to_string(expected_dim) + "[@ActiveSegment]\n");
            
                    for (size_t j = 0; j < query_vectors.size(); ++j) {
                        query_result.results.push_back(QueryBatchResult{});
                    }
                    return query_result;
                }
            }

            // Get all points from the pool
            auto points = m_pool->getAllPoints();
            
            if (points.empty() || query_vectors.empty()) {
                query_result.status = Status::OK();
                // Add empty results for each query
                for (size_t i = 0; i < query_vectors.size(); ++i) {
                    query_result.results.push_back(QueryBatchResult{});
                }
                return query_result;
            }

            // Pre-filter points that have the target vector name and cache their data
            std::vector<PointIdType> valid_point_ids;
            std::vector<const DenseVector*> valid_vectors;

            for (const Point* point : points) {
                if (auto vector_opt = point->getVector(vector_name)) {
                    const DenseVector& vector_data = vector_opt.value();
                    // Validate stored vector dimension matches the spec
                    if (vector_data.size() != expected_dim) {
                        // Skip points with mismatched dimensions
                        continue;
                    }
                    valid_point_ids.push_back(point->getId());
                    valid_vectors.push_back(&vector_data);
                }
            }


            if (valid_vectors.empty()) {
                query_result.status = Status::OK(); // No matching points is not an error
                // Return empty results for each query
                for (size_t i = 0; i < query_vectors.size(); ++i) {
                    query_result.results.push_back(QueryBatchResult{});
                }
                return query_result;
            }

            // Process each query vector
            for (const auto& query_vector : query_vectors) {
                QueryBatchResult batch_result;
                std::vector<ScoredId> scored_points;
                scored_points.reserve(valid_vectors.size());

                // Calculate scores for all valid points against this query
                for (size_t i = 0; i < valid_vectors.size(); ++i) {
                    float score = compute_distance(metric, query_vector, *valid_vectors[i]);
                    scored_points.emplace_back(ScoredId{valid_point_ids[i], score});
                }

                // Sort based on metric type and take top K
                if (metric == DistanceMetric::L2) {
                    // For L2, lower distance is better
                    if (scored_points.size() > k) {
                        std::partial_sort(
                            scored_points.begin(),
                            scored_points.begin() + k,
                            scored_points.end(),
                            [](const ScoredId& a, const ScoredId& b) {
                                return a.score < b.score;
                            }
                        );
                        batch_result.hits.assign(scored_points.begin(), scored_points.begin() + k);
                    } else {
                        std::sort(scored_points.begin(), scored_points.end(),
                                [](const ScoredId& a, const ScoredId& b) {
                                    return a.score < b.score;
                                });
                        batch_result.hits = std::move(scored_points);
                    }
                } else {
                    // For DOT and COSINE, higher score is better
                    if (scored_points.size() > k) {
                        std::partial_sort(
                            scored_points.begin(),
                            scored_points.begin() + k,
                            scored_points.end(),
                            [](const ScoredId& a, const ScoredId& b) {
                                return a.score > b.score;
                            }
                        );
                        batch_result.hits.assign(scored_points.begin(), scored_points.begin() + k);
                    } else {
                        std::sort(scored_points.begin(), scored_points.end(),
                                [](const ScoredId& a, const ScoredId& b) {
                                    return a.score > b.score;
                                });
                        batch_result.hits = std::move(scored_points);
                    }
                }

                query_result.results.push_back(std::move(batch_result));
            }

            query_result.status = Status::OK();

        } catch (const std::exception& e) {
            query_result.status = Status::Error(std::string("Search failed: ") + e.what());
            // Ensure we have the right number of result slots even on error
            while (query_result.results.size() < query_vectors.size()) {
                query_result.results.push_back(QueryBatchResult{});
            }
        }

        // auto end_time = std::chrono::high_resolution_clock::now();
        // result.time_seconds = std::chrono::duration<double>(end_time - start_time).count();

        return query_result;
    }

private:
    SegmentType seg_type{SegmentType::Appendable};
    std::unique_ptr<PointMemoryPool> m_pool;
    CollectionInfo m_info;
    IndexSpec m_index_spec;
    size_t m_max_capacity;
    mutable std::mutex m_mutex;
};

} // namespace vectordb
