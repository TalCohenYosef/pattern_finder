#include "JsonGraphManager.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <fstream>

#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>

/* =====================
   Read graph from JSON
   ===================== */

Graph JsonGraphManager::read_graph(const std::string& path, const bool directed)
{
    boost::property_tree::ptree root;
    
    try {
        boost::property_tree::read_json(path, root);
    } catch (const std::exception& e) {
        throw std::runtime_error("JsonGraphManager: cannot open JSON: " + std::string(e.what()));
    }

    // Check if nodes and links exist
    if (root.find("nodes") == root.not_found() || root.find("links") == root.not_found()) {
        throw std::runtime_error("JsonGraphManager: missing nodes/links");
    }

    /* ---- colors ---- */
    std::vector<int32_t> colors;
    std::unordered_map<int, uint32_t> id_to_index;
    uint32_t node_index = 0;

    for (const auto& node_pair : root.get_child("nodes")) {
        const auto& node = node_pair.second;
        
        int id = node.get<int>("id");
        int color = node.get<int>("color");

        id_to_index[id] = node_index;
        colors.push_back(static_cast<int32_t>(color));
        ++node_index;
    }

    const uint32_t num_nodes = node_index;

    /* ---- edges ---- */
    std::vector<std::pair<uint32_t, uint32_t>> edges;

    for (const auto& edge_pair : root.get_child("links")) {
        const auto& edge = edge_pair.second;
        
        int src = edge.get<int>("source");
        int tgt = edge.get<int>("target");

        uint32_t src_idx = id_to_index.at(src);
        uint32_t tgt_idx = id_to_index.at(tgt);

        edges.emplace_back(src_idx, tgt_idx);
    }

    return Graph(num_nodes, edges, colors, directed);
}


/* =====================
   Write graph to JSON
   ===================== */

   void JsonGraphManager::write_graph(
    const std::string& path,
    const BoostGraph& graph)
{
    boost::property_tree::ptree root;
    boost::property_tree::ptree nodes;
    boost::property_tree::ptree links;

    /* ---- vertices ---- */
    for (auto v : boost::make_iterator_range(vertices(graph))) {
        boost::property_tree::ptree node;
        node.put<int>("id", static_cast<int>(v));
        node.put<int>("color", static_cast<int>(graph[v].color));
        nodes.push_back(std::make_pair("", node));
    }

    /* ---- edges ---- */
    for (auto e : boost::make_iterator_range(edges(graph))) {
        auto u = source(e, graph);
        auto v = target(e, graph);

        // avoid duplicating undirected edges
        if (u < v) {
            boost::property_tree::ptree edge;
            edge.put<int>("source", static_cast<int>(u));
            edge.put<int>("target", static_cast<int>(v));
            links.push_back(std::make_pair("", edge));
        }
    }

    root.add_child("nodes", nodes);
    root.add_child("links", links);

    try {
        // Write JSON with proper numeric values
        std::ofstream json_file(path);
        if (!json_file.is_open()) {
            throw std::runtime_error("JsonGraphManager: failed to open file for writing: " + path);
        }
        
        json_file << "{\n";
        json_file << "    \"nodes\": [\n";
        
        // Write nodes
        bool first_node = true;
        for (auto v : boost::make_iterator_range(vertices(graph))) {
            if (!first_node) json_file << ",\n";
            json_file << "        {\n";
            json_file << "            \"id\": " << static_cast<int>(v) << ",\n";
            json_file << "            \"color\": " << static_cast<int>(graph[v].color) << "\n";
            json_file << "        }";
            first_node = false;
        }
        json_file << "\n    ],\n";
        
        json_file << "    \"links\": [\n";
        
        // Write edges
        bool first_edge = true;
        for (auto e : boost::make_iterator_range(edges(graph))) {
            auto u = source(e, graph);
            auto v = target(e, graph);
            
            // avoid duplicating undirected edges
            if (u < v) {
                if (!first_edge) json_file << ",\n";
                json_file << "        {\n";
                json_file << "            \"source\": " << static_cast<int>(u) << ",\n";
                json_file << "            \"target\": " << static_cast<int>(v) << "\n";
                json_file << "        }";
                first_edge = false;
            }
        }
        json_file << "\n    ]\n";
        json_file << "}\n";
        
        json_file.close();
    } catch (const std::exception& e) {
        throw std::runtime_error("JsonGraphManager: failed to write JSON: " + std::string(e.what()));
    }
}

