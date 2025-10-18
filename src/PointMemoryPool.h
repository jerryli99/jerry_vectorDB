#pragma once

#include "DataTypes.h"
#include "Point.h"
#include <vector>
#include <memory>
#include <mutex>
#include <cstddef>
#include <type_traits>

namespace vectordb {

class PointMemoryPool {
public:
    explicit PointMemoryPool(size_t max_points = 5000)
        : m_max_points{max_points}
        , m_total_allocated{0}
        , m_buffer{std::make_unique<PointStorage[]>(max_points)}
        , m_current{m_buffer.get()}
        , m_occupied(max_points, false) // Use bool instead of char
    {
        m_free_list.reserve(max_points);
    }

    ~PointMemoryPool() {
        clearPool();
    }

    PointMemoryPool(const PointMemoryPool&) = delete;
    PointMemoryPool& operator=(const PointMemoryPool&) = delete;

    PointMemoryPool(PointMemoryPool&& other) noexcept
        : m_max_points{other.m_max_points}
        , m_total_allocated{other.m_total_allocated}
        , m_buffer{std::move(other.m_buffer)}
        , m_current{other.m_current}
        , m_free_list{std::move(other.m_free_list)}
        , m_occupied{std::move(other.m_occupied)}
    {
        other.m_max_points = 0;
        other.m_total_allocated = 0;
        other.m_current = nullptr;
    }

    PointMemoryPool& operator=(PointMemoryPool&& other) noexcept {
        if (this != &other) {
            clearPool();

            m_max_points = other.m_max_points;
            m_total_allocated = other.m_total_allocated;
            m_buffer = std::move(other.m_buffer);
            m_current = other.m_current;
            m_free_list = std::move(other.m_free_list);
            m_occupied = std::move(other.m_occupied);

            other.m_max_points = 0;
            other.m_total_allocated = 0;
            other.m_current = nullptr;
        }
        return *this;
    }

    // Simple pointer return instead of StatusOr
    Point* allocatePoint(const PointIdType& point_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // std::cout << "Hello i am in memory pool\n";
        if (m_total_allocated >= m_max_points) {
            return nullptr;  // Return nullptr instead of StatusOr
        }

        Point* point = nullptr;
        size_t slot_index;

        if (!m_free_list.empty()) {
            auto* memory = m_free_list.back();
            m_free_list.pop_back();
            slot_index = getSlotIndex(memory);
            point = new (memory) Point(point_id);
        } else if (m_current < m_buffer.get() + m_max_points) {
            auto* memory = m_current;
            slot_index = getSlotIndex(memory);
            m_current++;
            point = new (memory) Point(point_id);
        }

        if (point) {
            m_occupied[slot_index] = true;
            ++m_total_allocated;
        }

        return point;  // Returns pointer or nullptr
    }

    void deallocatePoint(Point* point) {
        if (!point) return;

        std::lock_guard<std::mutex> lock(m_mutex);

        auto* storage = reinterpret_cast<PointStorage*>(point);
        size_t slot_index = getSlotIndex(storage);
        if (!m_occupied[slot_index]) return; // already freed

        point->~Point();
        m_free_list.push_back(storage);
        m_occupied[slot_index] = false;
        --m_total_allocated;
    }

    std::vector<Point*> getAllPoints() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<Point*> result;
        result.reserve(m_total_allocated);

        for (size_t i = 0; i < m_max_points; i++) {
            if (m_occupied[i]) {
                auto* point = reinterpret_cast<Point*>(m_buffer.get() + i);
                result.push_back(point);
            }
        }

        return result;
    }

