/**
 * @brief 
 * The LRUCache class is indented to be used for query. 
 * But i can modify it for the query vector.
 * So query#1 might be similiar to query#2 vector, like 0.1 difference kind,
 * then why not just do a cache quick query vector difference check to see if
 * the 2 queries or more stored in the LRU cache are similiar, and if so, then
 * simply retrieve it instead of adding a new one to the list?
 * 
 */
#pragma once

#include <list>
#include <unordered_map>
#include <mutex>
#include <optional>

template <typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : m_capacity(capacity) {}

    //Insert or update a key-value pair, and make it the most recently used.
    void put(const Key& key, Value value) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_map.find(key);
        if (it != m_map.end()) {
            m_list.erase(it->second);
            m_map.erase(it);
        }

        m_list.emplace_front(key, std::move(value));
        m_map[key] = m_list.begin();//Update the hash map to point to that new list node.

        //If we exceed capacity: Find the last element (end() - 1 = least recently used). 
        //Remove it from both m_map and m_list.
        if (m_map.size() > m_capacity) {
            auto last = m_list.end();
            --last;
            m_map.erase(last->first);
            m_list.pop_back();
        }
    }

    //Retrieve value if present, and mark it as most recently used.
    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_map.find(key);
        if (it == m_map.end()) return std::nullopt;

        // Move to front (most recently used)
        // splice is O(1) since it just changes pointers in the linked list.
        m_list.splice(m_list.begin(), m_list, it->second);
        return it->second->second;
    }

    bool contains(const Key& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.find(key) != m_map.end();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.size();
    }

private:
    mutable std::mutex m_mutex;
    size_t m_capacity;
    std::list<std::pair<Key, Value>> m_list;  // front = most recent
    // a hash map that maps from a Key to an iterator pointing to a node in the linked list
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> m_map;
};
