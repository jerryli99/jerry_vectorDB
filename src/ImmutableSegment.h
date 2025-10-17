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
    ImmutableSegment(const SegPointData& point_data, const CollectionInfo& info)
        : m_info{info}
        , m_index_spec{info.index_specs}
    {
        std::cout << "Hello from Immutabe\n";
        buildHNSWIndexes(point_data);
    }

    ~ImmutableSegment() = default;

    // Prevent copying
    ImmutableSegment(const ImmutableSegment&) = delete;
    ImmutableSegment& operator=(const ImmutableSegment&) = delete;

    ImmutableSegment(ImmutableSegment&&) noexcept = default;
    ImmutableSegment& operator=(ImmutableSegment&&) noexcept = default;

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

    QueryResult searchTopK(const std::string& vector_name,
                           const std::vector<DenseVector>& query_vectors,
                           size_t k) const
    {
        QueryResult query_result;
        // auto start = std::chrono::high_resolution_clock::now();
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
                        // Convert cosine inner product -> similarity if needed
                        if (metric == DistanceMetric::COSINE) {
                            // optional: score = 1.0f - score; // if you want distance
                        }
                        score = std::round(score * 10000.0f) / 10000.0f;
                        batch.push_back({*point_id, score});
                    }
                }
            }

            query_result.status = Status::OK();
        } catch (const std::exception& e) {
            query_result.status = Status::Error(std::string("Search failed: ") + e.what());
        }

        // auto end = std::chrono::high_resolution_clock::now();
        // query_result.time_seconds = std::chrono::duration<double>(end - start).count();
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

private:
    std::vector<PointIdType> m_point_ids;
    std::unordered_map<VectorName, std::unique_ptr<faiss::IndexHNSW>> m_hnsw_indexes;
    std::unordered_map<VectorName, size_t> m_vector_dims;
    CollectionInfo m_info;
    IndexSpec m_index_spec;
    IdTracker m_id_tracker;
};

} // namespace vectordb

