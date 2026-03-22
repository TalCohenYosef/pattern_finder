#pragma once

#include "IGraphManager.h"
#include "Graph.h"
#include "BoostGraph.h"
#include <string>

/**
 * @class JsonGraphManager
 * @brief Graph manager for reading and writing JSON format files.
 *
 * Expected JSON format:
 * {
 *   "nodes": [ { "id": int, "color": int } ],
 *   "links": [ { "source": int, "target": int } ]
 * }
 *
 * Notes:
 * - JSON node IDs are mapped internally to Boost vertex descriptors.
 * - Vertex color is stored in VertexProperty::color.
 * - EdgeProperty::is_reversed is set to false by default when reading.
 */
class JsonGraphManager : public IGraphManager {
public:
    /**
     * @brief Constructor.
     */
    JsonGraphManager() = default;

    /**
     * @brief Destructor.
     */
    ~JsonGraphManager() override = default;

    /**
     * @brief Read a graph from a JSON file.
     * @param path Path to JSON file
     * @param directed Whether the graph should be treated as directed
     * @return Constructed Graph object
     * @throws std::runtime_error on file or parse error
     */
    Graph read_graph(const std::string& path, bool directed = false) override;

    /**
     * @brief Write a graph to a JSON file.
     * @param path Output file path
     * @param graph Boost graph to serialize
     * @param is_graph_directed Whether the graph is directed
     * @throws std::runtime_error on file error
     */
    void write_graph(const std::string& path,
                     const BoostGraph& graph,
                     bool is_graph_directed) override;

};
