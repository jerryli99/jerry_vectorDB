#pragma once

#include "DataTypes.h"
#include "IdTracker.h"
#include "Point.h"
#include "PointPayloadStore.h"

#include <faiss/IndexHNSW.h>

/*
Each segment has vectors, payloads stored.


Uhm, still thinking about adding SegmentInfo struct.
*/


namespace vectordb {

class Segment {
public:
Segment(SegmentIdType segmentid);
~Segment();

void toImmutableType();

//dimension?
//point should be unique pointer in param? not sure yet.
void addPoint(Point& point);

//calculate only the (vector dim) * (# of vectors) in segment
//maybe use lambda functions in future?
const uint64_t calculateSegmentSizeBytes();

void buildIndex();//?? need this?

//search the top-k results in this segment
void searchTopK(size_t topK);

//need to change this..but is used to search a particular point to get all the named vectors. 
//implement this extra function later 
void searchPoint(PointIdType pointid);



//markVectorDelete()
//FlushToDisk()

private:
//Each segment will have their own id tracker. At the end, 
//when searching, return the pointid and vector name.
IdTracker m_pointid_tracker; 
SegmentIdType m_segmentid;
PointPayloadStore m_payload_store;//can init this later in constructor
SegmentType m_curr_seg_type {SegmentType::Appendable}; //by default;
AppendableStorage m_append_store;

};

}  // namespace vectordb

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
