#pragma once

#include "Index.h"
#include <vector>

namespace vector_db {

class IndexFlat : public Index {
public:
    explicit IndexFlat(int dim);
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
