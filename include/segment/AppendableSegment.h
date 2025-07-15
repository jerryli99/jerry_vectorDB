#pragma once

#include "DataTypes.h"
#include "IdTracker.h"
#include "Segment.h"
#include "PlainVectorIndex.h"

namespace vectordb {

class AppendableSegment : public Segment {
public:
    AppendableSegment(size_t dim, DistanceMetric metric)
        : id_tracker_(std::make_shared<IdTracker>()),
          vector_index_(dim, metric) {}

    bool isAppendable() const override { return true; }

    void insertPoint(PointIdType id, const DenseVector& vec) override {
        auto offset = id_tracker_->insert(id);
        vector_index_.addPoint(vec, offset);
    }

    std::vector<std::pair<size_t, float>> search(const DenseVector& query, int top_k) const override {
        return vector_index_.search(query, top_k);
    }

    std::shared_ptr<IdTracker> getIdTracker() const override {
        return id_tracker_;
    }

private:
    std::shared_ptr<IdTracker> id_tracker_;
    PlainVectorIndex vector_index_;
};

} // namespace vectordb
