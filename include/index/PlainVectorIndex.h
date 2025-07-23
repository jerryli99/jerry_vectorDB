#pragma once

#include "../DataTypes.h"
#include "../segment/Point.h"

#include <Eigen/Dense>
#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>


/*
So when we add the point, i think i will skip the id tracker here since
we just use std::vector to store the points.
*/

namespace vectordb {

class PlainVectorIndex {
public:
    PlainVectorIndex(size_t dim, DistanceMetric metric);

    PlainVectorIndex(const PlainVectorIndex& other);

    //add one or more points to the appendable segment
    void addPoints(const std::vector<Point>& points);

    // Get all stored points
    const std::vector<Point>& getAllPoints() const;
    
    //update? or no
    //update()

    //delete is like maybe use bitmap to mark the elements?
    //but never remove from memory? or maybe after a while?
    //markDelete();

    std::vector<std::vector<std::pair<PointIdType, float>>> search(
        const std::vector<DenseVector>& queries,
        int top_k,
        const std::string& vector_name = "default") const;


private:
    size_t dim_;
    DistanceMetric metric_;
    std::vector<Point> points_;  // Store complete Point objects
    IdTracker id_tracker_; 

    // Helper to get vectors by name, UHmmm??? maybe ignore this for now.
    // std::vector<std::pair<size_t, DenseVector>> getVectorsByName(
    //     const std::string& vector_name) const;
    
    float getDistance(const DenseVector& a, const DenseVector& b) const;
};

} // namespace vectordb
