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
//m_hnsw_index(std::make_unique<faiss::IndexHNSWFlat>(dim, 32))
Segment(SegmentIdType segmentid);
~Segment();

//dimension?
//point should be unique pointer in param? not sure yet.
void upsert(const std::vector<Point>& points);

void toImmutable();

//search the top-k results in this segment
//could assert(m_seg_type == SegmentType::Immutable && m_append_store.empty()) 
//in searchTopK() to catch bugs where appendable vectors are still hanging around??
void searchTopK(size_t topK);

//need to change this..but is used to search a particular point to get all the named vectors. 
//implement this extra function later 
void searchPoint(PointIdType pointid);


//markVectorDelete()
//FlushToDisk()

private:
size_t m_vector_dim;
//Each segment will have their own id tracker. At the end, 
//when searching, return the pointid and vector name.
IdTracker m_pointid_tracker; 
SegmentIdType m_segmentid;
PointPayloadStore m_payload_store;//can init this later in constructor
SegmentType m_seg_type {SegmentType::Appendable}; //by default;
AppendableStorage m_append_store;
size_t m_append_count{0};
std::unique_ptr<faiss::IndexHNSWFlat> m_hnsw_index;

};

}  // namespace vectordb


/*
Eigen by default is column-major, but FAISS expects row-major layout:
Each row = one vector
Shape must be [n_vectors][dimension]

void Segment::searchTopK(size_t topK, const DenseVector& query) {
    if (query.size() != m_vector_dim) {
        throw std::runtime_error("Query dimension mismatch");
    }

    std::vector<faiss::Index::idx_t> labels(topK);
    std::vector<float> distances(topK);

    m_hnsw_index->search(
        1,                // number of queries
        query.data(),     // pointer to query vector
        topK,
        distances.data(),
        labels.data()
    );

    for (size_t i = 0; i < topK; ++i) {
        std::cout << "Result " << i << ": ID = " << labels[i] << ", Distance = " << distances[i] << std::endl;
    }
}


*/

/*
Notes:

There could be VectorStorageType::InMemory, Mmap, ChunkedMmap
https://github.com/qdrant/qdrant/blob/master/lib/segment/src/segment_constructor/segment_constructor_base.rs

Mmap on disk, not appendable
Chunked mmap on disk, appendable

However, now the focus is just inMemory so i will disreguard these.

*/
