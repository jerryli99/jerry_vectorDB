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

I am not familiar with writing API endpoints, especially in C++...
So I might miss one or two json field type checking or have tedious code. 

Also, i don't really care much about the exact error code i need to return to the user 
since i only care about if the prototype is working or not, i just care about the error msg.
*/


int main() {

// std::unique_ptr<vectordb::DB> vec_db = std::make_unique<vectordb::DB>();
// Get the singleton instance as a reference
vectordb::DB& vec_db = vectordb::DB::getInstance();

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

        //maybe in the future check number of fields as well (must be 2)? ignore this now...

        auto status = vec_db.addCollection(collection_name, json_body);
        if (!status.ok) {
            vectordb::api_send_error(res, 400, status.message, vectordb::APIErrorType::UserInput);
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


// List Collections, yes i can make it look like /collections/v1/....but i will just keep it simple
//thinking about using LRU to cache the results for GET requests...
svr.Get("/collections", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (!req.body.empty()) {
            vectordb::api_send_error(res, 400, "GET collections does not accept a request body", vectordb::APIErrorType::UserInput);
            return;
        }
        auto result_json = vec_db.listCollections();
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

//search by vector(s) or by point id
//the api is like /collection/{collection_name}/points/query
// Query endpoint
// Query endpoint
svr.Post(R"(/collections/(.+)/query)", [&](const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            vectordb::api_send_error(res, 400, "Missing collection name", vectordb::APIErrorType::UserInput);
            return;
        }
        std::cout << "Hello\n" << std::endl;
        std::string collection_name = req.matches[1];
        auto json_body = vectordb::json::parse(req.body);

        std::cout << "Query request for collection: " << collection_name << "\n" << json_body.dump(4) << "\n";

        // Basic validation
        if (!json_body.is_object()) {
            vectordb::api_send_error(res, 400, "Request body must be a JSON object", vectordb::APIErrorType::UserInput);
            return;
        }

        bool has_vectors = json_body.contains("query_vectors");
        bool has_ids = json_body.contains("query_pointids");

        if (has_vectors && has_ids) {
            vectordb::api_send_error(res, 400,
                "Provide only one of 'query_vectors' or 'query_pointids'", 
                vectordb::APIErrorType::UserInput);
            return;
        }

        if (!has_vectors && !has_ids) {
            vectordb::api_send_error(res, 400,
                "Must provide at least 'query_vectors' or 'query_pointids'", 
                vectordb::APIErrorType::UserInput);
            return;
        }

        if (has_vectors && !json_body["query_vectors"].is_array()) {
            vectordb::api_send_error(res, 400, "'query_vectors' must be an array", vectordb::APIErrorType::UserInput);
            return;
        }

        if (has_ids && !json_body["query_pointids"].is_array()) {
            vectordb::api_send_error(res, 400, "'query_pointids' must be an array", vectordb::APIErrorType::UserInput);
            return;
        }

        // Optional fields with defaults
        std::string using_index = "default";
        std::size_t top_k = 0;

        if (json_body.contains("using")) {
            if (!json_body["using"].is_string()) {
                vectordb::api_send_error(res, 400, "'using' must be a string", vectordb::APIErrorType::UserInput);
                return;
            }
            using_index = json_body["using"];
        }

        if (json_body.contains("top_k")) {
            if (!json_body["top_k"].is_number_integer()) {
                vectordb::api_send_error(res, 400, "'top_k' must be an integer", vectordb::APIErrorType::UserInput);
                return;
            }
            top_k = json_body["top_k"];
            if (top_k <= 0 || top_k > 1000) {
                vectordb::api_send_error(res, 400, "'top_k' must be between 1 and 1000", vectordb::APIErrorType::UserInput);
                return;
            }
        }

        std::cout << "=============> Entering query" << std::endl;

        auto result_json = vec_db.queryCollection(collection_name, json_body, using_index, top_k);
        
        // Send results
        res.set_content(result_json.dump(), "application/json");

    } catch (const vectordb::json::parse_error &e) {
        vectordb::api_send_error(res, 400,
            std::string("Invalid JSON: ") + e.what(),
            vectordb::APIErrorType::UserInput);
    } catch (const std::exception &e) {
        vectordb::api_send_error(res, 500,
            std::string("Internal server error: ") + e.what(),
            vectordb::APIErrorType::Server);
    } catch (...) {
        vectordb::api_send_error(res, 500, "Unknown error", vectordb::APIErrorType::Connection);
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
            vectordb::api_send_error(res, 400, "DELETE collections does not accept a request body", 
                vectordb::APIErrorType::UserInput);
            return;
        }

        std::string collection_name = req.matches[1];
        std::cout << "Delete collection: " << collection_name << "\n";

        auto status = vec_db.deleteCollection(collection_name);
        if (!status.ok) {
            vectordb::api_send_error(res, 500, status.message, vectordb::APIErrorType::Server);
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
//i did not do a good job at handling or checking inputs because i will have to spend a lot of time
//and i decide to use this time to do better stuff, so yeah. Ok, here we go...
svr.Post("/upsert", [&](const httplib::Request& req, httplib::Response& res) {
    //Check raw body size first (before JSON parsing!)
    if (req.body.size() > vectordb::MAX_JSON_REQUEST_SIZE) {
        vectordb::json error = {
            {"error", "REQUEST_TOO_LARGE"},
            {"message", "Request body too large"},
            {"max_num_vectors", vectordb::MAX_POINTS_PER_REQUEST},
            {"max_size_bytes", vectordb::MAX_JSON_REQUEST_SIZE},
            {"received_bytes", req.body.size()},
            {"suggestion", "Split into smaller batches"}
        };
        res.set_content(error.dump(), "application/json");
        return;
    }

    try {
        auto json_body = vectordb::json::parse(req.body);
        // std::cout << "Received /upsert request:\n" << json_body.dump(4) << "\n";

        // Check number of points
        if (json_body.contains("points") && json_body["points"].is_array()) {
            if (json_body["points"].size() > vectordb::MAX_POINTS_PER_REQUEST) {
                vectordb::api_send_error(res, 413, 
                    "Too many points. Maximum: " + std::to_string(vectordb::MAX_POINTS_PER_REQUEST),
                    vectordb::APIErrorType::UserInput);
                return;
            }
        }

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

        auto status = vec_db.upsertPointsToCollection(collection_name, points_json);

        if (!status.ok) {
            // std::cout << "Upsert failed: " << status.message << std::endl;
            vectordb::api_send_error(res, 500, status.message, vectordb::APIErrorType::Server);
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

std::cout << "Server listening on http://127.0.0.1:8989\n";
if (!svr.listen("127.0.0.1", 8989)) {
    std::cerr << "[FATAL] Failed to start server on port 8989\n";
    return 1;
}

}