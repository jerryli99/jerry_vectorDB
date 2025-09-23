#include "PointPayloadStore.h"

#include <rocksdb/cache.h>
#include <rocksdb/table.h>
#include <rocksdb/options.h>
#include <rocksdb/filter_policy.h>
#include <stdexcept>

namespace vectordb {

    PointPayloadStore::PointPayloadStore(const std::filesystem::path& db_path, 
                                         size_t cache_size_mb) : m_rkdb_path{db_path} 
    {
        // Create the directory if it doesn't exist
        std::filesystem::create_directories(db_path.parent_path());
        
        rocksdb::Options options;
        options.create_if_missing = true;
        options.IncreaseParallelism();
        options.OptimizeLevelStyleCompaction();

        // Configure cache if specified
        if (cache_size_mb > 0) {
            auto cache = rocksdb::NewLRUCache(cache_size_mb * 1024 * 1024);
            rocksdb::BlockBasedTableOptions table_options;
            table_options.block_cache = cache;
            table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
            options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
        }

        rocksdb::Status status = rocksdb::DB::Open(options, db_path.string(), &m_rkdb);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
        }
    }

    PointPayloadStore::~PointPayloadStore() {
        delete m_rkdb;
    }

    Status PointPayloadStore::putPayload(const PointIdType& id, const Payload& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        rocksdb::Status status = m_rkdb->Put(rocksdb::WriteOptions(), id, data.dump());
        if (!status.ok()) {
            return Status::Error("Put failed: " + status.ToString());
        }
        return Status::OK();
    }

    StatusOr<Payload> PointPayloadStore::getPayload(const PointIdType& id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::string value;
        rocksdb::Status status = m_rkdb->Get(rocksdb::ReadOptions(), id, &value);
        
        if (status.IsNotFound()) {
            return Status::Error("Not found");
        } else if (!status.ok()) {
            return Status::Error("Get failed: " + status.ToString());
        }

        try {
            return Payload::parse(value);
        } catch (const Payload::exception& e) {
            return Status::Error("Payload parse error: " + std::string(e.what()));
        }
    }

    Status PointPayloadStore::deletePayload(const PointIdType& id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        rocksdb::Status status = m_rkdb->Delete(rocksdb::WriteOptions(), id);
        if (!status.ok()) {
            return Status::Error("Delete failed: " + status.ToString());
        }
        return Status::OK();
    }

}