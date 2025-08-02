#include "SegmentHolder.h"

namespace vectordb {
    std::string SegmentHolder::generate_new_key() {
        //generate key, and if key exists in the holder, regenerate. I will do this later
        next_id_ = next_id_ + 1;
        return ("seg_" + std::to_string(next_id_));
    }

    //The segment gets assigned a new unique ID.
    void SegmentHolder::addSegment(std::shared_ptr<Segment> segment) {
        SegmentIdType segment_id;
        do {
            segment_id = generate_new_key();
        } while (m_segments.find(segment_id) != m_segments.end());

        m_segments[segment_id] = SegmentEntry{ .m_segment = segment };
    }

    std::optional<std::shared_ptr<Segment>> SegmentHolder::getSegment(SegmentIdType id) const {
        auto it = m_segments.find(id);
        if (it != m_segments.end()) {
            return it->second.m_segment;
        } else {
            return std::nullopt;
        }
    }

    const std::unordered_map<SegmentIdType, SegmentEntry> SegmentHolder::getAllSegments() const {
        return m_segments;
    }

}