#pragma once

#include "DataTypes.h"
#include "Segment.h"

#include <shared_mutex>
#include <thread>
#include <mutex>

namespace vectordb {

    struct SegmentEntry {
        std::shared_ptr<Segment> m_segment;// and then in segment there is segment id
        // SegmentType type_;
        // uint64_t segment_size_;??
        //other segment meta data here...like version for example, or created time..
    };

}

//might add something else in the future here.