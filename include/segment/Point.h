#include "DataTypes.h"
#include "NamedVectors.h"

namespace vectordb {
    struct Point {
        PointId point_id;
        NamedVectors named_vectors;
        Payload payload; //stored in disk, only used if query adds the filter option...
    };
}