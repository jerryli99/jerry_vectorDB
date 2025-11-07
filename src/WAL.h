#pragma once

#include "Point.h"

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

entry type meaning insert, delete, update, etc.

Not sure if this is correct or not. 
*/
#pragma once

#include "Point.h"
#include "DataTypes.h"
#include "Status.h"

#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>
#include <vector>
#include <string>

namespace vectordb {

enum class WalEntryType : uint8_t {
    INSERT_VECTOR = 0x01,
    DELETE_POINT = 0x02,
    SEGMENT_FLUSH = 0x03  // Marker for segment conversion
};

#pragma pack(push, 1)
struct WalHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t entry_count;
    SegmentIdType segment_id;
    uint32_t checksum;
};

struct WalEntryHeader {
    WalEntryType type;
    uint64_t timestamp;
    uint32_t data_size;
    uint32_t checksum;
};
#pragma pack(pop)

class WAL {
public:
    WAL() = default;
    
    WAL(const std::filesystem::path& base_path, SegmentIdType segment_id) 
        : m_base_path{base_path}
        , m_segment_id{segment_id}
    {
        std::filesystem::create_directories(m_base_path);
        m_current_wal_path = getWalPath(segment_id);
    }
    
    ~WAL() {
        close();
    }

    // Open new WAL file for this segment
    Status open() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (m_is_open) {
            return Status::OK();
        }

        m_fd = ::open(m_current_wal_path.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);//0644 permission code
        if (m_fd == -1) {
            return Status::Error("Failed to open WAL file: " + std::string(strerror(errno)));
        }

        // Check if file is new or existing
        off_t file_size = ::lseek(m_fd, 0, SEEK_END);
        if (file_size == 0) {
            // New file, write header
            if (!writeHeader()) {
                ::close(m_fd);
                m_fd = -1;
                return Status::Error("Failed to write WAL header");
            }
        } else {
            // Existing file, read header to get entry count
            if (!readHeader()) {
                ::close(m_fd);
                m_fd = -1;
                return Status::Error("Failed to read WAL header from existing file");
            }
        }

