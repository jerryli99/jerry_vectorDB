#pragma once

#include "DataTypes.h"
#include "CollectionInfo.h"
#include "Status.h"
#include "Utils.h"
#include "IdTracker.h"
#include "Distance.h"
#include "QueryResult.h"

#include <faiss/IndexHNSW.h>
#include <faiss/IndexFlat.h>
#include <faiss/index_io.h>
#include <faiss/Clustering.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

namespace vectordb {

class ImmutableSegment {
public:
    // Constructor that takes copied data 
    ImmutableSegment(const SegPointData& point_data, const CollectionInfo& info, const SegmentIdType seg_id)
        : m_info{info}
        , m_index_spec{info.index_specs}
        , m_segment_id{seg_id}
    {
        std::cout << "Hello from ImmutableSegment, ID: " << m_segment_id << "\n";
        buildHNSWIndexes(point_data);
        computeKMeansClusters(point_data);//could use macro to represent maxiterations, now just use default.
        //buildFilterMartix(point_data);??maybe 1 filtermatrix for 1 immutable_seg
    }

    ~ImmutableSegment() = default;

    // Prevent copying
    ImmutableSegment(const ImmutableSegment&) = delete;
    ImmutableSegment& operator=(const ImmutableSegment&) = delete;

    ImmutableSegment(ImmutableSegment&&) noexcept = default;
    ImmutableSegment& operator=(ImmutableSegment&&) noexcept = default;

    const std::string& getSegmentId() const { 
        return m_segment_id; 
    }

    // Get statistics
    size_t getPointCount() const { 
        return m_point_ids.size(); 
    }
    
    const std::vector<PointIdType>& getPointIds() const { 
        return m_point_ids; 
    }
    
    const std::unordered_map<VectorName, size_t>& getVectorDimensions() const { 
        return m_vector_dims; 
    }
    
    const IndexSpec& getIndexSpec() const { 
        return m_index_spec; 
    }

    const IdTracker& getIdTracker() const { 
        return m_id_tracker; 
    }

    const std::map<VectorName, std::vector<DenseVector>>& getCentroids() const {
        return m_centroids;
    }

    const std::vector<DenseVector>& getCentroids(const VectorName& name) const {
        auto it = m_centroids.find(name);
        if (it == m_centroids.end()) {
            throw std::runtime_error("No centroids found for vector space: " + name);
        }
        return it->second;
    }

    //maybe change the path to std::filesystem::path?
    void writeIndex(const std::string& base_path = "./vectordb") {
        try {
            std::cout << "[Writing to disk...]\n";
            // Create collection directory: ./VectorDB/{collection_name}
            std::string collection_dir = base_path + "/" + m_info.name;
            std::filesystem::create_directories(collection_dir);
            
            // Create segments directory: ./VectorDB/{collection_name}/segments
            std::string segments_dir = collection_dir + "/segments";
            std::filesystem::create_directories(segments_dir);
            
            // Create this specific segment directory: ./VectorDB/{collection_name}/segments/{segment_id}
            std::string segment_dir = segments_dir + "/" + m_segment_id;
            std::filesystem::create_directories(segment_dir);
            
            // Write each vector index
            for (auto& [vec_name, index] : m_hnsw_indexes) {
                std::string path = segment_dir + "/" + vec_name + ".index";
                faiss::write_index(index.get(), path.c_str());
                std::cout << "[WRITE] Index written: " << path << "\n";
            }
            
            // Also write segment metadata
            writeSegmentMetadata(segment_dir);
            
        } catch (const std::exception& e) {
            std::cerr << "[WRITE ERROR] " << e.what() << "\n";
            throw; // Re-throw to let caller handle
        }
    }

    // Synchronous load from disk
    void loadIndex(const std::string& base_path = "./vectordb") {
        //need try/catch here?
        for (auto& [vec_name, _] : m_info.vec_specs) {
            std::string path = base_path + "/" + vec_name + ".index";//fix this to match writeInedx(..)
            if (!std::filesystem::exists(path)) continue;

            auto loaded_index = std::unique_ptr<faiss::IndexHNSW>(
                dynamic_cast<faiss::IndexHNSW*>(faiss::read_index(path.c_str()))
            );
            m_hnsw_indexes[vec_name] = std::move(loaded_index);
            std::cout << "[LOAD] Index loaded: " << path << "\n";
        }
        //after loaded need to write searchTopK for this? 
    }

