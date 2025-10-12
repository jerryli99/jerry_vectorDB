// #include "IdTracker.h"

// namespace vectordb {

// //if i added the delete operation, (soft delete) then I will need to add code to skip the marked id.
// std::optional<PointOffSetType> IdTracker::get_internal_id(PointIdType point_id) const {
//     auto it = point_id_to_offset_.find(point_id);
//     return (it != point_id_to_offset_.end() ? std::make_optional(it->second) : std::nullopt);
// }

// std::optional<PointIdType> IdTracker::get_external_id(PointOffSetType offset) const {
//     return (offset < offset_to_point_id_.size() ? offset_to_point_id_[offset] : std::nullopt);
// }

// PointOffSetType IdTracker::insert(PointIdType point_id) {
//     if (auto existing = get_internal_id(point_id)) {
//         return *existing;
//     }

//     PointOffSetType offset;
//     if (!free_slots_.empty()) {
//         offset = free_slots_.top();
//         free_slots_.pop();
//         offset_to_point_id_[offset] = point_id;
//     } else {
//         offset = offset_to_point_id_.size();
//         offset_to_point_id_.push_back(point_id);
//     }
    
//     point_id_to_offset_[point_id] = offset;
//     return offset;
// }

// void IdTracker::remove(PointIdType point_id) {
//     auto it = point_id_to_offset_.find(point_id);
//     if (it == point_id_to_offset_.end()) return;

//     PointOffSetType offset = it->second;
//     offset_to_point_id_[offset] = std::nullopt;
//     point_id_to_offset_.erase(it);
//     free_slots_.push(offset);
// }

// std::vector<PointOffSetType> IdTracker::iter_internal_ids() const {
//     std::vector<PointOffSetType> result;
//     for (PointOffSetType i = 0; i < offset_to_point_id_.size(); ++i) {
//         if (offset_to_point_id_[i].has_value()) {
//             result.push_back(i);
//         }
//     }
//     return result;
// }

// std::vector<PointIdType> IdTracker::iter_external_ids() const {
//     std::vector<PointIdType> result;
//     result.reserve(point_id_to_offset_.size());
//     for (const auto& [point_id, _] : point_id_to_offset_) {
//         result.push_back(point_id);
//     }
//     return result;
// }

// size_t IdTracker::size() const {
//     return point_id_to_offset_.size();
// }

// bool IdTracker::empty() const {
//     return point_id_to_offset_.empty();
// }

// } // namespace vectordb