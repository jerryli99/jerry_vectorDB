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
        entry.config = std::move(config_json);

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

    // Use thread-safe iteration
    auto collection_names = container.getCollectionNames();
    for (const auto& name : collection_names) {
        if (const auto* entry = container.getCollection(name)) {
            json item = {
                {"name", name},
                {"config", entry->config}
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

// Helper: validate and parse a single vector against spec
StatusOr<DenseVector> validateVector(
    const VectorName& name,
    const json& jvec,
    const CollectionInfo& collection_info)
{
    // Ensure it's an array
    if (!jvec.is_array()) {
        return Status::Error("Vector '" + name + "' must be an array of floats");
    }

    // Check schema
    auto it_spec = collection_info.vec_specs.find(name);
    if (it_spec == collection_info.vec_specs.end()) {
        return Status::Error("Unknown vector name '" + name + "'");
    }

    // Validate elements
    for (const auto& elem : jvec) {
        if (!elem.is_number()) {
            return Status::Error("Vector '" + name + "' must contain only numeric values");
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


Status DB::upsertPointsToCollection(const CollectionId& collection_name, const json& points_json) {
    // First, check if collection exists (shared lock)
    auto* entry = container.getCollection(collection_name);
    if (!entry) {
        return Status::Error("Collection '" + collection_name + "' does not exist");
    }

    // Get collection-specific mutex for fine-grained locking
    auto& collection_mutex = container.getCollectionMutex(collection_name);
    
    // Use UNIQUE lock for writing operations (upserts modify data)
    std::unique_lock lock(collection_mutex);

    // Get collection info (safe now that we have the lock)
    auto collection_info = entry->collection->m_collection_info;

    if (!points_json.is_array()) {
        return Status::Error("Points must be an array");
    }

    // Extract the points
    for (const auto& p : points_json) {
        if (!p.contains("id") || !p.contains("vector")) {
            return Status::Error("Point missing id or vector field");
        }

        std::string id = p["id"].get<PointIdType>();
        auto payload = p.value("payload", vectordb::json::object());

        // Schema 1: vector is array (single default vector)
        if (p["vector"].is_array()) {
            auto result = validateVector("default", p["vector"], collection_info);
            if (!result.ok()) {
                return result.status();
            }
            auto status = upsertPoints(collection_name, id, result.value(), payload);
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
                return Status::Error("No valid vectors found for point " + id);
            }

            auto status = upsertPoints(collection_name, id, named_vectors, payload);
            if (!status.ok) { return status; }
        } else {
            return Status::Error("Invalid json vector format for point " + id);
        }
    }

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
            return Status::Error("Vector '" + name + "' must contain only numeric values");
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