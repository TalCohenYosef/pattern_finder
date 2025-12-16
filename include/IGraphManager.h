#pragma once

#include <string>
#include "Graph.h"

/**
 * @class IGraphManager
 * @brief Interface for graph serialization and deserialization.
 */
class IGraphManager {
public:
    virtual ~IGraphManager() = default;

    /**
     * @brief Read a graph from a file.
     */
    virtual Graph read_graph(const std::string& path) = 0;

    /**
     * @brief Write a graph to a file.
     */
    virtual void write_graph(const std::string& path,
                             const Graph& graph) = 0;
};
