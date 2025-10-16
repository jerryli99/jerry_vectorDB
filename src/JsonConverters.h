#pragma once

#include "DataTypes.h"
#include "QueryResult.h"

namespace vectordb {

inline void to_json(json& j, const ScoredId& s) {
    j = json{
        {"id", s.id},
        {"score", s.score}
    };
}

inline void to_json(json& j, const QueryBatchResult& b) {
    // Make it an object with a field "hits" instead of a bare array
    j = json{
        {"hits", b.hits}  // This will automatically call to_json for ScoredId
    };
}

inline void to_json(json& j, const QueryResult& r) {
    j = json{
        {"status", r.status.ok ? "ok" : r.status.message},
        {"time", r.time_seconds},
        {"result", r.results} // let json lib expand using the converters above
    };
}

} // namespace vectordb