    // -------------------------------
    //ideally, i think i will make K be sqrt(n), where n is the num of points.
    //not sure if this is the best function design, but I can optimize this later.
    //call this method after the HNSW index build. I expect k to be like around 70 or 50 depends. I will just hard code it.
    void computeKMeansClusters(const SegPointData& points, size_t max_iters = 20) {
        std::map<VectorName, std::vector<DenseVector>> per_name_vectors;

        //collect all DenseVectors by VectorName
        for (const auto& [id, vec_map] : points) {
            for (const auto& [name, vec] : vec_map) {
                per_name_vectors[name].push_back(vec);
            }
        }

        //run KMeans per VectorName using FAISS
        for (auto& [name, vectors] : per_name_vectors) {
            if (vectors.empty()) continue;

            size_t dim = vectors[0].size();
            size_t n = vectors.size();

            size_t k = calculateOptimalK(n);

            std::cout << "Running FAISS KMeans on '" << name
                    << "' with " << n << " vectors of dim " << dim << "...\n";

            if (n <= k) {
                std::cout << "Warning: Not enough vectors for proper clustering. Using all vectors as centroids.\n";
                m_centroids[name] = vectors;
                continue;
            }
            //flatten to float*
            std::vector<float> data_flat(n * dim);
            for (size_t i = 0; i < n; ++i) {
                std::copy(vectors[i].begin(), vectors[i].end(),
                        data_flat.begin() + i * dim);
            }

            //fAISS clustering
            faiss::ClusteringParameters cp;
            cp.niter = max_iters;
            cp.verbose = false;

            faiss::Clustering clus(dim, k, cp);
            faiss::IndexFlatL2 index(dim);
            clus.train(n, data_flat.data(), index);

            // Extract centroids
            const float* centroids_flat = clus.centroids.data();
            std::vector<DenseVector> cluster_centroids;
            cluster_centroids.reserve(k);
            for (size_t i = 0; i < k; ++i) {
                DenseVector c(dim);
                std::copy(centroids_flat + i * dim, centroids_flat + (i + 1) * dim, c.begin());
                cluster_centroids.push_back(std::move(c));
            }

            m_centroids[name] = std::move(cluster_centroids);
        }

        // return centroids;
    }

    size_t calculateOptimalK(size_t n) const {
        double k = std::sqrt(static_cast<double>(n));
        
        //i wish i can have a GPU so i can have faster compute time...Alas
        const size_t MIN_CLUSTERS = 5;
        const size_t MAX_CLUSTERS = 100;
        
        size_t optimal_k = static_cast<size_t>(std::round(k));
        optimal_k = std::max(MIN_CLUSTERS, std::min(optimal_k, MAX_CLUSTERS));
        
        std::cout << "Calculated optimal k: sqrt(" << n << ") = " << k 
                  << " -> rounded to " << optimal_k << std::endl;
        
        return optimal_k;
    }
    //---------------------------------------

