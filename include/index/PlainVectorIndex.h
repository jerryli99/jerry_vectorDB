#pragma once

#include "../DataTypes.h"
#include <Eigen/Dense>
#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>

namespace vectordb {

class PlainVectorIndex {
public:
    PlainVectorIndex(size_t dim, DistanceMetric metric);
    
    void addPoint(const DenseVector& vec, size_t id);
    void addBatch(const std::vector<std::pair<size_t, DenseVector>>& points);
    
    std::vector<std::pair<size_t, float>> search(const DenseVector& query, int top_k) const;
    std::vector<std::vector<std::pair<size_t, float>>> searchBatch(const std::vector<DenseVector>& queries, int top_k) const;

private:
    size_t dim_;
    DistanceMetric metric_;
    std::vector<std::pair<size_t, DenseVector>> vectors_;  // Store (id, vector) pairs
    
    float calculateDistance(const DenseVector& a, const DenseVector& b) const;
};

} // namespace vectordb