        m_is_open = true;
        return Status::OK();
    }

    void close() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        if (m_fd != -1) {
            // Don't fsync here to avoid performance hit on every close
            ::close(m_fd);
            m_fd = -1;
        }
        m_is_open = false;
    }

    // Log insertion of a point with named vectors
    Status logInsert(const std::string& collection_name, 
                     PointIdType point_id,
                     const std::map<VectorName, DenseVector>& vectors) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (!m_is_open) {
            return Status::Error("WAL not open");
        }

        try {
            // Serialize data to binary buffer
            std::vector<uint8_t> buffer;
            
            // Collection name (length-prefixed)
            uint32_t coll_name_len = collection_name.size();
            appendToBuffer(buffer, coll_name_len);
            buffer.insert(buffer.end(), collection_name.begin(), collection_name.end());
            
            // Point ID
            uint32_t point_id_len = point_id.size();
            appendToBuffer(buffer, point_id_len);
            buffer.insert(buffer.end(), point_id.begin(), point_id.end());
            
            // Number of vectors
            uint32_t num_vectors = vectors.size();
            appendToBuffer(buffer, num_vectors);
            
            // Each vector: name + vector data
            for (const auto& [vec_name, vec_data] : vectors) {
                // Vector name
                uint32_t vec_name_len = vec_name.size();
                appendToBuffer(buffer, vec_name_len);
                buffer.insert(buffer.end(), vec_name.begin(), vec_name.end());
                
                // Vector data (as raw bytes)
                uint32_t vec_data_bytes = vec_data.size() * sizeof(float);
                appendToBuffer(buffer, vec_data_bytes);
                const uint8_t* float_data = reinterpret_cast<const uint8_t*>(vec_data.data());
                buffer.insert(buffer.end(), float_data, float_data + vec_data_bytes);
            }

            return writeEntry(WalEntryType::INSERT_VECTOR, buffer);
            
        } catch (const std::exception& e) {
            return Status::Error(std::string("Failed to serialize insert entry: ") + e.what());
        }
    }

    // Log deletion of a point
    Status logDelete(const std::string& collection_name, PointIdType point_id) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (!m_is_open) {
            return Status::Error("WAL not open");
        }

        try {
            std::vector<uint8_t> buffer;
            
            // Collection name
            uint32_t coll_name_len = collection_name.size();
            appendToBuffer(buffer, coll_name_len);
            buffer.insert(buffer.end(), collection_name.begin(), collection_name.end());
            
            // Point ID
            uint32_t point_id_len = point_id.size();
            appendToBuffer(buffer, point_id_len);
            buffer.insert(buffer.end(), point_id.begin(), point_id.end());
            
            return writeEntry(WalEntryType::DELETE_POINT, buffer);
            
        } catch (const std::exception& e) {
            return Status::Error(std::string("Failed to serialize delete entry: ") + e.what());
        }
    }

    // Mark that segment is being converted to immutable
    Status logSegmentFlush() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (!m_is_open) {
            return Status::Error("WAL not open");
        }

        WalEntryHeader entry_header;
        entry_header.type = WalEntryType::SEGMENT_FLUSH;
        entry_header.timestamp = getCurrentTimestamp();
        entry_header.data_size = 0;
        entry_header.checksum = computeChecksum(&entry_header, sizeof(entry_header) - sizeof(uint32_t));

        ssize_t written = ::write(m_fd, &entry_header, sizeof(entry_header));
        if (written != sizeof(entry_header)) {
            return Status::Error("Failed to write segment flush marker");
        }

        m_entry_count++;
        updateHeaderEntryCount();
        
        // Force sync to disk for durability
        if (fsync(m_fd) == -1) {
            return Status::Error("Failed to sync WAL after flush marker: " + std::string(strerror(errno)));
        }
        
        ::close(m_fd);
        m_fd = -1;
        m_is_open = false;

        return Status::OK();
    }

    // Sync WAL to disk (for durability)
    Status sync() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (!m_is_open) {
            return Status::Error("WAL not open");
        }

        if (fsync(m_fd) == -1) {
            return Status::Error("Failed to sync WAL: " + std::string(strerror(errno)));
        }
        
        return Status::OK();
    }

    // Get the path to this WAL file
    std::filesystem::path getFilePath() const {
        return m_current_wal_path;
    }

    // Check if WAL can be safely deleted (contains flush marker)
    bool isSegmentFlushed() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        
        // If file is still open, it hasn't been flushed yet
        if (m_is_open) {
            return false;
        }

        // Read the file to check for flush marker
        int fd = ::open(m_current_wal_path.c_str(), O_RDONLY);
        if (fd == -1) {
            return false;
        }

        bool has_flush_marker = false;
        
        // Get file size
        off_t file_size = ::lseek(fd, 0, SEEK_END);
        if (file_size >= static_cast<off_t>(sizeof(WalHeader) + sizeof(WalEntryHeader))) {
            // Read last entry header
            WalEntryHeader last_entry;
            if (::pread(fd, &last_entry, sizeof(last_entry), 
                       file_size - sizeof(WalEntryHeader)) == sizeof(WalEntryHeader)) {
                has_flush_marker = (last_entry.type == WalEntryType::SEGMENT_FLUSH);
            }
        }
        
        ::close(fd);
        return has_flush_marker;
    }

    // Get current entry count
    uint64_t getEntryCount() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_entry_count;
    }

    // Get current WAL file size
    size_t getSize() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (!m_is_open) {
            // File is closed, get size from filesystem
            std::error_code ec;
            auto size = std::filesystem::file_size(m_current_wal_path, ec);
            return ec ? 0 : size;
        }
        
        off_t size = ::lseek(m_fd, 0, SEEK_END);
        return size >= 0 ? static_cast<size_t>(size) : 0;
    }

    // Check if WAL is empty (only has header)
    bool isEmpty() const {
        return getSize() <= sizeof(WalHeader);
    }

