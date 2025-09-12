#include "AppendableSegment.h"

namespace vectordb {

    Status AppendableSegment::insertPoint(PointIdType point_id, const DenseVector& vector)
    {
        Point<MIN_ENTRIES_TINYMAP> p{point_id};
        if (!p.addVector("default", vector)) {
            return Status::Error("Too many named vectors for TinyMap capacity");
        }

        //dont worry, payload is stored in rocksdb, in a seperate obj
        active_buf.emplace_back(std::move(p));
        return Status::OK();
    } 

    Status AppendableSegment::insertPoint(PointIdType point_id, 
                                          const std::map<VectorName, DenseVector>& named_vectors)
    {
        Point<MAX_ENTRIES_TINYMAP> p(point_id);
        for (const auto& [name, vec] : named_vectors) {
            if (!p.addVector(name, vec)) {
                return Status::Error("Too many named vectors for TinyMap capacity");
            }
        }
        //dont worry, payload will get added in a seperate object...
        active_buf.emplace_back(std::move(p));
        return Status::OK();
    }

    // const AppendableStorage& AppendableSegment::data() const { 
    //         return active_buf; 
    // }

}