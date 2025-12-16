#pragma once

#include "IArgumentManager.h"

/**
 * @class CMDArgumentManager
 * @brief Parses command-line arguments using Boost.Program_options.
 */
class CMDArgumentManager : public IArgumentManager {
public:
    /**
     * @brief Parse command-line arguments from argc/argv.
     * @throws std::runtime_error on invalid arguments
     */
    void read_arguments(int argc, char** argv) override;
};
