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
    HnswVectorIndex(size_t dim, 
                    DistanceMetric metric, 
                    size_t max_elements,
                    size_t M = 32, 
                    size_t ef_construction = 512);

    void addPoints(const DenseVector& vec, size_t id);

    std::vector<std::pair<size_t, float>> search(const DenseVector& query, int top_k) const;

    //update

    //delete
    //use bitmap or bitset?

    // Before adding/querying vectors, normalize them:
    DenseVector normalize(const DenseVector& vec);

    void setEf(size_t ef);

private:
    size_t dim_;
    DistanceMetric metric_;
    bool normalize_;
    std::unique_ptr<hnswlib::SpaceInterface<float>> space_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> index_;
};

}
