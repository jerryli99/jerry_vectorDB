// #include "../include/index/PlainVectorIndex.h"

// namespace vectordb {

// PlainVectorIndex::PlainVectorIndex(size_t dim, DistanceMetric metric)
//     : dim_(dim), metric_{(metric) {}



// float PlainVectorIndex::getDistance(const DenseVector& a, const DenseVector& b) const {
//     if (metric_ == DistanceMetric::L2) {
//         return (a - b).squaredNorm();  // L2 squared distance
//     } else {  // DistanceMetric::IP
//         return -a.dot(b);  // Negative dot product for consistent ordering
//     }
// }

// } // namespace vectordb