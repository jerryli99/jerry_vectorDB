#pragma once

#include "../DataTypes.h"
#include "../third_party/hnswlib/hnswlib/hnswlib.h"
#include "../third_party/hnswlib/hnswlib/hnswalg.h"
#include "../third_party/hnswlib/hnswlib/space_l2.h"

#include <Eigen/Dense>
#include <memory>
#include <vector>
#include <utility>
#include <stdexcept>

namespace vectordb {

class HnswVectorIndex {
public:
    HnswVectorIndex(size_t dim, DistanceMetric metric, size_t max_elements,
                    size_t M = 16, size_t ef_construction = 200);

    void addPoint(const Eigen::VectorXf& vec, size_t id);
    void addBatch(const std::vector<std::pair<size_t, Eigen::VectorXf>>& points);

    std::vector<std::pair<size_t, float>> search(const Eigen::VectorXf& query, int top_k) const;
    std::vector<std::vector<std::pair<size_t, float>>> searchBatch(const std::vector<Eigen::VectorXf>& queries, int top_k) const;

    void setEf(size_t ef);

private:
    size_t dim_;
    DistanceMetric metric_;
    std::unique_ptr<hnswlib::SpaceInterface<float>> space_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> index_;
};

}
