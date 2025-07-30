#pragma once

#include "DataTypes.h"
#include "NamedVectors.h"
#include "PointPayloadStore.h"
/*
A point can also have versions, but for now just ignore it.
std::string version?
getVersion()

*/
namespace vectordb {

    struct Point {
        PointIdType point_id;
        NamedVectors named_vecs;
        //in DataTypes.h have using Version = std::string;
        //std::vector<std::pair<Version, NamedVectors>> vectors;
    };
}