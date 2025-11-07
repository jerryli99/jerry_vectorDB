#pragma once

#include "DataTypes.h"

#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>

/*
The graph will only store the vector id and the relationship like 
"similar_to", "contains", "derived_from", the error handling in this class is ignored 
for prototyping.
The graph is implemented using adjacency list.

(11/7/2025) Intended to be in Collection level, and not tested for now. Just experimenting. Still learning
more about graphs as well.

That's all.
*/

#pragma once

#include "DataTypes.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>


namespace vectordb {

struct GraphNode {
    PointIdType vector_id;
    std::string named_vector;
};

struct GraphEdge {
    PointIdType from_id;
    PointIdType to_id;
    std::string relationship;
    float weight = 1.0f;
    
    // Helper for JSON serialization
    json to_json() const {
        return {
            {"from_id", from_id},
            {"to_id", to_id},
            {"relationship", relationship},
            {"weight", weight}
        };
    }
};

class VectorGraph {
public:
    void add_node(PointIdType vector_id, const std::string& named_vector) {
        m_nodes[vector_id] = {vector_id, named_vector};
    }

    bool node_exists(PointIdType vector_id) const {
        return m_nodes.find(vector_id) != m_nodes.end();
    }

    //add relationship with weight - weight indicates relationship strength
    void add_relationship(PointIdType from_id, PointIdType to_id, 
                          const std::string& relationship, float weight = 1.0f) 
    {
        if (!node_exists(from_id)) add_node(from_id, "unknown");
        if (!node_exists(to_id)) add_node(to_id, "unknown");

        GraphEdge edge{from_id, to_id, relationship, weight};
        m_outgoing_edges[from_id].push_back(edge);
        m_incoming_edges[to_id].push_back(edge);
        
        // Store all edges for quick lookup
        m_all_edges.push_back(edge);
    }

    //get weighted outgoing connections (filter by min weight if needed..)
    std::vector<std::pair<PointIdType, float>> get_outgoing_connections(PointIdType vector_id, 
                                                                       float min_weight = 0.0f) {
        std::vector<std::pair<PointIdType, float>> result;
        if (m_outgoing_edges.find(vector_id) != m_outgoing_edges.end()) {
            for (const auto& edge : m_outgoing_edges[vector_id]) {
                if (edge.weight >= min_weight) {
                    result.push_back({edge.to_id, edge.weight});
                }
            }
        }
        return result;
    }

    //find shortest path using Dijkstra (considering weights)
    std::vector<PointIdType> find_shortest_path(PointIdType start_id, PointIdType end_id) {
        std::unordered_map<PointIdType, float> distances;
        std::unordered_map<PointIdType, PointIdType> previous;
        std::priority_queue<std::pair<float, PointIdType>, 
                           std::vector<std::pair<float, PointIdType>>,
                           std::greater<>> pq;

        for (const auto& node : m_nodes) {
            distances[node.first] = std::numeric_limits<float>::max();
        }
        
        distances[start_id] = 0.0f;
        pq.push({0.0f, start_id});

        while (!pq.empty()) {
            auto [current_dist, current_id] = pq.top();
            pq.pop();

            if (current_id == end_id) break;

            for (const auto& edge : m_outgoing_edges[current_id]) {
                float new_dist = current_dist + (1.0f / edge.weight); // Higher weight = shorter distance
                if (new_dist < distances[edge.to_id]) {
                    distances[edge.to_id] = new_dist;
                    previous[edge.to_id] = current_id;
                    pq.push({new_dist, edge.to_id});
                }
            }
        }

        // Reconstruct path
        std::vector<PointIdType> path;
        for (PointIdType at = end_id; at != start_id; at = previous[at]) {
            path.push_back(at);
            if (previous.find(at) == previous.end()) return {}; // No path
        }
        path.push_back(start_id);
        std::reverse(path.begin(), path.end());
        return path;
    }

