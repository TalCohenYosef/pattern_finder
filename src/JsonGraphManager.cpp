#include "JsonGraphManager.h"
#include <json-c/json.h>

#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>

/* =====================
   Read graph from JSON
   ===================== */

Graph JsonGraphManager::read_graph(const std::string& path, const bool directed)
{
    json_object* root = json_object_from_file(path.c_str());
    if (!root)
        throw std::runtime_error("JsonGraphManager: cannot open JSON");

    json_object* nodes = nullptr;
    json_object* links = nullptr;

    if (!json_object_object_get_ex(root, "nodes", &nodes) ||
        !json_object_object_get_ex(root, "links", &links))
    {
        json_object_put(root);
        throw std::runtime_error("JsonGraphManager: missing nodes/links");
    }

    const uint32_t num_nodes = json_object_array_length(nodes);

    /* ---- colors ---- */
    std::vector<int32_t> colors(num_nodes);
    std::unordered_map<int, uint32_t> id_to_index;

    for (uint32_t i = 0; i < num_nodes; ++i) {
        json_object* node = json_object_array_get_idx(nodes, i);

        json_object* id_obj = nullptr;
        json_object* color_obj = nullptr;

        if (!json_object_object_get_ex(node, "id", &id_obj) ||
            !json_object_object_get_ex(node, "color", &color_obj))
        {
            json_object_put(root);
            throw std::runtime_error("Invalid node entry");
        }

        int id = json_object_get_int(id_obj);
        int color = json_object_get_int(color_obj);

        id_to_index[id] = i;
        colors[i] = static_cast<uint32_t>(color);
    }

    /* ---- edges ---- */
    std::vector<std::pair<uint32_t, uint32_t>> edges;

    const uint32_t num_edges = json_object_array_length(links);
    
        edges.reserve(num_edges); 
    
    for (uint32_t i = 0; i < num_edges; ++i) {
        json_object* edge = json_object_array_get_idx(links, i);

        json_object* src_obj = nullptr;
        json_object* tgt_obj = nullptr;

        if (!json_object_object_get_ex(edge, "source", &src_obj) ||
            !json_object_object_get_ex(edge, "target", &tgt_obj))
        {
            json_object_put(root);
            throw std::runtime_error("Invalid edge entry");
        }

        uint32_t src = id_to_index.at(json_object_get_int(src_obj));
        uint32_t tgt = id_to_index.at(json_object_get_int(tgt_obj));

        // if undirected → add both
        edges.emplace_back(src, tgt);
    }

    json_object_put(root);

    return Graph(num_nodes, edges, colors, directed);
}


/* =====================
   Write graph to JSON
   ===================== */

   void JsonGraphManager::write_graph(
    const std::string& path,
    const BoostGraph& graph)
{
    json_object* root  = json_object_new_object();
    json_object* nodes = json_object_new_array();
    json_object* links = json_object_new_array();

    /* ---- vertices ---- */
    for (auto v : boost::make_iterator_range(vertices(graph))) {
        json_object* node = json_object_new_object();

        json_object_object_add(
            node,
            "id",
            json_object_new_int(static_cast<int>(v))
        );

        json_object_object_add(
            node,
            "color",
            json_object_new_int(graph[v].color)
        );

        json_object_array_add(nodes, node);
    }

    /* ---- edges ---- */
    for (auto e : boost::make_iterator_range(edges(graph))) {
        auto u = source(e, graph);
        auto v = target(e, graph);

        // avoid duplicating undirected edges
        if (u < v) {
            json_object* edge = json_object_new_object();

            json_object_object_add(
                edge,
                "source",
                json_object_new_int(static_cast<int>(u))
            );

            json_object_object_add(
                edge,
                "target",
                json_object_new_int(static_cast<int>(v))
            );

            json_object_array_add(links, edge);
        }
    }

    json_object_object_add(root, "nodes", nodes);
    json_object_object_add(root, "links", links);

    if (json_object_to_file_ext(
            path.c_str(),
            root,
            JSON_C_TO_STRING_PRETTY) != 0)
    {
        json_object_put(root);
        throw std::runtime_error("JsonGraphManager: failed to write JSON");
    }

    json_object_put(root);
}

