#include "DB.h"
#include "Utils.h"

namespace vectordb {
Status DB::addCollection(const CollectionId& collection_name, const json& config_json) {
    // Use thread-safe check
    if (container.contains(collection_name)) {
        return Status::Error("Collection already exists: " + collection_name);
    }

    CollectionInfo collection_info;
    collection_info.name = collection_name;

    // Multi-vector configuration handling
    bool is_multi = std::any_of(config_json.begin(), config_json.end(),
        [](const auto& item) { return item.is_object(); });

    if (is_multi) {
        if (config_json.size() > MAX_ENTRIES_TINYMAP) {
            return Status::Error("Too many NamedVectors per Collection");
        }

        for (auto& [vec_name, vec_cfg] : config_json.items()) {
            auto [spec, status] = parseVectorSpec(vec_name, vec_cfg);
            if (!status.ok) return status;
            collection_info.vec_specs[vec_name] = std::move(spec);
        }
    } else {
        // Single vector configuration
        auto [spec, status] = parseVectorSpec("default", config_json);
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

    return {VectorSpec{dim, metric, on_disk}, Status::OK()};
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
            // Convert CollectionInfo to JSON manually
            json vector_specs_json = json::object();
            for (const auto& [vec_name, vec_spec] : collection_ptr->m_collection_info.vec_specs) {
                vector_specs_json[vec_name] = {
                    {"size", vec_spec.dim},
                    {"distance", to_string(vec_spec.metric)}, // You'll need to implement this
                    {"on_disk", vec_spec.on_disk}
                };
            }
            
            json item = {
                {"name", name},
                {"config", {
                    {"vectors", vector_specs_json}
                }}
            };
            result.push_back(item);
        }
    }

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
    auto collection_info = collection->m_collection_info;

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
            auto status = upsertPoints(collection, point_id, result.value());
            if (!status.ok) { return status; }
        }
        // Schema 2: vector is object (multiple named vectors)
        else if (p["vector"].is_object()) {
            std::map<VectorName, DenseVector> named_vectors;

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

            auto status = upsertPoints(collection, point_id, named_vectors);
            if (!status.ok) { return status; }
        } else {
            return Status::Error("Invalid json vector format for point " + point_id);
        }

        //add payload here
        auto& collection_point_payload = collection->m_point_payload;
        collection_point_payload.putPayload(point_id, payload);

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
Status DB::upsertPoints(std::shared_ptr<Collection> collection, 
                        const PointIdType& point_id, 
                        const DenseVector& vector) 
{
    // std::cout << "Upsert into collection: " << collection_name << "\n";
    std::cout << "Point ID: " << point_id << "\n";
    std::cout << "Vector: [ ";

    for (float v : vector) {
        std::cout << v << " ";
    }
    std::cout << "]\n";
    return Status::OK();
}

//overload for multiple named vector inserts
Status DB::upsertPoints(std::shared_ptr<Collection> collection, 
                        const PointIdType& point_id, 
                        const std::map<VectorName, DenseVector>& named_vectors)
{
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