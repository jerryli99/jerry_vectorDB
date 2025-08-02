#pragma once

#include "DataTypes.h"


namespace vectordb {

struct CollectionInfo {
    CollectionId name;
    size_t vec_dim;
    DistanceMetric metric;
    bool is_sharded;
    CollectionStatus status;  // e.g., Loaded, Unloaded, Building
    //creation time
    //memory or disk usage;
};

}