    // Get strongly connected nodes by weight
    // Depends on the users definition of what weight threshold is about of course, i am just putting a random value here
    std::vector<PointIdType> get_strongly_connected(PointIdType vector_id, float weight_threshold = 0.7f) {
        std::vector<PointIdType> result;
        
        // Check outgoing edges with high weight
        for (const auto& edge : m_outgoing_edges[vector_id]) {
            if (edge.weight >= weight_threshold) {
                result.push_back(edge.to_id);
            }
        }
        
        // Check incoming edges with high weight
        for (const auto& edge : m_incoming_edges[vector_id]) {
            if (edge.weight >= weight_threshold) {
                result.push_back(edge.from_id);
            }
        }
        
        return result;
    }

    // Serialize graph to JSON
    json to_json() const {
        json result = {
            {"nodes", json::object()},
            {"edges", json::array()}
        };
        
        for (const auto& [id, node] : m_nodes) {
            result["nodes"][id] = {
                {"vector_id", node.vector_id},
                {"named_vector", node.named_vector}
            };
        }
        
        for (const auto& edge : m_all_edges) {
            result["edges"].push_back(edge.to_json());
        }
        
        return result;
    }

    std::vector<PointIdType> traverse_outwards(PointIdType start_id, int max_hops = 2) {
        return bfs_traversal(start_id, max_hops, true);
    }
    
    std::vector<PointIdType> traverse_inwards(PointIdType start_id, int max_hops = 2) {
        return bfs_traversal(start_id, max_hops, false);
    }
    
    std::vector<PointIdType> find_by_relationship(PointIdType vector_id, 
                                                 const std::string& relationship,
                                                 bool outgoing = true) {
        std::vector<PointIdType> result;
        auto& edges = outgoing ? m_outgoing_edges[vector_id] : m_incoming_edges[vector_id];
        
        for (const auto& edge : edges) {
            if (edge.relationship == relationship) {
                result.push_back(outgoing ? edge.to_id : edge.from_id);
            }
        }
        return result;
    }
    
    std::vector<GraphEdge> get_node_relationships(PointIdType vector_id) const {
        std::vector<GraphEdge> result;
        
        // Use find() instead of operator[] for const correctness
        auto outgoing_it = m_outgoing_edges.find(vector_id);
        if (outgoing_it != m_outgoing_edges.end()) {
            result.insert(result.end(), outgoing_it->second.begin(), outgoing_it->second.end());
        }
        
        auto incoming_it = m_incoming_edges.find(vector_id);
        if (incoming_it != m_incoming_edges.end()) {
            result.insert(result.end(), incoming_it->second.begin(), incoming_it->second.end());
        }
        
        return result;
    }
    
private:
    std::unordered_map<PointIdType, GraphNode> m_nodes;
    std::unordered_map<PointIdType, std::vector<GraphEdge>> m_outgoing_edges;
    std::unordered_map<PointIdType, std::vector<GraphEdge>> m_incoming_edges;
    std::vector<GraphEdge> m_all_edges; // For serialization

    std::vector<PointIdType> bfs_traversal(PointIdType start_id, int max_hops, bool outwards) {
        // Your existing BFS implementation
        std::vector<PointIdType> results;
        std::unordered_set<PointIdType> visited;
        std::queue<std::pair<PointIdType, int>> queue;

        if (!node_exists(start_id)) return results;

        queue.push({start_id, 0});
        visited.insert(start_id);

        while (!queue.empty()) {
            auto [current_id, hops] = queue.front();
            queue.pop();
            
            if (hops > 0) results.push_back(current_id);
            
            if (hops < max_hops) {
                auto& connections = outwards ? m_outgoing_edges : m_incoming_edges;
                if (connections.find(current_id) != connections.end()) {
                    for (const auto& edge : connections[current_id]) {
                        PointIdType next_id = outwards ? edge.to_id : edge.from_id;
                        if (visited.find(next_id) == visited.end()) {
                            visited.insert(next_id);
                            queue.push({next_id, hops + 1});
                        }
                    }
                }
            }
        }
        return results;
    }
};

} // namespace vectordb
