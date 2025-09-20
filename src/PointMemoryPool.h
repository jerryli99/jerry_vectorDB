#pragma once

#include "DataTypes.h"
#include "Point.h"
#include <vector>
#include <memory>
#include <mutex>
#include <cstddef>

namespace vectordb {

template<size_t TinyMapCapacity>//for Point obj's configurable number of named vectors storage
class PointMemoryPool {
public:
    explicit PointMemoryPool(size_t max_points)
        : m_max_points{max_points}
        , m_total_allocated{0}
        , m_buffer{std::make_unique<std::byte[]>(max_points * sizeof(Point<TinyMapCapacity>))}
        , m_current{m_buffer.get()}
        , m_bitmap{max_points, false}
    {
        m_free_list.reserve(max_points);
    }

    ~PointMemoryPool() {
        clear();
    }

    PointMemoryPool(const PointMemoryPool&) = delete;
    PointMemoryPool& operator=(const PointMemoryPool&) = delete;

    PointMemoryPool(PointMemoryPool&& other) noexcept
        : m_max_points{other.m_max_points}
        , m_total_allocated{other.m_total_allocated}
        , m_buffer{std::move(other.m_buffer)}
        , m_current{other.m_current}
        , m_free_list{std::move(other.m_free_list)}
        , m_bitmap{std::move(other.m_bitmap)}
    {
        other.m_max_points = 0;
        other.m_total_allocated = 0;
        other.m_current = nullptr;
    }

    PointMemoryPool& operator=(PointMemoryPool&& other) noexcept {
        if (this != &other) {
            clear();

            m_max_points = other.m_max_points;
            m_total_allocated = other.m_total_allocated;
            m_buffer = std::move(other.m_buffer);
            m_current = other.m_current;
            m_free_list = std::move(other.m_free_list);
            m_bitmap = std::move(other.m_bitmap);

            other.m_max_points = 0;
            other.m_total_allocated = 0;
            other.m_current = nullptr;
        }
        return *this;
    }

    StatusOr<Point<TinyMapCapacity>*> allocatePoint(const PointIdType& point_id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_total_allocated >= m_max_points) {
            return Status::Error("Active segment is full");
        }

        Point<TinyMapCapacity>* point = nullptr;
        size_t slot_index;

        if (!m_free_list.empty()) {
            auto* memory = m_free_list.back();
            m_free_list.pop_back();
            slot_index = getSlotIndex(memory);
            point = new (memory) Point<TinyMapCapacity>(point_id);
        } else if (m_current < m_buffer.get() + m_max_points * sizeof(Point<TinyMapCapacity>)) {
            auto* memory = m_current;
            slot_index = getSlotIndex(memory);
            m_current += sizeof(Point<TinyMapCapacity>);
            point = new (memory) Point<TinyMapCapacity>(point_id);
        } else {
            return Status::Error("Out of memory in pool");
        }

        m_bitmap[slot_index] = true;
        m_total_allocated++;
        return point;
    }

    void deallocatePoint(Point<TinyMapCapacity>* point) {
        if (!point) return;

        std::lock_guard<std::mutex> lock(m_mutex);

        size_t slot_index = getSlotIndex(reinterpret_cast<std::byte*>(point));
        if (!m_bitmap[slot_index]) return; // already freed

        point->~Point<TinyMapCapacity>();
        m_free_list.push_back(reinterpret_cast<std::byte*>(point));
        m_bitmap[slot_index] = false;
        m_total_allocated--;
    }

    std::vector<Point<TinyMapCapacity>*> getAllPoints() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<Point<TinyMapCapacity>*> result;
        result.reserve(m_total_allocated);

        for (size_t i = 0; i < m_max_points; i++) {
            if (m_bitmap[i]) {
                auto* point = reinterpret_cast<Point<TinyMapCapacity>*>(
                    m_buffer.get() + i * sizeof(Point<TinyMapCapacity>)
                );
                result.push_back(point);
            }
        }

        return result;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Destroy all allocated points
        for (size_t i = 0; i < m_max_points; i++) {
            if (m_bitmap[i]) {
                auto* point = reinterpret_cast<Point<TinyMapCapacity>*>(
                    m_buffer.get() + i * sizeof(Point<TinyMapCapacity>)
                );
                point->~Point<TinyMapCapacity>();
                m_bitmap[i] = false;
            }
        }

        m_free_list.clear();
        m_current = m_buffer.get();
        m_total_allocated = 0;
    }

    size_t getTotalAllocated() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_total_allocated; 
    }
    
    size_t getMaxCapacity() const { return m_max_points; }

    size_t getFreeSlots() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (m_max_points - m_total_allocated);
    }

    bool containsPoint(const Point<TinyMapCapacity>* point) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t slot_index = getSlotIndex(reinterpret_cast<const std::byte*>(point));
        return slot_index < m_max_points && m_bitmap[slot_index];
    }

private:
    size_t getSlotIndex(const std::byte* memory) const {
        return (memory - m_buffer.get()) / sizeof(Point<TinyMapCapacity>);
    }

    size_t m_max_points;
    size_t m_total_allocated;
    mutable std::mutex m_mutex;
    
    std::unique_ptr<std::byte[]> m_buffer;
    std::byte* m_current;
    std::vector<std::byte*> m_free_list;
    std::vector<bool> m_bitmap; //slot usage tracker, size of bool is 1 byte
};

} // namespace vectordb
