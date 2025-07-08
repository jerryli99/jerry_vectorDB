#include "../include/index/HnswVectorIndex.h"

#include <stdexcept>
#include <iostream>

namespace vectordb {

HnswVectorIndex::HnswVectorIndex(size_t dim, DistanceMetric metric, size_t max_elements,
                                 size_t M, size_t ef_construction)
    : dim_(dim), metric_(metric) {
    // Create the appropriate space based on the distance metric
    switch (metric) {
        case DistanceMetric::L2:
            space_ = std::make_unique<hnswlib::L2Space>(dim);
            break;
        case DistanceMetric::DOT:
            space_ = std::make_unique<hnswlib::InnerProductSpace>(dim);
            break;
        default:
            throw std::runtime_error("Unsupported distance metric");
    }

    // Create the HNSW index
    index_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(
        space_.get(), max_elements, M, ef_construction);
}

void HnswVectorIndex::addPoint(const Eigen::VectorXf& vec, size_t id) {
    if (vec.size() != dim_) {
        throw std::runtime_error("Vector dimension mismatch");
    }

    // Convert Eigen vector to raw float array
    const float* data = vec.data();
    index_->addPoint(data, static_cast<hnswlib::labeltype>(id));
}

void HnswVectorIndex::addBatch(const std::vector<std::pair<size_t, Eigen::VectorXf>>& points) {
    for (const auto& point : points) {
        addPoint(point.second, point.first);
    }
}

std::vector<std::pair<size_t, float>> HnswVectorIndex::search(const Eigen::VectorXf& query, int top_k) const {
    if (query.size() != dim_) {
        throw std::runtime_error("Query dimension mismatch");
    }

    // Convert Eigen vector to raw float array
    const float* query_data = query.data();

    // Search the index
    auto result = index_->searchKnn(query_data, top_k);

    // Convert the priority queue to a vector of pairs
    std::vector<std::pair<size_t, float>> results;
    while (!result.empty()) {
        results.emplace_back(result.top().second, result.top().first);
        result.pop();
    }

    // Reverse to get descending order (highest similarity / lowest distance first)
    std::reverse(results.begin(), results.end());
    return results;
}

std::vector<std::vector<std::pair<size_t, float>>> HnswVectorIndex::searchBatch(
    const std::vector<Eigen::VectorXf>& queries, int top_k) const {
    std::vector<std::vector<std::pair<size_t, float>>> results;
    results.reserve(queries.size());

    for (const auto& query : queries) {
        results.push_back(search(query, top_k));
    }

    return results;
}

void HnswVectorIndex::setEf(size_t ef) {
    index_->setEf(ef);
}

} // namespace vectordb