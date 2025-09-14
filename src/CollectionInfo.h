#pragma once

#include "DataTypes.h"

namespace vectordb {

//since i allow namedvectors, then a collection can have multiple dims of the same data
//i am not sure if this design is good or not, but limiting it is fine i guess. 
//reference link https://milvus.io/docs/hnsw.md
struct VectorSpec {
    size_t dim;
    DistanceMetric metric;
    bool on_disk = false;
    //bool is_sharded = false;//Uhm, maybe use this after i got the first version of the db working
    //shard_key...
    //create_timestamp...
};

struct IndexSpec {
    //need to add below fields in python client in Collection creation request, now just use default value.
    size_t index_threshold = 4000; //?? i might change this
    bool wait_indexing = true; //wait for the vectors to be indexed before returning, instead of searching buffers linearly.
    size_t m_edges = 64;//Maximum number of connections (or edges) each node can have in the graph at each level.
    size_t ef_construction = 360; //The number of candidates considered during index construction.
    size_t ef_search = 20; //The number of neighbors evaluated during a search. Should be at least as large as Top K.
};

//The name CollectionInfo is vague here for sure, like is it a schema or something metadata?
//well i am still learning DB implementations and terminologies, so i will figure it out later.
struct CollectionInfo {
    CollectionId name;//i think i will still keep the name here just in case i need it for something else.
    // CollectionStatus status;  // e.g., Loaded, Unloaded, Building
    std::map<VectorName, VectorSpec> vec_specs; //vector specifications, lol not sure if this is a good name
    IndexSpec index_specs;
};

}