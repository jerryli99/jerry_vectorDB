#include "index/IndexFlat.h"
#include <stdexcept>
#include <algorithm>
#include <limits>

using namespace vectordb;

IndexFlat::IndexFlat(int dim) : dim(dim) {}

void IndexFlat::add(const Eigen::VectorXf& vec) {
    if (vec.size() != dim) throw std::runtime_error("Dimension mismatch");
    vectors.push_back(vec);
}

std::vector<int> IndexFlat::search(const Eigen::VectorXf& query, int k) const {
    if (query.size() != dim) throw std::runtime_error("Query dimension mismatch");

    std::vector<std::pair<float, int>> distances;
    for (int i = 0; i < vectors.size(); ++i) {
        float dist = (vectors[i] - query).squaredNorm();
        distances.emplace_back(dist, i);
    }

    std::partial_sort(distances.begin(), distances.begin() + k, distances.end());
    std::vector<int> result;
    for (int i = 0; i < k && i < distances.size(); ++i) {
        result.push_back(distances[i].second);
    }

    return result;
}

int IndexFlat::dimension() const {
    return dim;
}

std::vector<Eigen::VectorXf> IndexFlat::get_all() const {
    return vectors;
}

Eigen::VectorXf IndexFlat::get_vector(int index) const {
    if (index < 0 || index >= vectors.size()) {
        throw std::out_of_range("Index out of range");
    }
    return vectors[index];
}
