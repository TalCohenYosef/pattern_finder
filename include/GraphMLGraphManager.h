#pragma once

#include "IGraphManager.h"
#include "Graph.h"
#include "BoostGraph.h"
#include <string>

/**
 * @class GraphMLGraphManager
 * @brief Graph manager for reading and writing GraphML format files.
 *
 * Expected GraphML format:
 * - Standard GraphML with nodes and edges
 * - Node color stored in "color" attribute (optional, defaults to 0)
 * - Node IDs can be any string, mapped internally to sequential indices
 * - Edge direction determined by graph type (directed/undirected)
 */
class GraphMLGraphManager : public IGraphManager {
public:
    /**
     * @brief Constructor.
     */
    GraphMLGraphManager() = default;

    /**
     * @brief Destructor.
     */
    ~GraphMLGraphManager() override = default;

    /**
     * @brief Read a graph from a GraphML file.
     * @param path Path to GraphML file
     * @param directed Whether the graph should be treated as directed
     * @return Constructed Graph object
     * @throws std::runtime_error on file or parse error
     */
    Graph read_graph(const std::string& path, bool directed = false) override;

    /**
     * @brief Write a graph to a GraphML file.
     * @param path Output file path
     * @param graph Boost graph to serialize
     * @throws std::runtime_error on file error
     */
    void write_graph(const std::string& path,
                     const BoostGraph& graph) override;

private:
    /**
     * @brief Extract color attribute from a GraphML node.
     * @param node_data Node data from GraphML parser
     * @return Color value (defaults to 0 if not found)
     */
    int32_t extract_color_from_node(const std::string& node_data);
};
