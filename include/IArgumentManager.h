#pragma once

#include <string>
#include <cstdint>

/**
 * @class IArgumentManager
 * @brief Interface for parsing command-line arguments.
 *
 * Stores global configuration parameters required by the program.
 */
class IArgumentManager {
public:
    virtual ~IArgumentManager() = default;

    /// Path to S graphs folder
    std::string s_path;

    /// Are the graphs directed
    bool directed = false;

    /// Alive threshold parameter
    double alive_threshold = 0.0;

    /// Single graph mode (find partial pattern instead of full convergence)
    bool single_graph = false;

    /// Score threshold for single graph mode (stop when pattern score < threshold)
    double score_threshold = 0.0;

    /// Path to the background graph G (required in single-graph mode)
    std::string g_path;

    /**
     * @brief Parse command-line arguments.
     */
    virtual void read_arguments(int argc, char** argv) = 0;
};
