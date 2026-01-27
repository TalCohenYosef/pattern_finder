#pragma once

#include "Graph.h"
#include "BoostGraph.h"
#include <string>

/**
 * @class JsonGraphManager
 * @brief Static utility class for reading and writing Boost graphs in JSON format.
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
class JsonGraphManager {
public:
    JsonGraphManager() = delete;              // static-only class
    ~JsonGraphManager() = delete;

    /**
     * @brief Read a graph from a JSON file.
     * @param path Path to JSON file
     * @return Constructed Boost graph
     * @throws std::runtime_error on file or parse error
     */
    static Graph read_graph(const std::string& path, const bool directed);

    /**
     * @brief Write a graph to a JSON file.
     * @param path Output file path
     * @param graph Boost graph to serialize
     * @throws std::runtime_error on file error
     */
    static void write_graph(const std::string& path,
                            const BoostGraph& graph);
};
