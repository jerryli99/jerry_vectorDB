#include "Segment.h"

namespace vectordb {

    void upsert(const std::vector<Point>& points) {
        
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