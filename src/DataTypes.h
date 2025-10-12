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
#include <atomic>


/**
 * Currently, I only consider the dense vector implementation, sparse vectors later
 */

namespace vectordb {

    using CollectionId = std::string;

    using VectorName = std::string;

    // Dense vector, since faiss uses row major order, i will have to do this.
    using DenseVector = std::vector<float>; 

    // Point ID (unique vector identifier) add UUID later
    using PointIdType = std::string; //i will use uuid later. //std::variant<std::string, uint64_t>;
    
    //{point_id, distance}
    using SearchResult = std::pair<PointIdType, float>;//?

    // Segment ID add UUID later
    using SegmentIdType = std::string;
    
    using PointOffSetType = std::size_t;

    using json = nlohmann::json;

    //use this name for better type identification
    using Payload = nlohmann::json;

    using SegPointData = std::vector<std::pair<PointIdType, std::map<VectorName, DenseVector>>>;

    
    //constexpr ensures compile-time evaluation (no runtime overhead).
    //inline prevents "multiple definition" errors when included in headers.
    //move these later in Config.h part...
    inline constexpr size_t CACHE_SIZE = 128;// 128MB cache, can be specified by user...

    //could be adjusted, uhm, yeah i am thinking about just to have a config file here..but whatever, get the job done first.
    const std::filesystem::path PAYLOAD_DIR = "./VectorDB/Payload";

    //max tinymap entries
    inline constexpr size_t MAX_ENTRIES_TINYMAP = 8;

    inline constexpr size_t MIN_ENTRIES_TINYMAP = 1;

    inline constexpr size_t PRE_RESERVE_NUM_SEGMENTS = 1024; //??? maybe uhm, well...

    // Configurable rate limits for every http request for upsert points
    //i might add another special endpoint for async, but for now just do things simple.
    inline constexpr std::size_t MAX_POINTS_PER_REQUEST = 1000;

    inline constexpr size_t MAX_JSON_REQUEST_SIZE = 32 * 1024 * 1024; // 32MB

    inline constexpr std::size_t MAX_MEMORYPOOL_POINTS = 10000;

    inline constexpr std::size_t TINY_MAP_CAPACITY = 8;

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

    //uhm, maybe this is useless??
    enum class SegmentType {
        Appendable,
        Immutable,
        Merged,
    };

    //user client can see how things are going before 
    //continue upserting or doing other stuff...
    enum class SegmentStatus {
        Empty,          // No points yet
        Active,         // Accepting new points
        Full,           // Reached capacity, ready for merge
        Merging,        // Currently being merged
        Immutable,      // Merged and indexed, read-only
        ContinueInsert, // Ready for more data
        Error           // Something went wrong
    };

    enum class WalTruncateMode {
        FULL,
        KEEP_LAST_N,
    };
 }