/*
Main function goes here to start vectorDB service
*/

#include "httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>
#include <string>

using json = nlohmann::json;

struct CollectionConfig {
    json config;
};

static std::unordered_map<std::string, CollectionConfig> collections;

void send_error(httplib::Response &res, int status_code, const std::string &message) {
    res.status = status_code;
    res.set_content(json{
        {"status", "error"},
        {"message", message}
    }.dump(), "application/json");
}

int main() {
    httplib::Server svr;

    // Create Collection
    svr.Put(R"(/collections/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            if (req.matches.size() < 2) {
                send_error(res, 400, "Missing collection name");
                return;
            }
            std::string collection_name = req.matches[1];

            auto j = json::parse(req.body);
            std::cout << "Create collection: " << collection_name << "\n" << j.dump(4) << "\n";

            if (collections.find(collection_name) != collections.end()) {
                send_error(res, 409, "Collection already exists");
                return;
            }

            collections[collection_name] = { j };
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (const json::parse_error &e) {
            send_error(res, 400, std::string("Invalid JSON: ") + e.what());
        } catch (const std::exception &e) {
            send_error(res, 500, std::string("Internal server error: ") + e.what());
        } catch (...) {
            send_error(res, 500, "Unknown error");
        }
    });

    // List Collections
    svr.Get("/collections", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            json result = json::array();
            for (const auto& kv : collections) {
                json entry;
                entry["name"] = kv.first;
                entry["config"] = kv.second.config;
                result.push_back(entry);
            }

            res.set_content(json{
                {"status", "ok"},
                {"collections", result}
            }.dump(), "application/json");

        } catch (const std::exception &e) {
            send_error(res, 500, std::string("Internal server error: ") + e.what());
        } catch (...) {
            send_error(res, 500, "Unknown error");
        }
    });
    // Delete Collection
    svr.Delete(R"(/collections/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            if (req.matches.size() < 2) {
                send_error(res, 400, "Missing collection name");
                return;
            }
            std::string collection_name = req.matches[1];
            std::cout << "Delete collection: " << collection_name << "\n";

            if (collections.erase(collection_name) > 0) {
                res.set_content(R"({"status":"ok"})", "application/json");
            } else {
                send_error(res, 404, "Collection not found");
            }
        } catch (const std::exception &e) {
            send_error(res, 500, std::string("Internal server error: ") + e.what());
        } catch (...) {
            send_error(res, 500, "Unknown error");
        }
    });

    // Upsert
    svr.Post("/upsert", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::cout << "Received /upsert request:\n" << j.dump(4) << "\n";

            if (!j.contains("collection_name")) {
                send_error(res, 400, "Missing collection_name");
                return;
            }
            std::string collection_name = j["collection_name"];

            if (collections.find(collection_name) == collections.end()) {
                send_error(res, 404, "Collection not found");
                return;
            }

            // Just respond OK for test purposes
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (const json::parse_error &e) {
            send_error(res, 400, std::string("Invalid JSON: ") + e.what());
        } catch (const std::exception &e) {
            send_error(res, 500, std::string("Internal server error: ") + e.what());
        } catch (...) {
            send_error(res, 500, "Unknown error");
        }
    });

    std::cout << "Server listening on http://127.0.0.1:8989\n";
    if (!svr.listen("127.0.0.1", 8989)) {
        std::cerr << "[FATAL] Failed to start server on port 8989\n";
        return 1;
    }
}



// #include "httplib.h"
// #include <nlohmann/json.hpp>
// #include <iostream>

// using json = nlohmann::json;

// int main() {
//     httplib::Server svr;

//     svr.Post("/upsert", [](const httplib::Request& req, httplib::Response& res) {
//         try {
//             auto j = json::parse(req.body);
//             std::cout << "Received /upsert request:\n" << j.dump(4) << std::endl;

//             // Just respond OK for test purposes
//             res.set_content(R"({"status":"ok"})", "application/json");
//         } catch (...) {
//             res.status = 400;
//             res.set_content(R"({"status":"error","message":"invalid json"})", "application/json");
//         }
//     });

//     std::cout << "Server listening on http://127.0.0.1:8989\n";
//     svr.listen("127.0.0.1", 8989);
// }