#pragma once

#include "DataTypes.h"
#include "Segment.h"

#include <shared_mutex>
#include <thread>
#include <mutex>

namespace vectordb {

    struct SegmentEntry {
        std::shared_ptr<Segment> segment;//?
        SegmentType type;
    };

}

//might add something else in the future here.