#pragma once

#include "WalEntry.h"
#include <mutex>

/*
Write ahead log for the vectors. The payloads are handled by rocksdb, so don't worry about them
WAL idea is about persist before it’s acknowledged.
I probably will not implement the full recovery of data, we will see.

WAL in my understanding is before i add vectors to memory pool, need to append them to the WAL file.
Insert flow:
User calls insertPoint() etc
You serialize the insert operation to the WAL (append to file).
Then you apply the insert operation to the memory pool / active segment.
If crash happens after WAL but before memory, replay restores it.
If crash happens after memory but before WAL, replay re-applies it (idempotent, you overwrite same ID).

Replay:
On startup, you open the WAL.
For each entry, reconstruct Point and put it back into the active segment (or memory pool).
Then resume as if nothing happened.


| checksum | entry_type | collection_name | point_id | named_vec_count | [[vec_name, dim, data], [vec_name, dim, data], ...] |

entry type meaning insert, delete, update, etc. In my case for now, just do insert.
*/

namespace vectordb {
    class WAL {
        public:
            WAL() = default;
            WAL(const std::filesystem::path& path) : m_wal_path{path} {}
            ~WAL() = default;
            
            void append(std::atomic<int64_t> id, const std::vector<Point>& vectors);//need to serialize the data here?
            std::vector<WalEntry> replay(...);//??
            void truncate(WalTruncateMode mode = WalTruncateMode::FULL, size_t keep_last_n = 0);//decide how to truncate the wal file 
            void newAppend(...);//??

        private:
            std::filesystem::path m_wal_path;
            mutable std::shared_mutex m_mutex;
            uint32_t checksum(...);//?? 
    };
}