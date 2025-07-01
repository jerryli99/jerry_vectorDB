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
private:
    std::array<std::pair<K, V>, N> data_;
    std::size_t size_ = 0;

public:
    TinyMap() = default;

    std::size_t size() const {
        return size_;
    }

    bool empty() const {
        return (size_ == 0);
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
    std::optional<std::reference_wrapper<const V>> get(const K& key) const {
        for (std::size_t i = 0; i < size_; ++i) {
            if (data_[i].first == key) {
                return std::cref(data_[i].second);  // wrap const reference
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
        for (std::size_t i = 0; i < size_; ++i) {
            if (data_[i].first == key) {
                data_[i].second = value;  // Update existing key
                return true;
            }
        }

        if (size_ >= N) {
            return false;  // Map full
        }

        data_[size_++] = std::make_pair(key, value);
        
        return true;
    }


    bool erase(const K& key) {
        for (std::size_t i = 0; i < size_; ++i) {
            if (data_[i].first == key) {
                data_[i] = data_[--size_];  // Swap with last and shrink
                return true;
            }
        }

        return false;
    }

    void clear() {
        size_ = 0;
    }

    /*
    The begin() and end() functions here are for better iteration like:
    
    for (const auto& [key, value] : map) {
    std::cout << key << ": " << value << "\n";
    }

    */
    const std::pair<K, V>* begin() const { 
        return data_.data(); 
    }
    
    const std::pair<K, V>* end() const { 
        return data_.data() + size_; 
    }
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