private:
    static constexpr uint32_t WAL_MAGIC = 0x57414C31;  // "WAL1"
    static constexpr uint32_t WAL_VERSION = 1;
    
    std::filesystem::path m_base_path;
    std::filesystem::path m_current_wal_path;
    SegmentIdType m_segment_id;
    int m_fd = -1;
    bool m_is_open = false;
    uint64_t m_entry_count = 0;
    mutable std::shared_mutex m_mutex;

    std::filesystem::path getWalPath(SegmentIdType segment_id) const {
        return m_base_path / ("segment_" + segment_id + ".wal");
    }

    bool writeHeader() {
        WalHeader header;
        header.magic = WAL_MAGIC;
        header.version = WAL_VERSION;
        header.entry_count = 0;
        header.segment_id = m_segment_id;
        header.checksum = computeChecksum(&header, sizeof(header) - sizeof(uint32_t));
        
        ssize_t written = ::write(m_fd, &header, sizeof(header));
        return written == sizeof(header);
    }

    bool readHeader() {
        WalHeader header;
        if (::pread(m_fd, &header, sizeof(header), 0) != sizeof(header)) {
            return false;
        }
        
        if (header.magic != WAL_MAGIC) {
            return false;
        }
        
        if (header.version != WAL_VERSION) {
            // In production, you might want version migration logic
            return false;
        }
        
        // Verify checksum
        uint32_t saved_checksum = header.checksum;
        header.checksum = 0;
        if (computeChecksum(&header, sizeof(header)) != saved_checksum) {
            return false;
        }
        
        m_entry_count = header.entry_count;
        return true;
    }

    void updateHeaderEntryCount() {
        WalHeader header;
        if (::pread(m_fd, &header, sizeof(header), 0) != sizeof(header)) {
            return;
        }
        
        header.entry_count = m_entry_count;
        header.checksum = computeChecksum(&header, sizeof(header) - sizeof(uint32_t));
        
        ::pwrite(m_fd, &header, sizeof(header), 0);
    }

    Status writeEntry(WalEntryType type, const std::vector<uint8_t>& data) {
        WalEntryHeader entry_header;
        entry_header.type = type;
        entry_header.timestamp = getCurrentTimestamp();
        entry_header.data_size = data.size();
        entry_header.checksum = 0;  // Will compute after setting data
        
        // Compute entry checksum (header + data)
        uint32_t header_checksum = computeChecksum(&entry_header, sizeof(entry_header) - sizeof(uint32_t));
        uint32_t data_checksum = computeChecksum(data.data(), data.size());
        entry_header.checksum = header_checksum ^ data_checksum;
        
        // Write entry header
        ssize_t written = ::write(m_fd, &entry_header, sizeof(entry_header));
        if (written != sizeof(entry_header)) {
            return Status::Error("Failed to write WAL entry header");
        }
        
        // Write entry data
        if (!data.empty()) {
            written = ::write(m_fd, data.data(), data.size());
            if (written != static_cast<ssize_t>(data.size())) {
                return Status::Error("Failed to write WAL entry data");
            }
        }
        
        // Update header with new entry count
        m_entry_count++;
        updateHeaderEntryCount();
        
        return Status::OK();
    }

    template<typename T>
    void appendToBuffer(std::vector<uint8_t>& buffer, const T& value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
    }

    uint64_t getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }

    uint32_t computeChecksum(const void* data, size_t size) const {
        // Simple XOR-based checksum for prototyping
        // In production, consider CRC32 or a proper hash
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint32_t checksum = 0;
        for (size_t i = 0; i < size; ++i) {
            checksum ^= (static_cast<uint32_t>(bytes[i]) << ((i % 4) * 8));
        }
        return checksum;
    }
};

} // namespace vectordb
