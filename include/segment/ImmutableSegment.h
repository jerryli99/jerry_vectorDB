// #pragma once

// #include "DataTypes.h"
// #include "Segment.h"
// #include "HnswVectorIndex.h"


// namespace vectordb {

// class ImmutableSegment : public Segment {
// public:
//     ImmutableSegment(size_t dim, DistanceMetric metric,
//                      size_t max_elements)
//         : id_tracker_(std::make_shared<IdTracker>()),
//           vector_index_(dim, metric, max_elements) {}

//     bool isAppendable() const override { return false; }

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
    

// private:
//     std::shared_ptr<IdTracker> id_tracker_;
//     HnswVectorIndex vector_index_;
// };

// } // namespace vectordb
