#include "JsonGraphManager.h"
#include <json.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <algorithm>

/* =====================
   Read graph from JSON
   ===================== */

Graph JsonGraphManager::read_graph(const std::string& path, bool directed)
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

   void JsonGraphManager::write_graph(const std::string& path, const BoostGraph& graph, bool is_directed)
{
    nlohmann::json root;

    for (auto v : boost::make_iterator_range(vertices(graph)))
    {
        root["nodes"].push_back({{"id", (int)v}, {"color", (int)graph[v].color}});
    }

    for (auto e : boost::make_iterator_range(edges(graph)))
    {
        if (is_directed || source(e, graph) < target(e, graph))
        {
            root["links"].push_back({{"source", (int)source(e, graph)},
                                    {"target", (int)target(e, graph)}});
        }
    }

    std::ofstream f(path);
    if (!f.is_open())
        throw std::runtime_error("JsonGraphManager: cannot open file: " + path);

    f << root.dump(4);
}
