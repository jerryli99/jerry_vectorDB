#pragma once

#include <Eigen/Dense>
#include <vector>

namespace vector_db {

class Index {
public:
    virtual ~Index() = default;
    virtual void add(const Eigen::VectorXf& vec) = 0;
    virtual std::vector<int> search(const Eigen::VectorXf& query, int k) const = 0;
    virtual int dimension() const = 0;
};

}
