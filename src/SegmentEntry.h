#pragma once

#include "DataTypes.h"
#include "Segment.h"

#include <shared_mutex>
#include <thread>
#include <mutex>

namespace vectordb {

    struct SegmentEntry {
        std::shared_ptr<Segment> segment_;// and then in segment there is segment id
        SegmentType type_;
    };

}

//might add something else in the future here.