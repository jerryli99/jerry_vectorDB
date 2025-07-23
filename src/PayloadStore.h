// #pragma once

// #include "DataTypes.h"

// #include <rocksdb/db.h>
// #include <rocksdb/options.h>

// namespace vectordb {

// class PayloadStore {
// public:
//     PayloadStore(const std::string& db_path) {
//         rocksdb::Options options;
//         options.create_if_missing = true;

//         rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
//         if (!status.ok()) {
//             throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
//         }
//     } 

//     ~PayloadStore() {
//         delete db_;
//     }

//     void save(const PointIdType& id, const Payload& payload) {
//         std::string key = serializeId(id);
//         std::string value = payload.dump();  // Assuming Payload has a .dump() method

//         rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), key, value);
//         if (!s.ok()) {
//             throw std::runtime_error("RocksDB save failed: " + s.ToString());
//         }
//     }

//     std::optional<Payload> load(const PointIdType& id) const {
//         std::string key = serializeId(id);
//         std::string value;

//         rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &value);
//         if (!s.ok()) {
//             return std::nullopt;
//         }

//         return Payload::parse(value);  // Assuming Payload has a parse method
//     }

//     void remove(const PointIdType& id) {
//         std::string key = serializeId(id);
//         db_->Delete(rocksdb::WriteOptions(), key);
//     }

// private:
//     rocksdb::DB* db_;

//     // Simple serialization for std::variant<string, uint64_t>
//     static std::string serializeId(const PointIdType& id) {
//         if (std::holds_alternative<std::string>(id)) {
//             // Prefix string keys to avoid collisions with uint64_t
//             return "s_" + std::get<std::string>(id);
//         }
//         else if (std::holds_alternative<uint64_t>(id)) {
//             // Store uint64_t in big-endian format for proper ordering
//             uint64_t num = std::get<uint64_t>(id);
//             uint64_t big_endian = __builtin_bswap64(num);  // or use htobe64
//             return std::string(reinterpret_cast<const char*>(&big_endian), sizeof(uint64_t));
//         }
//         else {
//             throw std::runtime_error("Unsupported PointIdType variant (rocksdb payload code error)");
//         }
//     }
// };

// }