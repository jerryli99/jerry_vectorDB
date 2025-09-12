#pragma once

#include "DataTypes.h"

namespace vectordb {

//since i allow namedvectors, then a collection can have multiple dims of the same data
//i am not sure if this design is good or not, but limiting it is fine i guess. 
struct VectorSpec {
    size_t dim;
    DistanceMetric metric;
    bool on_disk = false; 
    //bool is_sharded = false;//Uhm, maybe use this after i got the first version of the db working
    //shard_key...
    //create_timestamp...
};

//The name CollectionInfo is vague here for sure, like is it a schema or something metadata?
//well i am still learning DB implementations and terminologies, so i will figure it out later.
struct CollectionInfo {
    CollectionId name;//i think i will still keep the name here just in case i need it for something else.
    // CollectionStatus status;  // e.g., Loaded, Unloaded, Building
    std::map<VectorName, VectorSpec> vec_specs; //vector specifications, lol not sure if this is a good name
    
};

}

/*
Example:
CollectionInfo collection_info;
collection_info.vec_specs.insert("image", {512, DistanceMetric::Cosine});
collection_info.vec_specs.insert("text", {768, DistanceMetric::DotProduct});

//then we can have something like 
Point p;
p.point_id = 42;
p.named_vecs.insert("image", std::vector<float>(512, 0.1f));
p.named_vecs.insert("text", std::vector<float>(768, 0.2f));

*/