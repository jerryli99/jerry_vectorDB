#include <rocksdb/db.h>
#include <rocksdb/cache.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <chrono>
#include <memory>
#include <random>
#include <filesystem>
#include <vector>
#include <unordered_set>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Data types
using PointID = std::string;
using Payload = json;

class RocksDBBenchmark {
public:
    RocksDBBenchmark(const fs::path& db_path, size_t block_cache_size)
        : db_path_(db_path) {
        
        rocksdb::Options options;
        options.create_if_missing = true;
        options.IncreaseParallelism();
        options.OptimizeLevelStyleCompaction();
        
        if (block_cache_size > 0) {
            auto cache = rocksdb::NewLRUCache(block_cache_size);
            rocksdb::BlockBasedTableOptions table_options;
            table_options.block_cache = cache;
            table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
            options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
        }

        rocksdb::Status status = rocksdb::DB::Open(options, db_path.string(), &db_);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
        }
    }

    ~RocksDBBenchmark() {
        delete db_;
    }

    void insertData(size_t num_points) {
        rocksdb::WriteBatch batch;
        inserted_ids_.clear();
        inserted_ids_.reserve(num_points);
        
        for (size_t i = 0; i < num_points; ++i) {
            PointID id = "point_" + std::to_string(i);
            Payload payload = createPayload(i);
            batch.Put(id, payload.dump());
            inserted_ids_.push_back(id);
        }

        rocksdb::WriteOptions write_options;
        write_options.sync = false;
        rocksdb::Status status = db_->Write(write_options, &batch);
        if (!status.ok()) {
            throw std::runtime_error("Write failed: " + status.ToString());
        }
        num_points_ = num_points;
    }

    double benchmarkReads(size_t num_samples, bool warmup_cache = false) {
        if (inserted_ids_.empty()) {
            throw std::runtime_error("No data available for benchmarking");
        }

        // Create a random sample from the existing IDs
        std::vector<PointID> sample_ids;
        sample_ids.reserve(num_samples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, inserted_ids_.size() - 1);

        for (size_t i = 0; i < num_samples; ++i) {
            sample_ids.push_back(inserted_ids_[dist(gen)]);
        }

        if (warmup_cache) {
            // Warm up the cache by reading all samples first
            for (const auto& id : sample_ids) {
                std::string value;
                db_->Get(rocksdb::ReadOptions(), id, &value);
            }
        }

        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& id : sample_ids) {
            std::string value;
            rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), id, &value);
            if (!status.ok()) {
                // This should never happen since we're sampling from existing IDs
                throw std::runtime_error("Critical error: Missing point that should exist: " + id);
            }
            
            // Validate data
            try {
                auto payload = json::parse(value);
                if (payload["index"].get<size_t>() != std::stoul(id.substr(6))) {
                    throw std::runtime_error("Data corruption detected for " + id);
                }
            } catch (const json::exception& e) {
                throw std::runtime_error("JSON parse error for " + id + ": " + e.what());
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }

private:
    Payload createPayload(size_t index) {
        return {
            {"index", index},
            {"vector", std::vector<float>(128, 0.5f)},  // 128-dim vector
            {"metadata", {
                {"category", index % 2 == 0 ? "animal" : "object"},
                {"color", index % 3 == 0 ? "red" : (index % 3 == 1 ? "blue" : "green")},
                {"size", index % 10},
                {"active", index % 5 != 0}
            }}
        };
    }

    fs::path db_path_;
    rocksdb::DB* db_;
    size_t num_points_ = 0;
    std::vector<PointID> inserted_ids_;  // Track all inserted IDs
};

void runExperiment(const fs::path& base_dir, size_t data_size, size_t cache_size) {
    fs::path cached_db = base_dir / "with_cache";
    fs::path no_cache_db = base_dir / "no_cache";
    
    // Clean up previous runs
    fs::remove_all(cached_db);
    fs::remove_all(no_cache_db);

    // Test with cache
    {
        std::cout << "=== Testing WITH cache (" << cache_size / (1024 * 1024) << "MB) ===\n";
        RocksDBBenchmark db(cached_db, cache_size);
        
        std::cout << "Inserting " << data_size << " points...";
        auto start = std::chrono::high_resolution_clock::now();
        db.insertData(data_size);
        auto duration = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start);
        std::cout << " took " << duration.count() << "s\n";

        // Cold read (cache not warmed)
        std::cout << "Cold read (1000 samples)...";
        double cold_time = db.benchmarkReads(1000, false);
        std::cout << " took " << cold_time << "s (" << (1000 / cold_time) << " reads/s)\n";

        // Warm read (cache warmed)
        std::cout << "Warm read (1000 samples)...";
        double warm_time = db.benchmarkReads(1000, true);
        std::cout << " took " << warm_time << "s (" << (1000 / warm_time) << " reads/s)\n";
    }

    // Test without cache
    {
        std::cout << "\n=== Testing WITHOUT cache ===\n";
        RocksDBBenchmark db(no_cache_db, 0);  // 0 cache size
        
        std::cout << "Inserting " << data_size << " points...";
        auto start = std::chrono::high_resolution_clock::now();
        db.insertData(data_size);
        auto duration = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start);
        std::cout << " took " << duration.count() << "s\n";

        // Read test (always cold)
        std::cout << "Read test (1000 samples)...";
        double read_time = db.benchmarkReads(1000, false);
        std::cout << " took " << read_time << "s (" << (1000 / read_time) << " reads/s)\n";
    }
}

int main() {
    try {
        const fs::path test_dir = "./rocksdb_benchmark";
        const size_t data_size = 100000;  // 100K points
        const size_t cache_size = 128 * 1024 * 1024;  // 128MB cache

        runExperiment(test_dir, data_size, cache_size);
        
        std::cout << "\nBenchmark completed successfully.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

/*
=== Testing WITH cache (128MB) ===
Inserting 100000 points... took 7.14786s
Cold read (1000 samples)... took 0.107964s (9262.32 reads/s)
Warm read (1000 samples)... took 0.105159s (9509.42 reads/s)

=== Testing WITHOUT cache ===
Inserting 100000 points... took 7.28888s
Read test (1000 samples)... took 0.126257s (7920.34 reads/s)

Benchmark completed successfully.
*/