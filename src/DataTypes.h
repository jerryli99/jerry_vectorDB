#pragma once

#include <iostream>
#include <Eigen/Dense>
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
    using DenseVector = Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>;;

    // Point ID (unique vector identifier) add UUID later
    using PointIdType = std::string; //i will use uuid later. //std::variant<std::string, uint64_t>;
    
    //{point_id, distance}
    using SearchResult = std::pair<PointIdType, float>;//?

    // Segment ID add UUID later
    using SegmentIdType = std::string;
    
    using PointOffSetType = size_t;

    using Payload = nlohmann::json;

    using AppendableStorage = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>; // std::vector<DenseVector>;
    // constexpr std::size_t MAX_SEGMENT_SIZE_BYTES = 1024 * 1024; // 1 MiB

    /*
    constexpr ensures compile-time evaluation (no runtime overhead).
    inline prevents "multiple definition" errors when included in headers.
    */
    inline constexpr size_t INDEX_THRESHOLD_SIZE_BYTES = 1024 * 1024;  // configurable threshold??

    //move these 2 later in config part...
    inline constexpr size_t CACHE_SIZE = 128;// 128MB cache, can be specified by user...
    const std::filesystem::path PAYLAOD_DIR = "./VectorDB/Payload";

    //max tinymap entries
    inline constexpr size_t MAX_ENTRIES_TINYMAP = 3;

    inline constexpr size_t PRE_RESERVE_NUM_SEGMENTS = 1024;

    enum class DistanceMetric {
        L2,
        DOT,
        COSINE
    };

    enum class CollectionStatus {
        //?
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
 }


     // using PayloadValue = std::variant<
    //     int64_t,
    //     double,
    //     bool,
    //     std::string,
    //     std::vector<int64_t>,
    //     std::vector<double>,
    //     std::vector<bool>,
    //     std::vector<std::string>,
    // >; //skipped geo points here...
    // using PayloadIDType = std::string;

    // //intent: payload is stored per point not per named vector
    // //this is more like a placeholder, we store payload in rocksdb
    // using Payload = std::unordered_map<PayloadIDType, PayloadValue>;