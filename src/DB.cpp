#include "DB.h"
#include "Utils.h"
#include "JsonConverters.h"

namespace vectordb {
Status DB::addCollection(const CollectionId& collection_name, const json& config_json) {
    // Use thread-safe check
    if (container.contains(collection_name)) {
        return Status::Error("Collection already exists: " + collection_name);
    }

    if (!config_json.contains("vectors") || !config_json["vectors"].is_object()) {
        return Status::Error("Add Collection -- Invalid vector json format. [vectors] must be an object");
    }

    if (!config_json.contains("on_disk") || !config_json["on_disk"].is_string()) {
        return Status::Error("Invalid or missing [on_disk]; must be a string 'true' or 'false'");
    }

    std::string on_disk_str = config_json["on_disk"].get<std::string>();
    std::transform(on_disk_str.begin(), on_disk_str.end(), on_disk_str.begin(), ::tolower);

    CollectionInfo collection_info;
    collection_info.name = collection_name;

    bool on_disk = (on_disk_str == "true" || on_disk_str == "1" || on_disk_str == "yes");
    collection_info.on_disk = on_disk;

    auto vector_json = config_json["vectors"];

    // Multi-vector configuration handling
    bool is_multi = std::any_of(vector_json.begin(), vector_json.end(),
        [](const auto& item) { return item.is_object(); });

    if (is_multi) {
        if (vector_json.size() > TINY_MAP_CAPACITY) {
            return Status::Error("Too many NamedVectors per Collection");
        }

        for (auto& [vec_name, vec_cfg] : vector_json.items()) {
            auto [spec, status] = parseVectorSpec(vec_name, vec_cfg);
            if (!status.ok) return status;
            collection_info.vec_specs[vec_name] = std::move(spec);
        }
    } else {
        // Single vector configuration
        auto [spec, status] = parseVectorSpec("default", vector_json);
        if (!status.ok) return status;
        collection_info.vec_specs["default"] = std::move(spec);
    }

    try {
        auto collection = std::make_unique<Collection>(collection_name, collection_info);
        CollectionEntry entry;
        entry.collection = std::move(collection);
        entry.config = config_json;

        // Thread-safe addition to container
        return container.addCollection(collection_name, std::move(entry));
        
    } catch (const std::exception& e) {
        return Status::Error("Collection creation failed: " + std::string(e.what()));
    }
}

// Helper function for vector spec parsing
std::pair<VectorSpec, Status> DB::parseVectorSpec(const std::string& name, const json& config) {
    std::string dist_str = config.value("distance", "UNKNOWN");
    DistanceMetric metric = parse_distance(dist_str);
    size_t dim = config.value("size", 0);
    bool on_disk = config.value("on_disk", false);

    if (dim == 0) {
        return {VectorSpec{}, Status::Error("Zero dimension for: " + name)};
    }
    if (metric == DistanceMetric::UNKNOWN) {
        return {VectorSpec{}, Status::Error("Unknown distance metric for: " + name)};
    }

    return {VectorSpec{dim, metric}, Status::OK()};
}

//i actually am not expecting a lot of collections created on a single computer.
//though i might also set a limit or allow the user to set a limit how many collections
//they want to use? well, i will ignore this for now, but writing this down just in case
//i forgot.
json DB::listCollections() {
    json result = json::array();

    auto collection_names = container.getCollectionNames();
    for (const auto& name : collection_names) {
        auto collection_ptr = container.getCollectionPtr(name);
        if (collection_ptr) {
            auto collectionInfo = collection_ptr->getInfo();
            // Convert CollectionInfo to JSON manually
            json vector_specs_json = json::object();
            for (const auto& [vec_name, vec_spec] : collectionInfo.vec_specs) {
                vector_specs_json[vec_name] = {
                    {"size", vec_spec.dim},
                    {"distance", to_string(vec_spec.metric)}, // You'll need to implement this
                };
            }
            
            json item = {
                {"name", name},
                {"config", {
                    {"vectors", vector_specs_json},
                    {"on_disk", collectionInfo.on_disk ? "true" : "false"}
                }},
            };
            result.push_back(item);
        }
    }//end of iterating collections

    return {
        {"status", "ok"},
        {"collections", result}
    };
}

Status DB::deleteCollection(const CollectionId& collection_name) {
    // Use thread-safe removal
    if (container.removeCollection(collection_name)) {
        return Status::OK();
    }
    return Status::Error("Collection does not exist: " + collection_name);
}


Status DB::upsertPointsToCollection(const CollectionId& collection_name, const json& points_json) {
    // Use the new optional approach
    auto access_opt = container.getCollectionForWrite(collection_name);
    if (!access_opt) {
        return Status::Error("Collection '" + collection_name + "' does not exist");
    }
    
    auto& access = access_opt.value();
    auto& collection = access.first->collection;
    auto collection_info = collection->getInfo();

    std::cout << "Upsert into collection: " << collection_name << "\n";

    if (!points_json.is_array()) {
        return Status::Error("Points must be an array");
    }

    // Extract the points
    for (const auto& p : points_json) {
        if (!p.contains("id") || !p.contains("vector")) {
            return Status::Error("Point missing id or vector field");
        }

        std::string point_id = p["id"].get<PointIdType>();
        auto payload = p.value("payload", vectordb::json::object());

        // Schema 1: vector is array (single default vector)
        if (p["vector"].is_array()) {
            auto result = validateVector("default", p["vector"], collection_info);
            if (!result.ok()) {
                return result.status();
            }
            auto status = upsertPoints(collection, point_id, result.value(), payload);
            if (!status.ok) { return status; }
        }
        // Schema 2: vector is object (multiple named vectors)
        else if (p["vector"].is_object()) {
            std::map<VectorName, DenseVector> named_vectors;

            if (p["vector"].size() > TINY_MAP_CAPACITY) {
                return Status::Error("Point has too many named vectors (max " 
                        + std::to_string(TINY_MAP_CAPACITY) + ")");
            }

            for (auto it = p["vector"].begin(); it != p["vector"].end(); ++it) {
                const auto& vec_name = it.key();
                const auto& jvec = it.value();
                
                auto result = validateVector(vec_name, jvec, collection_info);
                if (!result.ok()) {
                    return result.status();
                }
                named_vectors[vec_name] = result.value();
            }

            // Must contain at least one valid vector
            if (named_vectors.empty()) {
                return Status::Error("No valid vectors found for point " + point_id);
            }

            auto status = upsertPoints(collection, point_id, named_vectors, payload);
            if (!status.ok) { return status; }
        } else {
            return Status::Error("Invalid json vector format for point " + point_id);
        }

        //add payload here
        // auto& collection_point_payload = collection->m_point_payload;
        // collection_point_payload.putPayload(point_id, payload);

    } //end of point adding for-loop

    return Status::OK();
}


// Helper: validate and parse a single vector against spec
StatusOr<DenseVector> DB::validateVector(const VectorName& name,
                                        const json& jvec,
                                        const CollectionInfo& collection_info) {
    // Ensure it's an array
    if (!jvec.is_array()) {
        return Status::Error("Vector '" + name + "' must be an array of floats");
    }

    // Check if vector name exists in collection schema
    auto it_spec = collection_info.vec_specs.find(name);
    if (it_spec == collection_info.vec_specs.end()) {
        return Status::Error("Unknown vector name '" + name + "'");
    }

    // Validate all elements are numeric
    for (const auto& elem : jvec) {
        if (!elem.is_number()) {
            return Status::Error("Vector '" + name + 
                "' must contain only numeric values. Retry upsert again.");
        }
    }

    // Parse and validate dimension
    auto vec = jvec.get<DenseVector>();
    if (vec.size() != it_spec->second.dim) {
        return Status::Error("Dimension mismatch for '" + name +
                            "': expected " + std::to_string(it_spec->second.dim) +
                            ", got " + std::to_string(vec.size()));
    }

    return vec;
}


//for single vector inserts, so just the default named vector, no multiple named vectors
Status DB::upsertPoints(std::shared_ptr<Collection>& collection, 
                        const PointIdType& point_id, 
                        const DenseVector& vector,
                        const json& payload) 
{
    // std::cout << "Upsert into collection: " << collection_name << "\n";
    // std::cout << "Point ID: " << point_id << "\n";
    // std::cout << "Vector: [ ";

    // for (float v : vector) {
    //     std::cout << v << " ";
    // }
    // std::cout << "]\n";

    return collection->insertPoint(point_id, vector, payload);
}

//overload for multiple named vector inserts
Status DB::upsertPoints(std::shared_ptr<Collection>& collection, 
                        const PointIdType& point_id, 
                        const std::map<VectorName, DenseVector>& named_vectors,
                        const json& payload)
{
    // std::cout << "Point ID: " << point_id << "\n";

    // for (const auto& [name, vec] : named_vectors) {
    //     std::cout << "Vector name: " << name << " -> [ ";
    //     for (float v : vec) {
    //         std::cout << v << " ";
    //     }
    //     std::cout << "]\n";
    // }

    return collection->insertPoint(point_id, named_vectors, payload);

}

//Still missing query by points here. Need to add the logic...
json DB::queryCollection(const std::string& collection_name, 
                         const json& query_body,
                         const std::string& using_index, 
                         size_t top_k)
{
    auto access_opt = container.getCollectionForRead(collection_name);
    if (!access_opt) {
        return { {"status", "error"}, {"message", "Collection not found"} };
    }
    
    auto& access = access_opt.value();
    auto& collection = access.first->collection;
    auto collection_info = collection->getInfo();
    QueryResult qr;
    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << query_body.dump(4) << std::endl;

    if (query_body.contains("query_vectors")) 
    {
        const auto& query_vectors_json = query_body["query_vectors"];
        if (!query_vectors_json.is_array() || query_vectors_json.empty()) {
            return { {"status", "error"}, {"message", "'query_vectors' must be a non-empty array"} };
        }

        std::vector<DenseVector> query_vectors;
        const std::string vector_name = query_body.value("using", "default");

        for (const auto& vec_json : query_vectors_json) {
            auto result = validateVector(vector_name, vec_json, collection_info);
            if (!result.ok()) {
                return { {"status", "error"}, {"message", result.status().message} };
            }
            query_vectors.push_back(result.value());
        }

        if (query_vectors.empty()) {
            qr.status = Status::Error("No valid vectors found for given point IDs");
        } else {
            qr = collection->searchTopK(vector_name, query_vectors, top_k);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    qr.time_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    json response;
    vectordb::to_json(response, qr);
    return response;

}

}// end of vectordb namespace