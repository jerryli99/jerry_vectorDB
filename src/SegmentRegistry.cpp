




// namespace vectordb {
// 
// SegmentRegistry::SegmentRegistry(size_t dim, DistanceMetric metric)
//     : dim_(dim),
//       metric_(metric),
//       current_size_bytes_(0) {
//     createNewAppendableSegment();
// }

// void SegmentRegistry::insert(PointIdType id, const DenseVector& vec) {
//     current_appendable_->insert(id, vec);
//     current_size_bytes_ += calculateVectorSize(vec);
    
//     if (current_size_bytes_ >= max_appendable_size_bytes_) {
//         flushToImmutable();
//     }
// }

// void SegmentRegistry::insertBatch(const std::vector<std::pair<PointIdType, DenseVector>>& points) {
//     for (const auto& [id, vec] : points) {
//         insert(id, vec);
//     }
// }

// std::vector<std::pair<PointIdType, float>> SegmentRegistry::search(const DenseVector& query, int top_k) const {
//     return holder_.search(query, top_k);
// }

// // Private implementations
// void SegmentRegistry::createNewAppendableSegment() {
//     current_appendable_ = std::make_shared<AppendableSegment>(dim_, metric_);
//     current_size_bytes_ = 0;
// }

// void SegmentRegistry::flushToImmutable() {
//     if (current_size_bytes_ == 0) return;
    
//     auto immutable = current_appendable_->convertToImmutable();
//     holder_.addSegment(immutable);
//     createNewAppendableSegment();
// }

// size_t SegmentRegistry::calculateVectorSize(const DenseVector& vec) const {
//     return sizeof(float) * vec.size();
// }

// } // namespace vectordb