#pragma once

#include "DataTypes.h"
#include "Status.h"
#include "Point.h"
#include "WAL.h"
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
            SegmentIdType seg_id = generateSegmentId();
            auto immutable_segment = std::make_unique<ImmutableSegment>(point_data, m_info, seg_id);
            
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
        // std::cout << "In ActiveSeg searchTopK\n";
        // std::cout << "[DEBUG] searchTopK k=" << k << "\n";
        // std::cout << "[DEBUG] searchTopK got " << query_vectors.size() << " query vectors\n";
        if (!query_vectors.empty())
            std::cout << "[ERROR] first query vector size=" << query_vectors[0].size() << "\n";
        QueryResult query_result;

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
            std::vector<DenseVector> valid_vectors;

            for (const Point* point : points) {
                auto vec_opt = point->getVector(vector_name);
                if (!vec_opt.has_value()) {
                    std::cout << "[WARN] point " << point->getId() << " missing vector " << vector_name << "\n";
                    continue;
                }

                const DenseVector& v = vec_opt.value();
                if (v.empty()) {
                    std::cout << "[WARN] point " << point->getId() << " vector empty\n";
                    continue;
                }

                valid_vectors.push_back(v); 
                valid_point_ids.push_back(point->getId());

            }

            // -----------------------After collecting points and valid_vectors:
            std::cout << "[DEBUG] ActiveSegment: total points in pool=" << points.size()
                    << ", valid_vectors=" << valid_vectors.size() << "\n";
            //---------------------------------------------------------

            if (valid_vectors.empty()) {
                query_result.status = Status::OK(); // No matching points is not an error
                // Return empty results for each query
                for (size_t i = 0; i < query_vectors.size(); ++i) {
                    query_result.results.push_back(QueryBatchResult{});
                }
                return query_result;
            }

            std::cout << "hello\n";
            // Process each query vector
            for (const auto& query_vector : query_vectors) {
                std::cout << "[DEBUG] new query start\n";
                QueryBatchResult batch_result;
                std::vector<ScoredId> scored_points;
                scored_points.reserve(valid_vectors.size());
                std::cout << "[DEBUG] valid_vectors.size()=" << valid_vectors.size() << "\n";
                
                //-------------------
                for (size_t i = 0; i < valid_vectors.size(); ++i) {
                    try {
                        float score = compute_distance(metric, query_vector, valid_vectors[i]);
                        // Unify metric convention: higher = better
                        if (metric == DistanceMetric::L2)
                            score = -score;
                        score = std::round(score * 10000.0f) / 10000.0f; //just round to 4 digits
                        // if (i < 3) std::cout << "[DEBUG] i=" << i << " score=" << score << "\n";
                        scored_points.emplace_back(ScoredId{valid_point_ids[i], score});
                    } catch (const std::exception& e) {
                        std::cerr << "[ERROR] compute_distance threw: " << e.what()
                                << " for i=" << i << " qsize=" << query_vector.size()
                                << " stored.size=" << (valid_vectors[i]).size()
                                << "\n";
                        // skip this point and continue — don't abort whole query
                        continue;
                    }
                }
                //--------------------
                std::cout << "[DEBUG] scored_points.size()=" << scored_points.size() << "\n";
                // Sort based on metric type and take top K
                if (metric == DistanceMetric::L2) {
                    // For L2, lower distance is better, but since i negate the score, so higher the better
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

        for (size_t qi = 0; qi < query_result.results.size(); ++qi) {
            std::cout << "[DEBUG] Final batch " << qi << " hits=" << query_result.results[qi].hits.size() << "\n";
        }
        return query_result;
    }

private:
    SegmentType seg_type{SegmentType::Appendable};
    std::unique_ptr<PointMemoryPool> m_pool;
    CollectionInfo m_info;
    IndexSpec m_index_spec;
    size_t m_max_capacity;
    mutable std::mutex m_mutex;

    //generate UUID-based segment ID, not sure if i should make it static, but for now sure.
    static std::string generateSegmentId() {
        uuid_t uuid;
        uuid_generate(uuid);
        
        char uuid_str[37]; // 36 chars + null terminator
        uuid_unparse(uuid, uuid_str);
        
        return "Segment_" + std::string(uuid_str);
    }
};

} // namespace vectordb
