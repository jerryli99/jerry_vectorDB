#pragma once

#include "DataTypes.h"
#include "IdTracker.h"


/*
Each segment has vectors, payloads stored.

*/

namespace vectordb 
{

class Segment 
{
public:



};

}

/*
Notes:

There could be VectorStorageType::InMemory, Mmap, ChunkedMmap
https://github.com/qdrant/qdrant/blob/master/lib/segment/src/segment_constructor/segment_constructor_base.rs

Mmap on disk, not appendable
Chunked mmap on disk, appendable

However, now the focus is just inMemory so i will disreguard these.

*/








// namespace vectordb {

// class Segment {
// public:
//     virtual ~Segment() = default;

//     virtual bool isAppendable() const = 0;

//     virtual void insertPoint(PointIdType id, const DenseVector& vec) = 0;

//     virtual void insertBatch(const std::vector<std::pair<PointIdType, DenseVector>>& points) = 0;
    
//     virtual std::vector<std::pair<size_t, float>> search(const DenseVector& query, int top_k) const = 0;

//     virtual std::shared_ptr<IdTracker> getIdTracker() const = 0;
// };

// } // namespace vectordb

/*
Add deletePoint and updatePoint to the Segment interface.

Expose segment flushing or conversion functions (Appendable → Immutable).

Parallel search in segments using std::async or thread pool.
*/

// #include "DataTypes.h"
// #include "Point.h"
// #include "index/IndexHNSW.h"

// #include <unordered_map>
// #include <vector>
// #include <stdexcept>

// namespace vectordb {

//     struct Segment {
//         SegmentIdType segment_id;
//         virtual void addPoint(const Point& point) = 0;
//         virtual bool isAppendable() const = 0;
//         virtual void flush_to_immutable_segment() = 0;
//         virtual ~Segment() {}
//     };

//     struct AppendableSegment : Segment {
//         std::unordered_map<PointIdType, size_t> external_to_internal;
//         std::vector<PointIdType> internal_to_external;

//         std::vector<DenseVector> vectors;
//         std::vector<Payload> payloads;

//         void addPoint(const Point& point) override {

//         }

//         bool isAppendable() const override { return true; }

//         void flush() override {
//             // TODO: persist to disk (binary or mmap), or convert to ReadOnlySegment
//         }
//     };

//     struct ReadOnlySegment : Segment {
//         IndexHNSW hnsw;

//         std::unordered_map<PointIdType, size_t> external_to_internal;
//         std::vector<PointIdType> internal_to_external;

//         std::vector<DenseVector> vectors;
//         std::vector<Payload> payloads;

//         std::vector<PointIdType> search(const std::string& vector_name, const DenseVector& query, int top_k) const {

//         }

//         void addPoint(const Point&) override {
//             throw std::runtime_error("ReadOnlySegment does not support insert.");
//         }

//         bool isAppendable() const override { return false; }

//         void flush() override {
//             // Already read-only — maybe noop or index compaction
//         }
//     };

// }
