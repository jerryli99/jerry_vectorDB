#include "PointPayloadStore.h"

#include <rocksdb/cache.h>
#include <rocksdb/table.h>
#include <rocksdb/options.h>
#include <rocksdb/filter_policy.h>
#include <stdexcept>

namespace vectordb {

PointPayloadStore::PointPayloadStore(const std::filesystem::path& db_path, size_t cache_size_mb=0)
    : m_rkdb_path{db_path} {
    
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

void PointPayloadStore::putPayload(const PointIdType& id, const Payload& data) {
    rocksdb::Status status = m_rkdb->Put(
        rocksdb::WriteOptions(), 
        id, 
        data.dump()
    );
    
    if (!status.ok()) {
        throw std::runtime_error("Put failed: " + status.ToString());
    }
}

std::optional<Payload> PointPayloadStore::getPayload(const PointIdType& id) {
    std::string value;
    rocksdb::Status status = m_rkdb->Get(rocksdb::ReadOptions(), id, &value);
    
    if (!status.ok()) {
        return std::nullopt;
    }

    try {
        auto metadata = Payload::parse(value);
        return metadata;
    } catch (const Payload::exception& e) {
        throw std::runtime_error("Payload parse error: " + std::string(e.what()));
    }
}

void PointPayloadStore::deletePayload(const PointIdType& id) {
    rocksdb::Status status = m_rkdb->Delete(rocksdb::WriteOptions(), id);
    if (!status.ok()) {
        throw std::runtime_error("Delete failed: " + status.ToString());
    }
}

}