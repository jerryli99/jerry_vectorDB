#pragma once
#include <vector>
#include <Eigen/Dense>

class VectorDB {
public:
    void add_vector(const Eigen::Ref<const Eigen::VectorXf>& vec);  // Efficient NumPy->Eigen
    const std::vector<Eigen::VectorXf>& get_vectors() const;
    Eigen::VectorXf add_vectors(size_t idx1, size_t idx2) const;
    Eigen::VectorXf subtract_vectors(size_t idx1, size_t idx2) const;

private:
    std::vector<Eigen::VectorXf> vectors_;
};