    //we are clearing the objects in the pool, not the entire pool obj.
    void clearPool() {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (size_t i = 0; i < m_max_points; i++) {
            if (m_occupied[i]) {
                auto* point = reinterpret_cast<Point*>(m_buffer.get() + i);
                point->~Point();
                m_occupied[i] = false;
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

    bool containsPoint(const Point* point) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto* storage = reinterpret_cast<const PointStorage*>(point);

        if (storage < m_buffer.get() || storage >= m_buffer.get() + m_max_points) {
            return false;
        }

        size_t slot_index = getSlotIndex(storage);
        return m_occupied[slot_index];
    }

private:
    using PointStorage = std::aligned_storage_t<sizeof(Point), alignof(Point)>;

    size_t getSlotIndex(const PointStorage* storage) const {
        return static_cast<size_t>(storage - m_buffer.get());
    }

    size_t m_max_points;
    size_t m_total_allocated;
    mutable std::mutex m_mutex;
    
    std::unique_ptr<PointStorage[]> m_buffer;
    PointStorage* m_current;
    std::vector<PointStorage*> m_free_list;
    std::vector<bool> m_occupied;
};

} // namespace vectordb

/*
Memory Layout of PointMemoryPool
==========================================================

[PointMemoryPool Object on Stack/Heap]
+-----------------------------------------+
| m_max_points    [size_t]                |
| m_total_allocated [size_t]              |
| m_mutex         [std::mutex]            |
| m_buffer        [unique_ptr] → ───────┐ |
| m_current       [PointStorage*] → ────┼─┘
| m_free_list     [vector<PointStorage*>] |
| m_occupied      [vector<bool>]          |
+-----------------------------------------+
         │
         │ Points to
         ▼
   
[Raw Memory Buffer (m_buffer) - Contiguous Block]
+---+---+---+---+---+---+---+---+---+---+
│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ ... │ N-1    │ ← Slot indices
+---+---+---+---+---+---+---+---+---+---+
│   │   │   │   │   │   │     │        │ ← PointStorage (aligned memory)
+---+---+---+---+---+---+---+---+---+---+
  │   │   │       │           │
  │   │   │       │           │
  │   │   │       │           │
  ▼   ▼   ▼       ▼           ▼

[Individual Point Objects Constructed in Place]
+-------------+-------------+-------------+-------------+
│ Point obj   │ Point obj   │ Point obj   │ ...         │
│ slot 0      │ slot 1      │ slot 2      │             │
│             │             │             │             │
│ point_id    │ point_id    │ point_id    │             │
│ named_vecs  │ named_vecs  │ named_vecs  │             │
│ m_mutex     │ m_mutex     │ m_mutex     │             │
+-------------+-------------+-------------+-------------+

[m_occupied Bit Vector - Tracks Used Slots]
+---+---+---+---+---+---+---+---+---+---+
│ 1 │ 1 │ 1 │ 0 │ 0 │ 1 │ ... │ 0 │ 1 │ ← true/false for each slot
+---+---+---+---+---+---+---+---+---+---+
  │   │   │   │   │   │       │   │
  │   │   │   │   │   │       │   │
Used Used Used Free Free Used     Used

[m_free_list - Vector of Available Slot Pointers]
+-------------------------+
│ → slot 3               │
│ → slot 4               │
│ → slot N-2             │
│ ...                    │
+-------------------------+

[m_current Pointer Progression]
Initially: m_current → slot 0
After 3 allocs: m_current → slot 3
After free(slot 1): m_free_list gains → slot 1
Next alloc: uses slot 1 from free_list
When free_list empty: continues from m_current

Initial State:
Slots:    [0][1][2][3][4][5]
Occupied: [0][0][0][0][0][0]
FreeList: []
Current: →[0]

After alloc P1, P2, P3:
Slots:    [P1][P2][P3][ ][ ][ ]
Occupied: [1][1][1][0][0][0]
FreeList: []
Current: →[3]

After dealloc P2:
Slots:    [P1][  ][P3][ ][ ][ ]
Occupied: [1][0][1][0][0][0]
FreeList: [→slot1]
Current: →[3]

After alloc P4:
Slots:    [P1][P4][P3][ ][ ][ ]
Occupied: [1][1][1][0][0][0]
FreeList: []
Current: →[3]  (unchanged - reused from free_list)

ALIGNED MEMORY POOL (Current Implementation)
============================================

[Memory Pool Buffer - Perfectly Aligned Slots]
+-------------+-------------+-------------+-------------+
| Slot 0      | Slot 1      | Slot 2      | Slot 3      |
| 0x1000      | 0x1040      | 0x1080      | 0x10C0      |
|             |             |             |             |
| [Point A]   | [Point B]   | [FREE]      | [Point D]   |
| point_id=1  | point_id=2  |             | point_id=4  |
| vectors...  | vectors...  |             | vectors...  |
| mutex...    | mutex...    |             | mutex...    |
+-------------+-------------+-------------+-------------+
     │              │                         │
     │              │                         │
Occupied        Occupied                    Occupied

UNALIGNED MEMORY POOL (If We Didn't Use aligned_storage)
========================================================

[Memory Pool Buffer - Misaligned Mess]
+-----------+-----------+-----------+-----------+-----------+
|Slot 0     |Slot 1     |Slot 2     |Slot 3     |Slot 4     |
|0x1000     |0x1038     |0x1070     |0x10A8     |0x10E0     |
|           |           |           |           |           |
|[Point A]  |[Point B]  |[Point C]  |[FREE]     |[Point E]  |
|point_id=1 |point_id=2 |point_id=3 |           |point_id=5 |
|vectors... |vectors... |vectors... |           |vectors... |
|mutex...   |mutex...   |mutex...   |           |mutex...   |
+-----------+-----------+-----------+-----------+-----------+
     │           │           │                    │
     │           │           │                    │
Starts at    MISALIGNED! MISALIGNED!           MISALIGNED!
proper       (0x1038 % 8   (0x1070 % 8         (0x10E0 % 8  
boundary     != 0)        != 0)               != 0)
*/