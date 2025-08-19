#pragma once

#include "DataTypes.h"

namespace vectordb {

/*
Uhm, maybe use this after i got the first version of the db working
*/
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