    QueryResult searchTopK(const std::string& vector_name,
                           const std::vector<DenseVector>& query_vectors,
                           size_t k) const
    {
        QueryResult query_result;
        std::cout << "In immutable searchTopK\n";
        auto it = m_hnsw_indexes.find(vector_name);
        if (it == m_hnsw_indexes.end()) {
            query_result.status = Status::Error("Vector space not found: " + vector_name);
            return query_result;
        }

        if (query_vectors.empty()) {
            query_result.status = Status::Error("No query vectors provided");
            return query_result;
        }

        size_t nq = query_vectors.size();
        size_t dim = m_vector_dims.at(vector_name);

        for (const auto& qvec : query_vectors) {
            if (qvec.size() != dim) {
                query_result.status = Status::Error("Query vector dimension mismatch [@ImmutableSegment]");
                return query_result;
            }
        }

        try {
            auto metric = m_info.vec_specs.at(vector_name).metric;

            // Normalize queries for cosine metric
            std::vector<DenseVector> processed_queries = query_vectors;
            if (metric == DistanceMetric::COSINE) {
                for (auto& qvec : processed_queries) {
                    float query_norm = vectordb::norm(qvec.data(), qvec.size());
                    if (query_norm > 1e-12f) {
                        for (auto& val : qvec)
                            val /= query_norm;
                    }
                }
            }

            //flatten queries
            std::vector<float> flat_queries;
            flat_queries.reserve(nq * dim);
            for (const auto& qvec : processed_queries)
                flat_queries.insert(flat_queries.end(), qvec.begin(), qvec.end());

            //output buffers
            std::vector<faiss::idx_t> indices(nq * k);
            std::vector<float> dists(nq * k);

            //perform FAISS search
            it->second->search(nq, flat_queries.data(), k, dists.data(), indices.data());

            //build QueryResult structure
            query_result.results.resize(nq);
            for (size_t i = 0; i < nq; i++) {
                auto& batch = query_result.results[i].hits;
                batch.reserve(k);

                for (size_t j = 0; j < k; j++) {
                    size_t idx = i * k + j;
                    if (auto point_id = m_id_tracker.getExternalId(vector_name, indices[idx])) {
                        float score = dists[idx];
                        //normalize L2 to match cosine "higher is better"
                        if (metric == DistanceMetric::L2)
                            score = -score;  // negate distance so higher = better
                        score = std::round(score * 10000.0f) / 10000.0f;
                        batch.push_back({*point_id, score});
                    }
                }
            }

            query_result.status = Status::OK();
        } catch (const std::exception& e) {
            query_result.status = Status::Error(std::string("Search failed: ") + e.what());
        }
        return query_result;//return a copy, which is fine.
    }//end of searchTopK here


private:
    void buildHNSWIndexes(const SegPointData& point_data) {
        m_point_ids.reserve(point_data.size());
        
        //get all possible vector names from collection info
        std::vector<VectorName> all_vector_names;
        all_vector_names.reserve(m_info.vec_specs.size());
        for (const auto& [name, _] : m_info.vec_specs) {
            all_vector_names.push_back(name);
        }
        
        //init IdTracker with all possible vector names
        m_id_tracker.init(all_vector_names, point_data.size());

        //first pass: collect point IDs and track which vectors exist
        std::unordered_map<VectorName, size_t> vector_counts;
        std::set<VectorName> existing_vector_names;
        
        for (const auto& [point_id, vectors] : point_data) {
            m_point_ids.push_back(point_id);
            
            // Track existing vectors for this point
            for (const auto& [name, vec] : vectors) {
                m_id_tracker.insert(name, point_id);
                vector_counts[name]++;
                existing_vector_names.insert(name);
                
                // Set dimension if not already set
                if (m_vector_dims.find(name) == m_vector_dims.end()) {
                    m_vector_dims[name] = vec.size();
                } else if (m_vector_dims[name] != vec.size()) {
                    throw std::runtime_error("Dimension mismatch for vector space: " + name);
                }
            }
        }

        //second pass: build batch buffers only for vectors that actually exist
        std::unordered_map<VectorName, std::vector<float>> batch_buffers;
        
        for (const auto& name : existing_vector_names) {
            size_t dim = m_vector_dims[name];
            size_t count = vector_counts[name];
            batch_buffers[name].reserve(count * dim);
        }

        //fill batch buffers in point order (important for ID mapping consistency)
        for (const auto& [point_id, vectors] : point_data) {
            for (const auto& name : existing_vector_names) {
                auto it = vectors.find(name);
                if (it != vectors.end()) {
                    const auto& vec = it->second;
                    auto& buf = batch_buffers[name];
                    // buf.insert(buf.end(), vec.begin(), vec.end());
                    //normalize vectors for cosine metric during build
                    if (m_info.vec_specs.at(name).metric == DistanceMetric::COSINE) {
                        DenseVector normalized_vec = vec;
                        float vec_norm = vectordb::norm(normalized_vec.data(), normalized_vec.size());
                        if (vec_norm > 1e-12f) {
                            for (auto& val : normalized_vec) {
                                val /= vec_norm;
                            }
                        }
                        buf.insert(buf.end(), normalized_vec.begin(), normalized_vec.end());
                    } else {
                        //for L2 and DOT, store vectors as-is
                        buf.insert(buf.end(), vec.begin(), vec.end());
                    }//
                }
            }
        }

        // Build FAISS indexes only for vector spaces that have data
        for (const auto& [name, buf] : batch_buffers) {
            size_t dim = m_vector_dims[name];
            size_t num_vectors = buf.size() / dim;

            if (num_vectors == 0) {
                continue; // Skip empty vector spaces
            }

            std::cout << "Building HNSW index for vector space: " << name 
                      << " with " << num_vectors << " vectors of dimension " << dim << std::endl;

            auto faiss_metric = to_faiss_metric(m_info.vec_specs.at(name).metric);
            auto index = std::make_unique<faiss::IndexHNSWFlat>(dim, m_index_spec.m_edges, faiss_metric);
            index->hnsw.efConstruction = m_index_spec.ef_construction;
            index->hnsw.efSearch = m_index_spec.ef_search;

            // Add vectors to the index
            index->add(num_vectors, buf.data());
            m_hnsw_indexes[name] = std::move(index);
        }

        std::cout << "ImmutableSegment: built " << m_hnsw_indexes.size() 
                  << " HNSW indexes for " << m_point_ids.size() << " points\n";
        
        // Debug: print vector space statistics
        for (const auto& [name, count] : vector_counts) {
            std::cout << "  Vector space '" << name << "': " << count << " vectors" << std::endl;
        }
    }
    
    
    void writeSegmentMetadata(const std::string& segment_dir) {
        std::string metadata_path = segment_dir + "/metadata.json";
        std::ofstream metadata_file(metadata_path);
        
        json metadata;
        metadata["segment_id"] = m_segment_id;
        metadata["collection_name"] = m_info.name;
        metadata["point_count"] = m_point_ids.size();
        metadata["created_at"] = getCurrentTimestamp();
        metadata["vector_spaces"] = json::object();
        
        for (const auto& [vec_name, dim] : m_vector_dims) {
            metadata["vector_spaces"][vec_name] = {
                {"dimension", dim},
                {"metric", to_string(m_info.vec_specs.at(vec_name).metric)}
            };
        }
        
        metadata_file << metadata.dump(4);
        metadata_file.close();
        std::cout << "[METADATA] Segment metadata written: " << metadata_path << "\n";
    }
    
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    SegmentIdType m_segment_id;
    std::vector<PointIdType> m_point_ids;
    std::unordered_map<VectorName, std::unique_ptr<faiss::IndexHNSW>> m_hnsw_indexes;
    std::unordered_map<VectorName, size_t> m_vector_dims;
    CollectionInfo m_info;
    IndexSpec m_index_spec;
    IdTracker m_id_tracker;
    
    //MetaIndex centroids
    std::map<VectorName, std::vector<DenseVector>> m_centroids;
};

} // namespace vectordb

