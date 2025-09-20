#pragma once

#include "DataTypes.h"
#include "Point.h"
#include "CollectionInfo.h"
#include "Status.h"
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

namespace vectordb {

template<size_t TinyMapCapacity>
class ImmutableSegment {
public:
    // Constructor from points
    ImmutableSegment(const std::vector<Point<TinyMapCapacity>*>& points,
                     const IndexSpec& index_spec)
        : m_index_spec(index_spec)
    {
        buildHNSWIndexes(points);
    }

    //Constructor from disk, use this for search
    ImmutableSegment(const std::string& segment_path) {
        loadFromDisk(segment_path);
    }

    ~ImmutableSegment() = default;

    //prevent copying
    ImmutableSegment(const ImmutableSegment&) = delete;
    ImmutableSegment& operator=(const ImmutableSegment&) = delete;

    ImmutableSegment(ImmutableSegment&&) noexcept = default;
    ImmutableSegment& operator=(ImmutableSegment&&) noexcept = default;

    // std::vector<SearchResult> search(const DenseVector& query,
    //                                 size_t top_k,
    //                                 const VectorName& vector_name = "default") const
    // {
    //     std::vector<SearchResult> results;
        
    //     auto it = m_hnsw_indexes.find(vector_name);
    //     if (it == m_hnsw_indexes.end()) {
    //         return results; // Empty if vector type not found
    //     }

    //     const auto& index = it->second;

    //     std::vector<faiss::idx_t> labels(top_k);
    //     std::vector<float> distances(top_k);

    //     index->search(1, query.data(), top_k, distances.data(), labels.data());

    //     results.reserve(top_k);
    //     for (size_t i = 0; i < top_k; ++i) {
    //         if (labels[i] >= 0 && static_cast<size_t>(labels[i]) < m_point_ids.size()) {
    //             results.emplace_back(m_point_ids[labels[i]], distances[i]);
    //         }
    //     }
    //     return results;
    // }

    // std::vector<SearchResult> search(const std::map<VectorName, DenseVector>& queries,
    //                                 size_t top_k) const
    // {
    //     std::vector<SearchResult> all_results;
        
    //     for (const auto& [vector_name, query] : queries) {
    //         auto results = search(query, top_k, vector_name);
    //         all_results.insert(all_results.end(), results.begin(), results.end());
    //     }

    //     std::sort(all_results.begin(), all_results.end(),
    //             [](const SearchResult& a, const SearchResult& b) {
    //                 return a.distance < b.distance;
    //             });

    //     auto last = std::unique(all_results.begin(), all_results.end(),
    //                         [](const SearchResult& a, const SearchResult& b) {
    //                             return a.point_id == b.point_id;
    //                         });
    //     all_results.erase(last, all_results.end());

    //     if (all_results.size() > top_k) {
    //         all_results.resize(top_k);
    //     }
    //     return all_results;
    // }

    // Write segment to disk
    Status writeToDisk(const std::string& segment_path) const {
        try {
            std::filesystem::create_directories(segment_path);

            // Save metadata
            std::ofstream meta_file(segment_path + "/metadata.txt");
            if (!meta_file) {
                return Status::Error("Failed to create metadata file");
            }

            meta_file << "point_count: " << m_point_ids.size() << "\n";
            meta_file << "tiny_map_capacity: " << TinyMapCapacity << "\n";
            meta_file << "index_threshold: " << m_index_spec.index_threshold << "\n";
            meta_file << "m_edges: " << m_index_spec.m_edges << "\n";
            meta_file << "ef_construction: " << m_index_spec.ef_construction << "\n";
            meta_file << "ef_search: " << m_index_spec.ef_search << "\n";
            meta_file << "wait_indexing: " << m_index_spec.wait_indexing << "\n";

            // Save point IDs
            std::ofstream id_file(segment_path + "/point_ids.bin", std::ios::binary);
            if (!id_file) {
                return Status::Error("Failed to create point IDs file");
            }
            id_file.write(reinterpret_cast<const char*>(m_point_ids.data()),
                        m_point_ids.size() * sizeof(PointIdType));

            // Save vector dimensions
            std::ofstream dim_file(segment_path + "/vector_dims.bin", std::ios::binary);
            if (!dim_file) {
                return Status::Error("Failed to create vector dimensions file");
            }
            for (const auto& [name, dim] : m_vector_dims) {
                size_t name_length = name.size();
                dim_file.write(reinterpret_cast<const char*>(&name_length), sizeof(size_t));
                dim_file.write(name.c_str(), name_length);
                dim_file.write(reinterpret_cast<const char*>(&dim), sizeof(size_t));
            }

            // Save Faiss indexes
            for (const auto& [name, index] : m_hnsw_indexes) {
                std::string index_path = segment_path + "/index_" + name + ".faiss";
                faiss::write_index(index.get(), index_path.c_str());
            }

            return Status::OK();

        } catch (const std::exception& e) {
            return Status::Error(std::string("Failed to write segment to disk: ") + e.what());
        }
    }

