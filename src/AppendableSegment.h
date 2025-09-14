#include "DataTypes.h"
#include "Status.h"
#include "Point.h"

namespace vectordb {

class AppendableSegment {
public:
    AppendableSegment() = default;
    ~AppendableSegment() = default;

    //prevent accidental copies?? uhmm maybe i guess.
    AppendableSegment(const AppendableSegment&) = delete;
    AppendableSegment& operator=(const AppendableSegment&) = delete;

    //allow moves, uhm also just trying to type out noexcept to get a feel of it.
    AppendableSegment(AppendableSegment&&) noexcept = default;
    AppendableSegment& operator=(AppendableSegment&&) noexcept = default;

    // Single unnamed vector
    Status insertPoint(PointIdType point_id, const DenseVector& vector);

    // Multiple named vectors
    Status insertPoint(PointIdType point_id, const std::map<VectorName, DenseVector>& named_vectors);

    void searchTopK(size_t top_k);
    // const AppendableStorage& data() const;

private:
    //i simply put this seg_type here to distinguish the big buffer after merging 
    //multiple active_bufs which does not belong to any seg type.
    SegmentType seg_type{SegmentType::Appendable};
    std::vector<Point<MAX_ENTRIES_TINYMAP>> active_buf;
};

}