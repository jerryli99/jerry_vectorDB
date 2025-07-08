// #pragma once

// #include <string>
// #include <variant>
// #include <unordered_map>
// #include <vector>
// #include <iostream>

// namespace vectordb {

// /// Types that can be stored in a payload
// using PayloadValue = std::variant<
//     std::string,
//     int64_t,
//     double,
//     bool,
//     std::nullptr_t,
//     std::vector<std::string>
// >;

// /// Represents per-point metadata used for filtering
// class Payload {
// public:
//     /// Set a key to a given value
//     void set(const std::string& key, PayloadValue value) {
//         m_data[key] = std::move(value);
//     }

//     /// Check if a key exists with the given type
//     template <typename T>
//     bool has(const std::string& key) const {
//         auto it = m_data.find(key);
//         return it != m_data.end() && std::holds_alternative<T>(it->second);
//     }

//     /// Get a pointer to the value if it exists and is of the right type
//     template <typename T>
//     const T* get(const std::string& key) const {
//         auto it = m_data.find(key);
//         return (it != m_data.end()) ? std::get_if<T>(&it->second) : nullptr;
//     }

//     /// Print all keys and values
//     void print(std::ostream& os = std::cout) const {
//         for (const auto& [k, v] : m_data) {
//             os << k << ": ";
//             std::visit([&os](const auto& val) {
//                 using V = std::decay_t<decltype(val)>;
//                 if constexpr (std::is_same_v<V, std::vector<std::string>>) {
//                     os << "[";
//                     for (const auto& s : val) os << s << ", ";
//                     os << "]";
//                 } else if constexpr (std::is_same_v<V, std::nullptr_t>) {
//                     os << "null";
//                 } else {
//                     os << val;
//                 }
//                 os << "\n";
//             }, v);
//         }
//     }

// private:
//     std::unordered_map<std::string, PayloadValue> m_data;
// };

// } // namespace vectordb


// /*
// #include "Payload.h"

// int main() {
//     vectordb::Payload p;
//     p.set("type", std::string("image"));
//     p.set("score", 0.92);
//     p.set("tags", std::vector<std::string>{"cnn", "deep-learning"});
//     p.set("active", true);

//     if (p.has<std::string>("type")) {
//         std::cout << "Type: " << *p.get<std::string>("type") << "\n";
//     }

//     p.print();
// }


// */