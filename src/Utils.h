#pragma once

#include "DataTypes.h"
#include <faiss/Index.h> // for MetricType
#include <algorithm> // for std::transform
#include <cctype>    // for ::tolower

namespace vectordb {

//ok, i use the trailing return type here to see if it works, just learning some c++ stuff
//std::transform is cool, this is not necessary, but i like to have some little fun.
auto parse_distance(const std::string& s) -> DistanceMetric {
    std::string s_lower = s;
    std::transform(s_lower.begin(), s_lower.end(), s_lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (s_lower == "l2") {
        return DistanceMetric::L2;
    } else if (s_lower == "dot") {
        return DistanceMetric::DOT;
    } else if (s_lower == "cosine") {
        return DistanceMetric::COSINE;
    } else {
        throw std::invalid_argument("Unknown distance metric: " + s);
    }
}

faiss::MetricType to_faiss_metric(DistanceMetric m) {
    switch (m) {
        case DistanceMetric::L2:     return faiss::METRIC_L2;
        case DistanceMetric::DOT:    return faiss::METRIC_INNER_PRODUCT;
        case DistanceMetric::COSINE: return faiss::METRIC_INNER_PRODUCT; // normalize vectors before
        default: throw std::invalid_argument("Unknown DistanceMetric enum");
    }
}

//extra stuff for debug printing
std::string to_string(DistanceMetric m) {
    switch (m) {
        case DistanceMetric::L2: return "L2";
        case DistanceMetric::DOT: return "Dot";
        case DistanceMetric::COSINE: return "Cosine";
        default: return "UNKNOWN";
    }
}

}