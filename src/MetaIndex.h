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