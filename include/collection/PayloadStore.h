#pragma once

#include "DataTypes.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include "Point.h"             // for PointIdType


namespace vectordb {

class PayloadStore {
public:
    PayloadStore(const std::string& db_path) {
        rocksdb::Options options;
        options.create_if_missing = true;

        rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
        }
    }

    ~PayloadStore() {
        delete db_;
    }

    //?????? param??
    void save(const PointIdType& id, const Payload& payload) {
        Payload j = payload;  // convert Payload → json
        std::string value = j.dump();

        rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), id, value);
        if (!s.ok()) {
            throw std::runtime_error("RocksDB save failed: " + s.ToString());
        }
    }

    std::optional<Payload> load(const PointIdType& id) const {
        std::string value;
        rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), id, &value);
        if (!s.ok()) {
            return std::nullopt;
        }

        Payload p = json::parse(value);
        return p;
    }

    void remove(const PointIdType& id) {
        db_->Delete(rocksdb::WriteOptions(), id);
    }

private:
    rocksdb::DB* db_;
};

}
