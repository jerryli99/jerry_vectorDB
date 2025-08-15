#include "Segment.h"

namespace vectordb {

    /*
    So perhaps it is fine to add point ids to id tracker in appendable phase?
    Because once we added our data to 
    */
    void upsert(const VectorName& vec_name, const DenseVector& dense_vec) {
        //warning, if we keep adding vectors, make sure once threshold is met, 
        //create another segment for the rest remaining vectors?
    }
    /*
    Add all the vectors from appendable storage to building hnsw index with faisslib
    Will also change the seg_type to SegmentType::Immutable
    */
    void Segment::toImmutable() {
        if (m_seg_type == SegmentType::Immutable || m_append_count == 0) return;

        m_hnsw_index->add(m_append_count, m_append_store.data());

        m_append_store.resize(0, m_vector_dim);  // free memory
        m_append_count = 0;
        m_seg_type = SegmentType::Immutable;
    }
}