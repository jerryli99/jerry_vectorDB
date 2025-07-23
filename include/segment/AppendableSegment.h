// #pragma once

// #include "DataTypes.h"
// #include "IdTracker.h"
// #include "Segment.h"
// #include "PlainVectorIndex.h"

// #pragma once

// #include "DataTypes.h"
// #include "IdTracker.h"
// #include "Segment.h"
// #include "PlainVectorIndex.h"
// #include "ImmutableSegment.h"  // Include the immutable segment definition

// namespace vectordb {

// class AppendableSegment : public Segment {
// public:
//     AppendableSegment(size_t dim, DistanceMetric metric)
//         : id_tracker_(std::make_shared<IdTracker>()),
//           vector_index_(dim, metric),
//           dim_(dim),
//           metric_(metric) {}

//     bool isAppendable() const override { return true; }

//     void insertPoint(PointIdType id, const DenseVector& vec) override {
//         auto offset = id_tracker_->insert(id);
//         vector_index_.addPoint(vec, offset);
//     }

//     std::vector<std::pair<size_t, float>> search(const DenseVector& query, int top_k) const override {
//         return vector_index_.search(query, top_k);
//     }

//     std::shared_ptr<IdTracker> getIdTracker() const override {
//         return id_tracker_;
//     }

//     // Convert to immutable segment with HNSW index
//     std::shared_ptr<ImmutableSegment> convertToImmutable() {
//         // Create new immutable segment with appropriate capacity
//         size_t num_points = id_tracker_->size();
//         auto immutable = std::make_shared<ImmutableSegment>(dim_, metric_, num_points);
        
//         // Transfer all points to the immutable segment
//         const auto& points = vector_index_.getAllPoints();
//         const auto& ids = id_tracker_->getAllIds();
        
//         for (size_t i = 0; i < points.size(); i++) {
//             immutable->insertPoint(ids[i], points[i]);
//         }
        
//         return immutable;
//     }

// private:
//     std::shared_ptr<IdTracker> id_tracker_;
//     PlainVectorIndex vector_index_;
//     size_t dim_;
//     DistanceMetric metric_;
// };

// } // namespace vectordb
