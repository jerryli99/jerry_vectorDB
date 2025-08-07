/*
Main function goes here to start vectorDB service
*/


#include "httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

int main() {
    httplib::Server svr;

    svr.Post("/upsert", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::cout << "Received /upsert request:\n" << j.dump(4) << std::endl;

            // Just respond OK for test purposes
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"status":"error","message":"invalid json"})", "application/json");
        }
    });

    std::cout << "Server listening on http://127.0.0.1:8989\n";
    svr.listen("127.0.0.1", 8989);
}