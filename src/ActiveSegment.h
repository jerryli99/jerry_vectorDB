#pragma once

#include "DataTypes.h"
#include "Status.h"
#include "Point.h"
#include "PointMemoryPool.h"
#include "CollectionInfo.h"
#include "ImmutableSegment.h"

#include <memory>
#include <mutex>
#include <functional>
#include <map>

namespace vectordb {

class ActiveSegment {
public:
    ActiveSegment(size_t max_capacity, const IndexSpec& index_spec)
        : m_pool{std::make_unique<PointMemoryPool>(max_capacity)}
        , m_index_spec{index_spec}
        , m_max_capacity{max_capacity}
    {}

    ~ActiveSegment() = default;

    //prevent accidental copies
    ActiveSegment(const ActiveSegment&) = delete;
    ActiveSegment& operator=(const ActiveSegment&) = delete;

    //do allow moves though
    ActiveSegment(ActiveSegment&&) noexcept = default;
    ActiveSegment& operator=(ActiveSegment&&) noexcept = default;

    //insert single unnamed vector
    Status insertPoint(PointIdType point_id, const DenseVector& vector) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "Hello from activeSegment inserting 1 vector\n";
        auto* point = m_pool->allocatePoint(point_id);
        if (!point) {
            return Status::Error("Active segment is full");
        }

        if (!point->addVector("default", vector)) {
            m_pool->deallocatePoint(point);
            return Status::Error("Failed to add vector to point");
        }

        return Status::OK();
    }

    //insert multiple named vectors
    Status insertPoint(PointIdType point_id,
                       const std::map<VectorName, DenseVector>& named_vectors) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "Hello from activeSegment inserting multi namedvectors\n";
        auto* point = m_pool->allocatePoint(point_id);
        if (!point) {
            return Status::Error("Active segment is full");
        }

        for (const auto& [name, vec] : named_vectors) {
            if (!point->addVector(name, vec)) {
                m_pool->deallocatePoint(point);
                return Status::Error("Too many named vectors for TinyMap capacity");
            }
        }

        return Status::OK();
    }

    //Check if indexing threshold is reached
    bool shouldIndex() const {
        return getPointCount() >= m_index_spec.index_threshold;
    }

    //Check if segment is full
    bool isFull() const {
        return getPointCount() >= m_max_capacity;
    }

    //Get a copy of the data from this activeSegment obj and put them in immutableSegment for index building, then clearPool 
    //for the next incoming vector data. I choose to copy to keep it safe and simple...i will optimize in future.
    StatusOr<std::unique_ptr<ImmutableSegment>> convertToImmutable() {
        std::lock_guard<std::mutex> lock(m_mutex);

        //Extract data safely by copying
        SegPointData point_data;
        // Lock only as long as needed for extracting data, then unlock before doing heavier stuff
        auto points = m_pool->getAllPoints();
        if (points.empty()) {
            return Status::Error("No points to convert");
        }

        point_data.reserve(points.size());
        for (const auto* point : points) {
            point_data.emplace_back(point->getId(), point->getAllVectors());
        }

        try {
            auto immutable_segment = std::make_unique<ImmutableSegment>(point_data, m_index_spec);
            
            //CLEAR THE POOL AFTER SUCCESSFUL CONVERSION
            m_pool->clearPool();

            return immutable_segment;

        } catch (const std::exception& e) {
            return Status::Error(std::string("ImmutableSegment creation failed: ") + e.what());
        }
    }


    // Get all points for conversion to immutable segment
    std::vector<Point*> getAllPoints() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool->getAllPoints();
    }

    // Get statistics about the segment
    size_t getPointCount() const {
        return m_pool->getTotalAllocated();
    }

    size_t getMaxCapacity() const {
        return m_max_capacity;
    }

    const IndexSpec& getIndexSpec() const {
        return m_index_spec;
    }

    // size_t getTinyMapCapacity() const {
    //     return TinyMapCapacity;
    // }

private:
    SegmentType seg_type{SegmentType::Appendable};
    std::unique_ptr<PointMemoryPool> m_pool;
    IndexSpec m_index_spec;
    size_t m_max_capacity;
    mutable std::mutex m_mutex;
};

} // namespace vectordb
