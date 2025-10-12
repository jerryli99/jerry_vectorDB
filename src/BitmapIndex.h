/*
BitmapIndex indexing for mark delete and pre-filtering, and 
multi-tenancy (store multiple users’ data in the same collection and 
filter queries by user_id (or tenant_id).)

Multi-tenancy = multiple users (tenants) share the same collection and storage.
Each point has a tenant/user ID in its payload.
Pre-filtering uses an inverted index (or BitmapIndex) to quickly select only the points belonging to that user.
Then the filtered internal IDs are passed to HNSW (or brute-force) for vector search.
still thinking: authentication/authorization ensures users can only query their own data.

ID:       0 1 2 3 4 5 6 7 8 9
Bitmap:   0 1 0 1 0 0 1 0 0 1

Now if query also requires category="image", that BitmapIndex might be:

ID:       0 1 2 3 4 5 6 7 8 9
Bitmap:   0 1 1 0 0 0 1 0 0 0

Intersection = bitwise AND:
ID:       0 1 2 3 4 5 6 7 8 9
result    0 1 0 0 0 0 1 0 0 0 = candidate IDs {1, 6}
*/
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstdint>

namespace vectordb {

class BitmapIndex {
public:
    BitmapIndex() = default;
    ~BitmapIndex() = default;
    
    BitmapIndex(size_t size = 0) : 
        m_bits{(size + 63) / 64, 0}, 
        m_size{size} {/*constructor body*/}

    void resize(size_t new_size) {
        m_bits.resize((new_size + 63) / 64, 0);
        m_size = new_size;
    }

    void set(size_t pos, bool value = true) {
        if (pos >= m_size) throw std::out_of_range("BitmapIndex::set out of range");
        size_t idx = pos / 64;
        size_t offset = pos % 64;
        if (value) {
            m_bits[idx] |= (1ULL << offset);
        } else {
            m_bits[idx] &= ~(1ULL << offset);
        }
    }

    bool get(size_t pos) const {
        if (pos >= m_size) throw std::out_of_range("BitmapIndex::get out of range");
        size_t idx = pos / 64;
        size_t offset = pos % 64;
        return (m_bits[idx] >> offset) & 1ULL;
    }

    //set everything in the bitmap to zero
    void clear() {
        std::fill(m_bits.begin(), m_bits.end(), 0);
    }

    size_t size() const { return m_size; }

    // Bitwise AND (intersection)
    BitmapIndex operator&(const BitmapIndex& other) const {
        if (m_size != other.m_size) throw std::invalid_argument("BitmapIndex sizes must match");
        BitmapIndex result(m_size);
        for (size_t i = 0; i < m_bits.size(); i++) {
            result.m_bits[i] = m_bits[i] & other.m_bits[i];
        }
        return result;
    }

    // Bitwise OR (union)
    BitmapIndex operator|(const BitmapIndex& other) const {
        if (m_size != other.m_size) throw std::invalid_argument("BitmapIndex sizes must match");
        BitmapIndex result(m_size);
        for (size_t i = 0; i < m_bits.size(); i++) {
            result.m_bits[i] = m_bits[i] | other.m_bits[i];
        }
        return result;
    }

    // Return list of active IDs
    std::vector<size_t> to_ids() const {
        std::vector<size_t> ids;
        for (size_t i = 0; i < m_size; i++) {
            if (get(i) == true) {
                ids.push_back(i);
            }
        }
        return ids;
    }

    std::string debugString(size_t limit = 64) const {
        std::ostringstream oss;
        for (size_t i = 0; i < std::min(m_size, limit); i++) {
            oss << (get(i) ? "1 " : "0 ");
        }
        return oss.str();
    }

private:
    std::vector<uint64_t> m_bits;
    size_t m_size;
};

} // namespace vectordb