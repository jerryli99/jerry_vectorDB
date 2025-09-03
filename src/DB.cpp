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
        // Case 2: single vector config → assign default name => 
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
    entry.config = config_json;//i just store the config in case i might need it

    container.m_collections[collection_name] = std::move(entry);

    return Status::OK();
}

//i actually am not expecting a lot of collections created on a single computer.
//though i might also set a limit or allow the user to set a limit how many collections
//they want to use? well, i will ignore this for now, but writing this down just in case
//i forgot.
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

//well, this will be ignored for now since it could be complicated when dealing with memory stuff.
//we will see about it.
Status DB::deleteCollection(const CollectionId& collection_name) {
    auto it = container.m_collections.find(collection_name);
    if (it == container.m_collections.end()) {
        return Status::Error("Collection does not exist: " + collection_name);
    } 
    container.m_collections.erase(it); //
    return Status::OK();
}

Status DB::upsertPointToCollection(const CollectionId& collection_name, const json& points_json) {
    // Check collection existence
    auto it = container.m_collections.find(collection_name);

    if (it == container.m_collections.end()) {
        return Status::Error("Collection '" + collection_name + "' does not exist");
    }

    auto& collection = it->second;  // reference to your Collection object

    if (!points_json.is_array()) {
        return Status::Error("Points must be an array");
    }

    //extract the points out of the json obj
    for (const auto& p : points_json) {
        if (!p.contains("id") || !p.contains("vector")) {
            return Status::Error("Point missing id or vector field");
        }

        std::string id = p["id"].get<PointIdType>();
        auto payload = p.value("payload", vectordb::json::object());

        // Schema 1: vector is array
        if (p["vector"].is_array()) {
            auto vec = p["vector"].get<DenseVector>();
            auto status = upsertPoints(collection_name, id, vec, payload);
            if (!status.ok) { return status; }
        }
        // Schema 2: vector is object of named vectors
        else if (p["vector"].is_object()) {
            std::map<VectorName, DenseVector> named_vectors;
            for (auto it = p["vector"].begin(); it != p["vector"].end(); ++it) {
                named_vectors[it.key()] = it.value().get<DenseVector>();
            }
            auto status = upsertPoints(collection_name, id, named_vectors, payload);
            if (!status.ok) { return status; }
        }
        else {
            return Status::Error("Invalid vector format for point " + id);
        }
    }

    return Status::OK();
}

//for single vector inserts, so just the default named vector, no multiple named vectors
Status DB::upsertPoints(const CollectionId& collection_name, 
                        const PointIdType& point_id, 
                        const DenseVector& vector, 
                        const json& payload) 
{
    std::cout << "Upsert into collection: " << collection_name << "\n";
    std::cout << "Point ID: " << point_id << "\n";
    std::cout << "Vector: [ ";
    for (float v : vector) {
        std::cout << v << " ";
    }
    std::cout << "]\n";
    return Status::OK();
}

//overload for multiple named vector inserts
Status DB::upsertPoints(const CollectionId& collection_name, 
                        const PointIdType& point_id, 
                        const std::map<VectorName, DenseVector>& named_vectors, 
                        const json& payload)
{
    std::cout << "Upsert into collection: " << collection_name << "\n";
    std::cout << "Point ID: " << point_id << "\n";

    for (const auto& [name, vec] : named_vectors) {
        std::cout << "Vector name: " << name << " -> [ ";
        for (float v : vec) {
            std::cout << v << " ";
        }
        std::cout << "]\n";
    }
    return Status::OK();

}
}// end of vectordb namespace