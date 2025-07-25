#pragma once

#include <Eigen/Dense>
#include <cstdint>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <cstdint>

/**
 * Currently, I only consider the dense vector implementation
 */

namespace vectordb 
{

    using CollectionId = std::string;

    using VectorName = std::string;

    // Dense vector
    //note: when using Faiss lib, do .data() to convert to float*
    // using DenseVector = Eigen::VectorXf;
    using DenseVector = std::vector<float>;

    // Point ID (unique vector identifier) add UUID later
    using PointIdType = std::variant<std::string, uint64_t>;
    
    //{point_id, distance}
    using SearchResult = std::pair<PointIdType, float>;

    // Segment ID add UUID later
    using SegmentIdType = uint64_t;
    
    using PointOffSetType = size_t;

    // JSON-based metadata payload
    using Payload = nlohmann::json;

    constexpr std::size_t MAX_SEGMENT_SIZE_KB = 100000;  // 100 MB

    const size_t FLUSH_THRESHOLD = 10000;  // configurable threshold??

    //max tinymap entries
    constexpr std::size_t MAX_ENTRIES_TINYMAP = 3;

    enum class DistanceMetric {
        L2,
        DOT,
        COSINE
    };

    enum class CollectionStatus {
        // //no idea yet
        // Green,   // Ready
        // Yellow,  // Degraded / partially loaded
        // Red      // Loading or unavailable
    };

    enum class SegmentType {
        Appendable,
        Immutable,
    };

    enum class SegmentStatus {
        Empty,
        Active,
        Full,
        NotFound,
    };
 }