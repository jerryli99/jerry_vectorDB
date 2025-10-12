#pragma once

#include "DataTypes.h"
#include "QueryResult.h" 

namespace vectordb {

inline void to_json(json& j, const ScoredId& s) {
    j = json{{"id", s.id}, {"score", s.score}};
}

inline void to_json(json& j, const QueryResult& r) {
    j["status"] = r.status.ok ? "ok" : r.status.message;
    j["time"] = r.time_seconds;
    j["results"] = json::array();
    for (const auto& batch : r.results) {
        json batch_json = json::array();
        for (const auto& hit : batch.hits)
            batch_json.push_back(hit);
        j["results"].push_back(batch_json);
    }
}

} // namespace vectordb
