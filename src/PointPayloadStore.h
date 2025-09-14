#pragma once

#include "DataTypes.h"
#include "Status.h"
#include <rocksdb/db.h>
// #include <rocksdb/options.h>

/*
Perhaps each Point has a version, so perhaps different payload versions. 
Well this is too much work, so I might update the code for handling this
if i ever have time.

Note:
I am trying to make Payload per Point storage not per named vector here, so yeah.
*/

namespace vectordb {
    class PointPayloadStore {
    public:

        PointPayloadStore(const std::filesystem::path& db_path, size_t cache_size_mb = 0);
        ~PointPayloadStore();

        // Disable copying
        PointPayloadStore(const PointPayloadStore&) = delete;
        PointPayloadStore& operator=(const PointPayloadStore&) = delete;

        Status putPayload(const PointIdType& id, const Payload& data);
        StatusOr<Payload> getPayload(const PointIdType& id);
        Status deletePayload(const std::string& id);

        // Filter points by metadata field (simple equality)
        //could also be tricky, might need helper member functions for this one
        std::vector<Payload> filterWithPayload(const std::string& metadata_field, const Payload& condition); //?

    private:
        // Payload vec_metadata_;
        rocksdb::DB* m_rkdb;
        std::filesystem::path m_rkdb_path;
        mutable std::mutex m_mutex;
    };

}


/*
So perhaps i can do in my other pp files

auto payload = payload_store.Get(point.point_id);
return EnrichedPoint{point.point_id, point.named_vecs, *payload};

something like that in the code?
add_point(Point p) {
    vector_index.add(p.point_id, p.named_vecs);
    payload_store.Put(p.point_id, payload);
}

*/