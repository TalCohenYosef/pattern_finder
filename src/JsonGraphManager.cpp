#include <json-c/json.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>

#include <unordered_map>
#include <stdexcept>
#include <string>
#include "Graph.h"


/* =====================
   JSON Graph Manager
   ===================== */

class JsonGraphManager {
public:
    static Graph read_graph(const std::string& path);
    static void write_graph(const std::string& path, const Graph& graph);
};

/* =====================
   Read graph from JSON
   ===================== */

Graph JsonGraphManager::read_graph(const std::string& path)
{
    json_object* root = json_object_from_file(path.c_str());
    if (!root)
        throw std::runtime_error("JsonGraphManager: cannot open or parse JSON file");

    json_object* nodes = nullptr;
    json_object* links = nullptr;

    if (!json_object_object_get_ex(root, "nodes", &nodes) ||
        !json_object_object_get_ex(root, "links", &links))
    {
        json_object_put(root);
        throw std::runtime_error("JsonGraphManager: invalid JSON format (missing nodes/links)");
    }

    Graph graph;
    std::unordered_map<int, Graph::vertex_descriptor> id_map;

    /* ---- vertices ---- */
    const int num_nodes = json_object_array_length(nodes);
    for (int i = 0; i < num_nodes; ++i) {
        json_object* node = json_object_array_get_idx(nodes, i);

        json_object* id_obj = nullptr;
        json_object* color_obj = nullptr;

        if (!json_object_object_get_ex(node, "id", &id_obj) ||
            !json_object_object_get_ex(node, "color", &color_obj))
        {
            json_object_put(root);
            throw std::runtime_error("JsonGraphManager: invalid node entry");
        }

        int id = json_object_get_int(id_obj);
        int color = json_object_get_int(color_obj);

        auto v = boost::add_vertex(
            VertexProperty{static_cast<uint32_t>(color)},
            graph
        );

        id_map[id] = v;
    }

    /* ---- edges ---- */
    const int num_edges = json_object_array_length(links);
    for (int i = 0; i < num_edges; ++i) {
        json_object* edge = json_object_array_get_idx(links, i);

        json_object* src_obj = nullptr;
        json_object* tgt_obj = nullptr;

        if (!json_object_object_get_ex(edge, "source", &src_obj) ||
            !json_object_object_get_ex(edge, "target", &tgt_obj))
        {
            json_object_put(root);
            throw std::runtime_error("JsonGraphManager: invalid edge entry");
        }

        int src = json_object_get_int(src_obj);
        int tgt = json_object_get_int(tgt_obj);

        boost::add_edge(
            id_map.at(src),
            id_map.at(tgt),
            EdgeProperty{false},
            graph
        );
    }

    json_object_put(root); // free JSON tree
    return graph;
}

/* =====================
   Write graph to JSON
   ===================== */

void JsonGraphManager::write_graph(const std::string& path,
                                   const Graph& graph)
{
    json_object* root = json_object_new_object();
    json_object* nodes = json_object_new_array();
    json_object* links = json_object_new_array();

    /* ---- vertices ---- */
    for (auto v : boost::make_iterator_range(vertices(graph))) {
        json_object* node = json_object_new_object();

        json_object_object_add(
            node, "id",
            json_object_new_int(static_cast<int>(v))
        );

        json_object_object_add(
            node, "color",
            json_object_new_int(graph[v].color)
        );

        json_object_array_add(nodes, node);
    }

    /* ---- edges ---- */
    for (auto e : boost::make_iterator_range(edges(graph))) {
        json_object* edge = json_object_new_object();

        json_object_object_add(
            edge, "source",
            json_object_new_int(static_cast<int>(source(e, graph)))
        );

        json_object_object_add(
            edge, "target",
            json_object_new_int(static_cast<int>(target(e, graph)))
        );

        json_object_array_add(links, edge);
    }

    json_object_object_add(root, "nodes", nodes);
    json_object_object_add(root, "links", links);

    if (json_object_to_file_ext(
            path.c_str(),
            root,
            JSON_C_TO_STRING_PRETTY) != 0)
    {
        json_object_put(root);
        throw std::runtime_error("JsonGraphManager: failed to write JSON file");
    }

    json_object_put(root); // free JSON tree
}
