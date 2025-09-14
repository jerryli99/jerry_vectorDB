// #include "SegmentHolder.h"

// namespace vectordb {

//     SegmentHolder::SegmentHolder() {
//         // Reserve space for x amount of segments
//         m_segments.reserve(PRE_RESERVE_NUM_SEGMENTS);
//     }

//     std::string SegmentHolder::generate_new_key() {
//         //generate key, and if key exists in the holder, regenerate. I will do this later
//         next_id_ = next_id_ + 1;
//         return ("seg_" + std::to_string(next_id_));
//     }

//     //The segment gets assigned a new unique ID.
//     void SegmentHolder::addSegment(std::shared_ptr<Segment> segment) {
//         SegmentIdType segment_id;
//         bool is_unique;
        
//         //later modify this code once i used uuid
//         do {
//             segment_id = generate_new_key();
//             is_unique = true;
            
//             // Check if this ID already exists
//             for (const auto& pair : m_segments) {
//                 if (pair.first == segment_id) {
//                     is_unique = false;
//                     break;
//                 }
//             }
//         } while (!is_unique);

//         m_segments.emplace_back(segment_id, SegmentEntry{.m_segment = segment});
//     }
    
//     //search all segments in the segmentholder, for each segment, get topK, 
//     //at the same time, maintain a fixed k length priority queue for storing 
//     //all these topk results, return uhm maybe a list of top k?
//     //void searchSegmentTop(...);

//     // std::optional<std::shared_ptr<Segment>> SegmentHolder::getSegment(SegmentIdType id) const {
//     //     auto it = m_segments.find(id);
//     //     if (it != m_segments.end()) {
//     //         return it->second.m_segment;
//     //     } else {
//     //         return std::nullopt;
//     //     }
//     // }

//     const std::vector<std::pair<SegmentIdType, SegmentEntry>> SegmentHolder::getAllSegments() const {
//         return m_segments;
//     }

// }