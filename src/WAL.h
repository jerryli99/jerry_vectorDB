#pragma once

#include "WalEntry.h"

/*
Write ahead log. I will implement this later.
*/

namespace vectordb {
    class WAL {
        public:
            WAL() = default;
            WAL(const std::filesystem::path& path) : m_wal_path{path} {}
            ~WAL() = default;
            
            void append(int64_t id, const AppendableStorage& vectors);
            std::vector<WalEntry> replay(...);//??maybe user can specify with http request to replay last or all ops..
            void truncate(WalTruncateMode mode = WalTruncateMode::FULL, size_t keep_last_n = 0);//user can decide how to truncate the wal file 
        
        private:
            std::filesystem::path m_wal_path;
            uint32_t checksum(...);//?? 
    };
}