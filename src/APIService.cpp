/*
Main function goes here to start vectorDB service
*/

#include "httplib.h"
#include "DataTypes.h"
#include "Utils.h"
#include "DB.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>
#include <string>

struct CollectionConfig {
    vectordb::json config;
};

//uhm, this is useful for just storing collection names and the corresponding configs.
//i will just put this here for now. The code here is a bit messy.
//the actual collection object will be stored in the DB though.
static std::unordered_map<vectordb::CollectionId, CollectionConfig> collect_config_table;

int main() {

std::unique_ptr<vectordb::DB> vec_db;
httplib::Server svr;

// Create Collection (I used [&] so i can capture all local variables by reference)
svr.Put(R"(/collections/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            vectordb::api_send_error(res, 400, "Missing collection name", vectordb::APIErrorType::UserInput);
            return;
        }

        std::string collection_name = req.matches[1];

        auto json_body = vectordb::json::parse(req.body);
        std::cout << "Create collection: " << collection_name << "\n" << json_body.dump(4) << "\n";

        if (collect_config_table.find(collection_name) != collect_config_table.end()) {
            vectordb::api_send_error(res, 409, "Collection already exists", vectordb::APIErrorType::UserInput);
            return;
        }

        if (!json_body.contains("vectors")) {
            vectordb::api_send_error(res, 400, "Missing the [vectors] field", vectordb::APIErrorType::UserInput);
            return;
        }

        vectordb::json vectors_json = json_body["vectors"];

        if (vectors_json.is_object()) {
            //case1: single vector config
            bool is_multi = false;
            for (auto& [key, value] : vectors_json.items()) {
                if (value.is_object()) {
                    //if values are objects themselves, then it has multiple named vectors to config
                    is_multi = true;
                    break;
                }
            }

            if (is_multi) {
                // Case 2: Multi-vector config
                std::cout << "Multi named vector collection detected:\n";
                //do a checking to limit num of namedvectors in a collection ! 
                if (vectors_json.size() > vectordb::MAX_ENTRIES_TINYMAP) {
                    vectordb::api_send_error(res, 400, "Too many NamedVectors per Collection. Maximum allowed is set to " + 
                        std::to_string(vectordb::MAX_ENTRIES_TINYMAP), vectordb::APIErrorType::UserInput);
                    return;
                }

                for (auto& [vec_name, vec_cfg] : vectors_json.items()) {
                    std::cout << "  Vector name: " << vec_name << "\n";
                    std::string dist_str = vec_cfg.value("distance", "UNKNOWN");
                    vectordb::DistanceMetric metric = vectordb::parse_distance(dist_str);
                    std::cout << "    distance: " << to_string(metric) << "\n";
                    std::cout << "    size: " << vec_cfg.value("size", -1) << "\n";
                    bool on_disk = vec_cfg.value("on_disk", false);
                    std::cout << "    on_disk: " << (on_disk ? "true" : "false") << "\n";
                }
            } else {
                // Case 1 confirmed, add default vector name "default" here
                std::string vec_name = "default";
                vectordb::json vec_cfg = vectors_json;  // whole object is the config
                std::cout << "  Vector name: " << vec_name << "\n";
                std::string dist_str = vec_cfg.value("distance", "UNKNOWN");
                vectordb::DistanceMetric metric = vectordb::parse_distance(dist_str);
                std::cout << "    distance: " << to_string(metric) << "\n";
                std::cout << "    size: " << vec_cfg.value("size", -1) << "\n";
                bool on_disk = vec_cfg.value("on_disk", false);
                std::cout << "    on_disk: " << (on_disk ? "true" : "false") << "\n";
            }
        } else {
            vectordb::api_send_error(res, 400, "[vectors] must be an object", vectordb::APIErrorType::UserInput);
            return;
        }

        //last just add meta info of our collections to the config table..
        collect_config_table[collection_name] = { json_body };
        res.set_content(R"({"status":"ok"})", "application/json");

    } catch (const vectordb::json::parse_error &e) {
        vectordb::api_send_error(res, 400, std::string("Invalid JSON: ") + e.what(), vectordb::APIErrorType::UserInput);
    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500, std::string("Internal server error: ") + e.what(), vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error", vectordb::APIErrorType::Connection);
    }
});

// List Collections
svr.Get("/collections", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (collect_config_table.empty()) {
            res.set_content(vectordb::json{
                {"status", "ok"},
                {"collections", vectordb::json::array()}
            }.dump(), "application/json");
            return;
        }

        vectordb::json result = vectordb::json::array();
        for (const auto& kv : collect_config_table) {
            vectordb::json entry = {
                {"name", kv.first},
                {"config", kv.second.config}
            };
            result.push_back(entry);
        }

        res.set_content(vectordb::json{
            {"status", "ok"},
            {"collections", result}
        }.dump(), "application/json");

    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500, std::string("Internal server error: ") + e.what(), vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error", vectordb::APIErrorType::Connection);
    }
});

// Delete Collection
svr.Delete(R"(/collections/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            vectordb::api_send_error(res, 400, "Missing collection name", vectordb::APIErrorType::UserInput);
            return;
        }
        std::string collection_name = req.matches[1];
        std::cout << "Delete collection: " << collection_name << "\n";

        if (collect_config_table.erase(collection_name) > 0) {
            res.set_content(R"({"status":"ok"})", "application/json");
        } else {
            vectordb::api_send_error(res, 404, "Collection not found", vectordb::APIErrorType::UserInput);
        }
    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500, std::string("Internal server error: ") + e.what(), vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error", vectordb::APIErrorType::Connection);
    }
});

// Upsert
svr.Post("/upsert", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = vectordb::json::parse(req.body);
        std::cout << "Received /upsert request:\n" << j.dump(4) << "\n";

        if (!j.contains("collection_name")) {
            vectordb::api_send_error(res, 400, "Missing collection_name");
            return;
        }
        std::string collection_name = j["collection_name"];

        if (collect_config_table.find(collection_name) == collect_config_table.end()) {
            vectordb::api_send_error(res, 404, "Collection not found");
            return;
        }

        // Just respond OK for test purposes
        res.set_content(R"({"status":"ok"})", "application/json");
    } catch (const vectordb::json::parse_error &e) {
        vectordb::api_send_error(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500, std::string("Internal server error: ") + e.what());
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error");
    }
});

std::cout << "Server listening on http://127.0.0.1:8989\n";
if (!svr.listen("127.0.0.1", 8989)) {
    std::cerr << "[FATAL] Failed to start server on port 8989\n";
    return 1;
}

}