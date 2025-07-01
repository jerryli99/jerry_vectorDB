#pragma once

#include <Eigen/Dense>
#include <cstdint>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <string>

/**
 * Currently, I only consider the dense vector implementation
 */

namespace vectordb {

    using CollectionId = std::string;

    using VectorName = std::string;

    // Dense vector
    using DenseVector = Eigen::VectorXf;

    // Point ID (unique vector identifier) add UUID later
    using PointId = uint64_t;
    
    // Segment ID add UUID later
    using SegmentId = uint64_t;

    // JSON-based metadata payload
    using Payload = nlohmann::json;

    constexpr std::size_t MAX_SEGMENT_SIZE_KB = 100000;  // 100 MB

    //max tinymap entries
    constexpr std::size_t MAX_ENTRIES_TINYMAP = 6;

    enum class DistanceMetric {
        COSINE,
        EUCLIDEAN,
        DOT
    };

 }