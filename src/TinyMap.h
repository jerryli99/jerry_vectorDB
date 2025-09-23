/**
 * @brief So we have the Named Vectors, something like this: 
 * {
 * "id": 42,
 * "vectors": {
 * "image": [...],
 * "text": [...],
 * "meta": [...]
 *   }
 * }
 * 
 *  Well, this will be in a single Point object. If we use a hashmap, it will be 
 *  overkill for this because less than 8 entries is usually needed, which makes 
 *  having a  hash function not useful. Thus, store this NamedVectors on stack is fine.
 */

#pragma once

#include <array>
#include <utility>
#include <optional>
#include <functional>

namespace vectordb {

template<typename K, typename V, std::size_t N>
class TinyMap {
public:
    TinyMap() = default;

    std::size_t size() const {
        return m_size;
    }

    bool empty() const {
        return (m_size == 0);
    }

    /**
     * @brief This get (read-only) function will return a non-modifiable 
     *        reference, i.e., const V&
     *        
     * For example: 
     *        if (auto val = map.get("some_key")) {
     *          const std::vector<float>& vec = val->get();
     *          ...use vec without copying or modifying
     *        }
     */
    std::optional<V> get(const K& key) const {
        for (std::size_t i = 0; i < m_size; ++i) {
            if (m_data[i].first == key) {
                return m_data[i].second;  // return a copy
            }
        }
        return std::nullopt;
    }


    bool contains(const K& key) const {
        return get(key).has_value();
    }

    /**
     * @brief users can update existing keys or add new keys
     */
    bool insert(const K& key, const V& value) {
        for (std::size_t i = 0; i < m_size; ++i) {
            if (m_data[i].first == key) {
                m_data[i].second = value;  // Update existing key
                return true;
            }
        }

        if (m_size >= N) {
            return false;  // Map full
        }

        m_data[m_size++] = std::make_pair(key, value);
        
        return true;
    }

    bool erase(const K& key) {
        for (std::size_t i = 0; i < m_size; ++i) {
            if (m_data[i].first == key) {
                m_data[i] = m_data[--m_size];  // Swap with last and shrink
                return true;
            }
        }

        return false;
    }

    void clear() {
        m_size = 0;
    }

    /*
    The begin() and end() functions here are for better iteration like:
    
    for (const auto& [key, value] : map) {
    std::cout << key << ": " << value << "\n";
    }

    */
    const std::pair<K, V>* begin() const { 
        return m_data.data(); 
    }
    
    const std::pair<K, V>* end() const { 
        return m_data.data() + m_size; 
    }

private:
    std::array<std::pair<K, V>, N> m_data;
    std::size_t m_size = 0;
};

}

/*
Usage:
#include "tinymap.h"
#include <iostream>

int main() {
    TinyMap<std::string, int, 4> map;
    map.insert("apple", 1);
    map.insert("banana", 2);

    if (auto val = map.get("banana"); val) {
        std::cout << "banana = " << *val << '\n';
    }

    for (const auto& [k, v] : map) {
        std::cout << k << ": " << v << '\n';
    }

    map.erase("apple");
}
*/