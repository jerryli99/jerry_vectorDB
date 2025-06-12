#include "vector_db.h"
#include <stdexcept>

void VectorDB::add_vector(const Eigen::Ref<const Eigen::VectorXf>& vec) {
    vectors_.emplace_back(vec);
}

const std::vector<Eigen::VectorXf>& VectorDB::get_vectors() const {
    return vectors_;
}

Eigen::VectorXf VectorDB::add_vectors(size_t idx1, size_t idx2) const {
    if (idx1 >= vectors_.size() || idx2 >= vectors_.size()) {
        throw std::out_of_range("Vector indices out of range");
    }
    return vectors_[idx1] + vectors_[idx2];  // Eigen handles the math
}

Eigen::VectorXf VectorDB::subtract_vectors(size_t idx1, size_t idx2) const {
    if (idx1 >= vectors_.size() || idx2 >= vectors_.size()) {
        throw std::out_of_range("Vector indices out of range");
    }
    return vectors_[idx1] - vectors_[idx2];
}