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

/*
Yes the code is a mess, but for now i think is it fine to put all the stuff in main for now.
I might have a CLI interface here. Just maybe..

I am not familiar with writing API endpoints, especially in C++...So I might miss one or two json field type checking
or have tedious code. But i feel like 
*/


int main() {

std::unique_ptr<vectordb::DB> vec_db = std::make_unique<vectordb::DB>();;
httplib::Server svr;

//So we only create one collection at a time. The concept of a collection is a collection of
//segments containing points. So when I designed this, I was not expecting a lot of collections
//being created on a single machine.
svr.Put(R"(/collections/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            vectordb::api_send_error(res, 400, "Missing collection name", vectordb::APIErrorType::UserInput);
            return;
        }

        std::string collection_name = req.matches[1];

        auto json_body = vectordb::json::parse(req.body);
        std::cout << "Create collection: " << collection_name << "\n" << json_body.dump(4) << "\n";

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
        if (!req.body.empty()) {
            vectordb::api_send_error(res, 400, "GET collections does not accept a request body", vectordb::APIErrorType::UserInput);
            return;
        }
        auto result_json = vec_db->listCollections();
        res.set_content(result_json.dump(), "application/json");

    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500,
            std::string("Internal server error: ") + e.what(),
            vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error",
            vectordb::APIErrorType::Connection);
    }
});

// Delete Collection
svr.Delete(R"(/collections/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            vectordb::api_send_error(
                res, 400, "Missing collection name", vectordb::APIErrorType::UserInput
            );
            return;
        }

        if (!req.body.empty()) {
            vectordb::api_send_error(res, 400, "DELETE collections does not accept a request body", vectordb::APIErrorType::UserInput);
            return;
        }

        std::string collection_name = req.matches[1];
        std::cout << "Delete collection: " << collection_name << "\n";

        auto status = vec_db->deleteCollection(collection_name);
        if (!status.ok) {
            vectordb::api_send_error(res, 404, status.message, vectordb::APIErrorType::UserInput);
            return;
        }

        res.set_content(R"({"status":"ok"})", "application/json");

    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500, std::string("Internal server error: ") + e.what(), vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error", vectordb::APIErrorType::Connection);
    }
});


// Upsert
//ok, where we go...
svr.Post("/upsert", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        auto json_body = vectordb::json::parse(req.body);
        std::cout << "Received /upsert request:\n" << json_body.dump(4) << "\n";

        // Validate top-level keys
        static const std::set<std::string> allowed_keys = {"collection_name", "points"};
        for (auto& [key, _] : json_body.items()) {
            if (allowed_keys.find(key) == allowed_keys.end()) {
                vectordb::api_send_error(res, 400, "Unexpected field: " + key, vectordb::APIErrorType::UserInput);
                return;
            }
        }

        // Required fields
        if (!json_body.contains("collection_name")) {
            vectordb::api_send_error(res, 400, "Missing collection_name", vectordb::APIErrorType::UserInput);
            return;
        }

        if (!json_body["collection_name"].is_string()) {
            vectordb::api_send_error(res, 400, "collection_name must be a string", vectordb::APIErrorType::UserInput);
            return;
        }

        std::string collection_name = json_body["collection_name"];

        if (!json_body.contains("points")) {
            vectordb::api_send_error(res, 400, "Missing points", vectordb::APIErrorType::UserInput);
            return;
        }

        auto points_json = json_body["points"];

        auto status = vec_db->upsertPointToCollection(collection_name, points_json);

        if (!status.ok) {
            vectordb::api_send_error(res, 404, status.message, vectordb::APIErrorType::UserInput);
        } else {
            vectordb::api_send_error(res, 400, status.message, vectordb::APIErrorType::UserInput);
        }

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