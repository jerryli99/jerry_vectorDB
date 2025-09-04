#pragma once

#include <iostream>
// #include <Eigen/Dense> //ahh, this eigen thing is really annoying
#include <cstdint>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <cstdint>
#include <filesystem>

/**
 * Currently, I only consider the dense vector implementation, sparse vectors later
 */

namespace vectordb {

    using CollectionId = std::string;

    using VectorName = std::string;

    // Dense vector, since faiss uses row major order, i will have to do this.
    using DenseVector = std::vector<float>; //Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>;

    // Point ID (unique vector identifier) add UUID later
    using PointIdType = std::string; //i will use uuid later. //std::variant<std::string, uint64_t>;
    
    //{point_id, distance}
    using SearchResult = std::pair<PointIdType, float>;//?

    // Segment ID add UUID later
    using SegmentIdType = std::string;
    
    using PointOffSetType = size_t;

    using json = nlohmann::json;

    //use this name for better type identification
    using Payload = nlohmann::json;

    using AppendableStorage = std::vector<DenseVector>;//Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    // constexpr std::size_t MAX_SEGMENT_SIZE_BYTES = 1024 * 1024; // 1 MiB

    /*
    constexpr ensures compile-time evaluation (no runtime overhead).
    inline prevents "multiple definition" errors when included in headers.
    */
    inline constexpr size_t INDEX_THRESHOLD_SIZE_BYTES = 1024 * 1024;  // configurable threshold??

    //move these 2 later in config part...
    inline constexpr size_t CACHE_SIZE = 128;// 128MB cache, can be specified by user...

    //could be adjusted, uhm, yeah i am thinking about just to have a config file here..but whatever, get the job done first.
    const std::filesystem::path PAYLAOD_DIR = "./VectorDB/Payload";

    //max tinymap entries
    inline constexpr size_t MAX_ENTRIES_TINYMAP = 3;

    inline constexpr size_t PRE_RESERVE_NUM_SEGMENTS = 1024;

    enum class DistanceMetric {
        L2,
        DOT,
        COSINE,
        UNKNOWN,
    };

    enum class CollectionStatus {
        //?
    };

    //i mean obiviously there are more types of error,
    //but i am lazy now to add more
    enum class APIErrorType {
        UserInput,
        Server,
        Connection
    };

    enum class SegmentType {
        Appendable,
        Immutable,
    };

    enum class SegmentStatus {
        Empty,
        Active,
        Full,
        NewMerged,
        None,
    };

    enum class WalTruncateMode {
        FULL,
        KEEP_LAST_N,
    };
 }