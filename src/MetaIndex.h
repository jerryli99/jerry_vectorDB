/**
 * @brief
 * So what is MetaIndex?
 * It is a layer for indexing K-means centroids from each immutable obj.
 * 
 * Why create such layer?
 * Because i don't want to searchTopK on every immutableSegment since some query vectors
 * are not as close to the vectors in immutableSegment, so to reduce the overhead, it is 
 * best to have such layer exist, and also, this layer is good for sharding and range vector queries.
 * I have not finished FilterMatrix.h yet as of (10/16/2025). 
 * 
 * Is this is a good idea?
 * For individual use case, I will say good enough. The sharding layer will be using this MetaIndex as well.
 * The name MetaIndex is used because well, it is indexing the representations of vectors from each segment, 
 * so yeah. 
 */

#pragma once

#include "DataTypes.h"

namespace vectordb {
class MetaIndex {
public:
    using CentroidsType = std::map<VectorName, std::vector<DenseVector>>;

    MetaIndex() = default;
    ~MetaIndex() = default;

    //add
    void insertToMetaIndex(const SegmentIdType seg_id, CentroidsType centroids, size_t immutableSeg_index) {
        m_meta_index[seg_id] = centroids;//??
        m_immutableSeg_tracker[seg_id] = immutableSeg_index;
    }

    //search(important here, still thinking about making this class a shard layer?)


    //perhaps write to disk to like preserve the MetaIndex state?


private:
    std::unordered_map<SegmentIdType, CentroidsType> m_meta_index;
    //for in memory use case, maybe need to store the location about 
    //where the immutableSeg is located? 
    //So we need mapping of segment id and m_immutableSegment[?] index integer?
    std::unordered_map<SegmentIdType, size_t> m_immutableSeg_tracker;
};

}