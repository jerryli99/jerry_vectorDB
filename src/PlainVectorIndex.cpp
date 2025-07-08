#include "../include/index/PlainVectorIndex.h"

namespace vectordb {

PlainVectorIndex::PlainVectorIndex(size_t dim, DistanceMetric metric)
    : dim_(dim), metric_(metric) {}

void PlainVectorIndex::addPoint(const DenseVector& vec, size_t id) {
    if (vec.size() != dim_) {
        throw std::runtime_error("Vector dimension mismatch");
    }
    vectors_.emplace_back(id, vec);
}

void PlainVectorIndex::addBatch(const std::vector<std::pair<size_t, DenseVector>>& points) {
    for (const auto& point : points) {
        addPoint(point.second, point.first);
    }
}

float PlainVectorIndex::calculateDistance(const DenseVector& a, const DenseVector& b) const {
    if (metric_ == DistanceMetric::L2) {
        return (a - b).squaredNorm();  // L2 squared distance
    } else {  // DistanceMetric::IP
        return -a.dot(b);  // Negative dot product for consistent ordering
    }
}

std::vector<std::pair<size_t, float>> PlainVectorIndex::search(const DenseVector& query, int top_k) const {
    if (query.size() != dim_) {
        throw std::runtime_error("Query dimension mismatch");
    }

    std::vector<std::pair<float, size_t>> distances;  // (distance, id)
    distances.reserve(vectors_.size());

    // Calculate distances to all vectors
    for (const auto& [id, vec] : vectors_) {
        distances.emplace_back(calculateDistance(query, vec), id);
    }

    // Sort based on distance (ascending for L2, descending for IP)
    if (metric_ == DistanceMetric::L2) {
        std::sort(distances.begin(), distances.end());
    } else {
        std::sort(distances.rbegin(), distances.rend());
    }

    // Prepare results (convert to id-first pairs)
    std::vector<std::pair<size_t, float>> results;
    size_t result_count = std::min(static_cast<size_t>(top_k), distances.size());
    results.reserve(result_count);

    for (size_t i = 0; i < result_count; i++) {
        results.emplace_back(distances[i].second, distances[i].first);
    }

    return results;
}

std::vector<std::vector<std::pair<size_t, float>>> PlainVectorIndex::searchBatch(
    const std::vector<DenseVector>& queries, int top_k) const {
    std::vector<std::vector<std::pair<size_t, float>>> results;
    results.reserve(queries.size());

    for (const auto& query : queries) {
        results.push_back(search(query, top_k));
    }

    return results;
}

} // namespace vectordb