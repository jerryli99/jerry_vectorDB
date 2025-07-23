// #pragma once

// #include "DataTypes.h"

// #include <Eigen/Dense>
// #include <cmath> //std::sqrt

// namespace vectordb {

//     inline float calculate_distance(const DistanceMetric metric, 
//                                     const DenseVector& a, 
//                                     const DenseVector& b) {
//         switch (metric) {
//             case DistanceMetric::L2:
//                 return (a - b).squaredNorm();
//             case DistanceMetric::DOT:
//                 return a.dot(b);
//             case DistanceMetric::COSINE: {
//                 float dot_product = a.dot(b);
//                 float norm_a = a.norm();
//                 float norm_b = b.norm();
//                 return dot_product / (norm_a * norm_b);
//             }
//             default:
//                 throw std::runtime_error("Unknown distance metric");
//                 //Uhmmm.. maybe have a better error handle machanism in future?
//         }
//     }
// } // namespace vectordb

