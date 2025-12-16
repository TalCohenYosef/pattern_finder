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

    /// Number of S graphs to parse
    int32_t s_size = 0;

    /// Alive threshold parameter
    double alive_threshold = 0.0;

    /**
     * @brief Parse command-line arguments.
     */
    virtual void read_arguments(int argc, char** argv) = 0;
};
