#include "DataTypes.h"
#include "NamedVectors.h"
#include "Payload.h"

namespace vectordb {
    struct Point {
        PointIdType point_id;
        NamedVectors named_vectors;
        Payload payload; //stored in disk, only used if query adds the filter option...
    };
}

/*
#include "Point.h"

int main() {
    vectordb::Point p;
    p.point_id = 42;

    // Add a string
    p.payload.set("label", std::string("dog"));

    // Add an integer
    p.payload.set("views", int64_t(150));

    // Add a float
    p.payload.set("score", 0.97);

    // Add a boolean
    p.payload.set("verified", true);

    // Add a string array
    p.payload.set("tags", std::vector<std::string>{"ai", "cnn", "nlp"});

    // Print everything
    p.payload.print();
}

*/