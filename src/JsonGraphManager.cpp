#include "JsonGraphManager.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
        node.put("id", static_cast<int>(v));
        node.put("color", graph[v].color);
        nodes.push_back(std::make_pair("", node));
    }

    /* ---- edges ---- */
    for (auto e : boost::make_iterator_range(edges(graph))) {
        auto u = source(e, graph);
        auto v = target(e, graph);

        // avoid duplicating undirected edges
        if (u < v) {
            boost::property_tree::ptree edge;
            edge.put("source", static_cast<int>(u));
            edge.put("target", static_cast<int>(v));
            links.push_back(std::make_pair("", edge));
        }
    }

    root.add_child("nodes", nodes);
    root.add_child("links", links);

    try {
        boost::property_tree::write_json(path, root);
    } catch (const std::exception& e) {
        throw std::runtime_error("JsonGraphManager: failed to write JSON: " + std::string(e.what()));
    }
}

