#include "DB.h"
#include "Utils.h"

namespace vectordb {
Status DB::addCollection(const CollectionId& collection_name, const json& config_json) {

    //just double checking here.
    if (container.m_collections.find(collection_name) != container.m_collections.end()) {
        return Status::Error("Collection already exists: " + collection_name);
    }

    CollectionInfo collection_info;
    collection_info.name = collection_name;
    

    // Case 1: multi named vectors
    bool is_multi = false;
    for (auto& [key, value] : config_json.items()) {
        if (value.is_object()) {
            is_multi = true;
            break;
        }
    }

    if (is_multi) {
        if (config_json.size() > MAX_ENTRIES_TINYMAP) {
            return Status::Error("Too many NamedVectors per Collection (max " + std::to_string(MAX_ENTRIES_TINYMAP) + ")");
        }

        for (auto& [vec_name, vec_cfg] : config_json.items()) {
            std::string dist_str = vec_cfg.value("distance", "UNKNOWN");
            DistanceMetric metric = parse_distance(dist_str);
            size_t dim = vec_cfg.value("size", 0);
            bool on_disk = vec_cfg.value("on_disk", false);

            VectorSpec spec{dim, metric, on_disk};
            collection_info.vec_specs.insert(vec_name, spec);
        }
    } else {
        // Case 2: single vector config → assign default name
        std::cout << "hello";
        std::string vec_name = "default";
        const json& vec_cfg = config_json;
        std::string dist_str = vec_cfg.value("distance", "UNKNOWN");
        DistanceMetric metric = parse_distance(dist_str);
        size_t dim = vec_cfg.value("size", 0);
        bool on_disk = vec_cfg.value("on_disk", false);

        VectorSpec spec{dim, metric, on_disk};
        collection_info.vec_specs.insert(vec_name, spec);
    }

    auto collection = std::make_unique<Collection>(collection_name, collection_info);

    CollectionEntry entry;
    entry.collection = std::move(collection);
    entry.config = config_json;

    container.m_collections[collection_name] = std::move(entry);

    return Status::OK();
}

json DB::listCollections() {
    json result = json::array();

    for (const auto& [name, entry] : container.m_collections) {
        json item = {
            {"name", name},
            {"config", entry.config}
        };
        result.push_back(item);
    }

    return {
        {"status", "ok"},
        {"collections", result}
    };
}

Status DB::deleteCollection(const CollectionId& collection_name) {
    auto it = container.m_collections.find(collection_name);
    if (it == container.m_collections.end()) {
        return Status::Error("Collection does not exist: " + collection_name);
    } 
    container.m_collections.erase(it); //
    return Status::OK();
}

Status DB::upsertPointToCollection(const CollectionId& collection_name, ...) {

    return Status::OK();
}

}// end of vectordb namespace