    // Get statistics
    size_t getPointCount() const { return m_point_ids.size(); }
    const std::vector<PointIdType>& getPointIds() const { return m_point_ids; }
    const std::unordered_map<VectorName, size_t>& getVectorDimensions() const { return m_vector_dims; }
    const IndexSpec& getIndexSpec() const { return m_index_spec; }
    size_t getTinyMapCapacity() const { return TinyMapCapacity; }

    bool shouldMerge() const {
        return m_point_ids.size() < m_index_spec.index_threshold * 2;
    }

private:
    void buildHNSWIndexes(const std::vector<Point<TinyMapCapacity>*>& points) {
        m_point_ids.reserve(points.size());
        for (const auto* point : points) {
            m_point_ids.push_back(point->getId());
        }

        std::unordered_map<VectorName, std::vector<const DenseVector*>> vector_data;

        for (const auto* point : points) {
            auto all_vectors = point->getAllVectors();
            for (const auto& [name, vec] : all_vectors) {
                vector_data[name].push_back(&vec);
                if (m_vector_dims.find(name) == m_vector_dims.end()) {
                    m_vector_dims[name] = vec.size();
                }
            }
        }

        for (const auto& [name, vectors] : vector_data) {
            size_t dim = m_vector_dims[name];
            size_t num_vectors = vectors.size();

            auto index = std::make_unique<faiss::IndexHNSWFlat>(dim, m_index_spec.m_edges);
            index->hnsw.efConstruction = m_index_spec.ef_construction;
            index->hnsw.efSearch = m_index_spec.ef_search;

            std::vector<float> batch_data;
            batch_data.reserve(num_vectors * dim);

            for (const auto* vec_ptr : vectors) {
                batch_data.insert(batch_data.end(), vec_ptr->begin(), vec_ptr->end());
            }

            index->add(num_vectors, batch_data.data());

            m_hnsw_indexes[name] = std::move(index);
        }
    }

    void loadFromDisk(const std::string& segment_path) {
        std::ifstream meta_file(segment_path + "/metadata.txt");
        if (!meta_file) {
            throw std::runtime_error("Failed to open metadata file");
        }

        std::ifstream id_file(segment_path + "/point_ids.bin", std::ios::binary);
        if (!id_file) {
            throw std::runtime_error("Failed to open point IDs file");
        }

        id_file.seekg(0, std::ios::end);
        size_t file_size = id_file.tellg();
        id_file.seekg(0, std::ios::beg);

        size_t point_count = file_size / sizeof(PointIdType);
        m_point_ids.resize(point_count);
        id_file.read(reinterpret_cast<char*>(m_point_ids.data()), file_size);

        std::ifstream dim_file(segment_path + "/vector_dims.bin", std::ios::binary);
        if (!dim_file) {
            throw std::runtime_error("Failed to open vector dimensions file");
        }

        while (dim_file) {
            size_t name_length;
            if (!dim_file.read(reinterpret_cast<char*>(&name_length), sizeof(size_t))) break;

            std::string name(name_length, '\0');
            dim_file.read(&name[0], name_length);

            size_t dim;
            dim_file.read(reinterpret_cast<char*>(&dim), sizeof(size_t));

            m_vector_dims[name] = dim;
        }

        for (const auto& [name, dim] : m_vector_dims) {
            std::string index_path = segment_path + "/index_" + name + ".faiss";
            faiss::Index* index = faiss::read_index(index_path.c_str());
            m_hnsw_indexes[name] = std::unique_ptr<faiss::IndexHNSW>(
                dynamic_cast<faiss::IndexHNSW*>(index));

            if (!m_hnsw_indexes[name]) {
                throw std::runtime_error("Failed to load HNSW index for vector: " + name);
            }
        }
    }

private:
    std::vector<PointIdType> m_point_ids;
    std::unordered_map<VectorName, std::unique_ptr<faiss::IndexHNSW>> m_hnsw_indexes;
    std::unordered_map<VectorName, size_t> m_vector_dims;
    IndexSpec m_index_spec;
};

} // namespace vectordb
