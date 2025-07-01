#pragma once

#include "DataTypes.h"
#include "Point.h"


namespace vectordb {

    struct Segment {
        SegmentId segment_id;       
        std::unordered_map<PointId, Point> points;
        
        void addPoint(const Point& point);
        std::vector<PointId> search(const std::string& vector_name, const DenseVector& query, int top_k) const;

    };
}