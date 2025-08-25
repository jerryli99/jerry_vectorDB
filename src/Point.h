#pragma once

#include "DataTypes.h"
#include "NamedVectors.h"
#include "PointPayloadStore.h"
/*
A point can also have versions, but for now just ignore it.
std::string version?
getVersion()

Should Point directly contain a PointPayloadStore?
Eh, no.
A Point is just a lightweight piece of data (id + vectors).
PointPayloadStore is a heavyweight database component with its own RocksDB handle, cache, etc.
If you put the store inside Point, every point would try to own/open its own RocksDB store, crazy~~
In a DB, you want a single payload store per segment(I will mostly do this) 
[could also be per collection (shared service), but 
you can flush, compact, or drop an entire segment (including payloads) without touching the rest in a collection.], 
so yeah, speration of concerns I will say. 
*/
namespace vectordb {
    struct Point {
        PointIdType point_id;
        NamedVectors named_vecs;
        //other random data here
    };
}

//oh, multi tenancy? group_id? maybe i can put it in the payloadstore class?