#include "CMDArgumentManager.h"
#include "JsonGraphManager.h"
#include "PatternFinder.h"

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

/**
 * @brief Load S graphs from disk using JsonGraphManager.
 *
 * Assumes files are named sequentially or all JSON files in a directory.
 */
static std::vector<Graph>
load_s_files(const IArgumentManager& options)
{
    std::vector<Graph> s_list;

    int32_t count = 0;
    for (const auto& entry :
         std::filesystem::directory_iterator(options.s_path)) {

        if (count >= options.s_size)
            break;

        if (entry.path().extension() == ".json") {
            s_list.push_back(
                JsonGraphManager::read_graph(entry.path().string()));
            ++count;
        }
    }

    if (s_list.size() != static_cast<size_t>(options.s_size)) {
        std::cerr << "Warning: loaded "
                  << s_list.size()
                  << " graphs instead of "
                  << options.s_size << "\n";
    }

    return s_list;
}

/**
 * @brief Program entry point.
 */
int main(int32_t argc, char** argv)
{
    try {
        /* ---------- Parse arguments ---------- */
        CMDArgumentManager options;
        options.read_arguments(argc, argv);

        /* ---------- Load input graphs ---------- */
        std::vector<Graph> s_list =
            load_s_files(options);

        /* ---------- Run pattern finder ---------- */
        Graph pattern =
            PatternFinder::find_pattern(
                options.s_size,
                s_list,
                options.alive_threshold);

        /* ---------- Write output ---------- */
        JsonGraphManager::write_graph(
            "pattern.json", pattern);

        std::cout << "Pattern written to pattern.json\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
