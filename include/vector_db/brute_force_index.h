#pragma once

#include "index.h"
#include <vector>

namespace vector_db {

class BruteForceIndex : public Index {
public:
    explicit BruteForceIndex(int dim);
    void add(const Eigen::VectorXf& vec) override;
    std::vector<int> search(const Eigen::VectorXf& query, int k) const override;
    int dimension() const override;
    std::vector<Eigen::VectorXf> get_all() const;
    Eigen::VectorXf get_vector(int index) const;


private:
    int dim;
    std::vector<Eigen::VectorXf> vectors;
};

}
