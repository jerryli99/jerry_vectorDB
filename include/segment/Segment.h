#pragma once

#include "DataTypes.h"
#include "Point.h"
#include "index/IndexHNSW.h"

#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace vectordb {

    struct Segment {
        SegmentIdType segment_id;
        virtual void addPoint(const Point& point) = 0;
        virtual bool isAppendable() const = 0;
        virtual void flush() = 0;
        virtual ~Segment() {}
    };

    struct AppendableSegment : Segment {
        std::unordered_map<PointIdType, size_t> external_to_internal;
        std::vector<PointIdType> internal_to_external;

        std::vector<DenseVector> vectors;
        std::vector<Payload> payloads;

        void addPoint(const Point& point) override {
            // if (external_to_internal.count(point.point_id)) {
            //     // Overwrite existing
            //     size_t internal_id = external_to_internal[point.point_id];
            //     vectors[internal_id] = point.vector;
            //     payloads[internal_id] = point.payload;
            // } else {
            //     size_t internal_id = vectors.size();
            //     external_to_internal[point.point_id] = internal_id;
            //     internal_to_external.push_back(point.point_id);
            //     vectors.push_back(point.vector);
            //     payloads.push_back(point.payload);
            // }
        }

        bool isAppendable() const override { return true; }

        void flush() override {
            // TODO: persist to disk (binary or mmap), or convert to ReadOnlySegment
        }
    };

    struct ReadOnlySegment : Segment {
        IndexHNSW hnsw;

        std::unordered_map<PointIdType, size_t> external_to_internal;
        std::vector<PointIdType> internal_to_external;

        std::vector<DenseVector> vectors;
        std::vector<Payload> payloads;

        std::vector<PointIdType> search(const std::string& vector_name, const DenseVector& query, int top_k) const {
            // auto internal_ids = hnsw.search(query, top_k);
            // std::vector<PointId> result;
            // for (auto internal_id : internal_ids) {
            //     result.push_back(internal_to_external[internal_id]);
            // }
            // return result;
        }

        void addPoint(const Point&) override {
            throw std::runtime_error("ReadOnlySegment does not support insert.");
        }

        bool isAppendable() const override { return false; }

        void flush() override {
            // Already read-only — maybe noop or index compaction
        }
    };

}
