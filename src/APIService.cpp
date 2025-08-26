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

        vectordb::json config_json = json_body["vectors"];

        if (config_json.is_object()) {
            auto status = vec_db->addCollection(collection_name, config_json);
            if (!status.ok) {
            vectordb::api_send_error(res, 400, status.message, vectordb::APIErrorType::UserInput);
            return;
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
            vectordb::api_send_error(res, 400, "Missing collection_name", vectordb::APIErrorType::UserInput);
            return;
        }
        std::string collection_name = j["collection_name"];

        if (collect_config_table.find(collection_name) == collect_config_table.end()) {
            vectordb::api_send_error(res, 404, "Collection not found", vectordb::APIErrorType::UserInput);
            return;
        }

        // Just respond OK for test purposes
        res.set_content(R"({"status":"ok"})", "application/json");
    } catch (const vectordb::json::parse_error &e) {
        vectordb::api_send_error(res, 400, std::string("Invalid JSON: ") + e.what(), vectordb::APIErrorType::UserInput);
    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500, std::string("Internal server error: ") + e.what(), vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error", vectordb::APIErrorType::Connection);
    }
});

std::cout << "Server listening on http://127.0.0.1:8989\n";
if (!svr.listen("127.0.0.1", 8989)) {
    std::cerr << "[FATAL] Failed to start server on port 8989\n";
    return 1;
}

}