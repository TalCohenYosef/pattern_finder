#include "JsonGraphManager.h"

#include <nlohmann/json.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>

#include <fstream>
#include <unordered_map>
#include <stdexcept>

using json = nlohmann::json;

/* ---------- Read graph ---------- */

Graph JsonGraphManager::read_graph(const std::string& path)
{
    std::ifstream graph_file(path);
    if (!graph_file.is_open())
        throw std::runtime_error("JsonGraphManager: cannot open input file");

    json graph_json;
    graph_file >> graph_json;

    Graph graph;

    // Map JSON node id → Boost vertex descriptor
    std::unordered_map<int, Graph::vertex_descriptor> id_map;

    /* ---- vertices ---- */
    for (const auto& node : graph_json["nodes"]) {
        int id = node.at("id");
        int color = node.at("color");

        auto v = boost::add_vertex(VertexProperty{color}, graph);
        id_map[id] = v;
    }

    /* ---- edges ---- */
    for (const auto& edge : graph_json["links"]) {
        int src = edge.at("source");
        int tgt = edge.at("target");

        boost::add_edge(
            id_map.at(src),
            id_map.at(tgt),
            EdgeProperty{false},   // default
            graph
        );

        boost::add_edge(
            id_map.at(tgt),
            id_map.at(src),
            EdgeProperty{true},   // default
            graph
        );
    }

    return graph;
}

/* ---------- Write graph ---------- */

void JsonGraphManager::write_graph(const std::string& path,
                                   const Graph& graph)
{
    std::ofstream graph_file(path);
    if (!graph_file.is_open())
        throw std::runtime_error("JsonGraphManager: cannot open output file");

    json graph_json;

    /* ---- vertices ---- */
    for (auto v : boost::make_iterator_range(vertices(graph))) {
        graph_json["nodes"].push_back({
            {"id", static_cast<int>(v)},
            {"color", graph[v].color}
        });
    }

    /* ---- edges ---- */
    for (auto e : boost::make_iterator_range(edges(graph))) {
        graph_json["links"].push_back({
            {"source", static_cast<int>(source(e, graph))},
            {"target", static_cast<int>(target(e, graph))}
        });
    }

    graph_file << graph_json.dump(4);
}
