#include "CMDArgumentManager.h"
#include "JsonGraphManager.h"
#include "GraphMLGraphManager.h"
#include "MultiGraphPatternFinder.h"
#include "SingleGraphPatternFinder.h"

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <memory>

/**
 * @brief Load S graphs and record file name for each S[i]
 */
static std::pair<std::vector<Graph>, std::vector<std::string>> load_s_files(const IArgumentManager& options)
{
    std::vector<Graph> s_list;
    std::vector<std::string> names;

    // Create appropriate graph manager based on format
    std::unique_ptr<IGraphManager> graph_manager;
    if (options.graph_format == "json") {
        graph_manager = std::make_unique<JsonGraphManager>();
    } else if (options.graph_format == "graphml") {
        graph_manager = std::make_unique<GraphMLGraphManager>();
    } else {
        throw std::runtime_error("Unsupported graph format: " + options.graph_format);
    }

    // Check if path exists
    if (!std::filesystem::exists(options.s_path)) {
        throw std::runtime_error("Path does not exist: " + options.s_path);
    }

    if (options.single_graph) {
        // Single graph mode: --path points to a single file
        if (!std::filesystem::is_regular_file(options.s_path)) {
            throw std::runtime_error("In single-graph mode, --path must point to a file, not a directory: " + options.s_path);
        }

        std::string extension = std::filesystem::path(options.s_path).extension().string();
        std::string filename = std::filesystem::path(options.s_path).filename().string();

        // Check if file extension matches the expected format
        if ((options.graph_format == "json" && extension == ".json") ||
            (options.graph_format == "graphml" && (extension == ".graphml" || extension == ".xml"))) {
            
            try {
                s_list.push_back(graph_manager->read_graph(options.s_path, options.directed));
                names.push_back(filename);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to load " + filename + ": " + e.what());
            }
        } else {
            throw std::runtime_error("File extension does not match expected format " + options.graph_format + ": " + options.s_path);
        }
    } else {
        // Multiple graphs mode: --path points to a directory
        if (!std::filesystem::is_directory(options.s_path)) {
            throw std::runtime_error("In multi-graph mode, --path must point to a directory, not a file: " + options.s_path);
        }

        int32_t count = 0;

        // Load files with correct extension
        for (const auto& entry : std::filesystem::directory_iterator(options.s_path)) {
            std::string extension = entry.path().extension().string();
            std::string filename = entry.path().filename().string();

            // Check if file extension matches the expected format
            if ((options.graph_format == "json" && extension == ".json") ||
                (options.graph_format == "graphml" && (extension == ".graphml" || extension == ".xml"))) {
                
                try {
                    s_list.push_back(graph_manager->read_graph(entry.path().string(), options.directed));
                    names.push_back(filename);
                    ++count;
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to load " << filename << ": " << e.what() << std::endl;
                }
            }
        }
        
        if (count == 0) {
            std::string msg = "No valid graph files found in directory: " + options.s_path + 
                             " with format: " + options.graph_format;
            throw std::runtime_error(msg);
        }
    }
    
    return {s_list, names};
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
        auto [s_list, s_names] = load_s_files(options);
           
        
            // for (int idx=0; idx<s_names.size(); idx++) {
            //     std::cout << "S[" << idx << "] -> " 
            //                 << s_names[idx] << "\n";
            // }
        // for(int i = 0; i < 4; i++)
        // {
        //     std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> n = s_list[0].get_neighbours(i);
        //     for(auto j = n.first; j != n.second; ++j)
        //     {
        //         std::cout << "Neighbour of " << i << ": " << *j << std::endl;
        //     }
        // }

        /* ---------- Run pattern finder ---------- */
        BoostGraph pattern;
        std::unordered_set<uint32_t> alive_indexes;

         std::unique_ptr<IGraphManager> graph_manager;
        if (options.graph_format == "json") {
            graph_manager = std::make_unique<JsonGraphManager>();
        } else if (options.graph_format == "graphml") {
            graph_manager = std::make_unique<GraphMLGraphManager>();
        } else {
            throw std::runtime_error("Unsupported graph format: " + options.graph_format);
        }

        if (options.single_graph) {
            Graph g = graph_manager->read_graph(options.g_path, options.directed);
            SingleGraphPatternFinder sgpf;
            pattern =
                sgpf.find_pattern(s_list[0], g, options.score_threshold, options.directed);
        } else {
            std::tie(pattern, alive_indexes) =
                MultiGraphPatternFinder::find_pattern(
                    s_list,
                    options.alive_threshold,
                    options.directed,
                    false);
        }
    
        
        if (!alive_indexes.empty()) {
            // Copy unordered_set to vector and sort
            std::vector<int> alive_sorted(alive_indexes.begin(), alive_indexes.end());
            std::sort(alive_sorted.begin(), alive_sorted.end());
        
            std::cout << "\nPattern appears in " 
                        << alive_sorted.size() 
                        << " graphs:\n";
        
            for (int idx : alive_sorted) {
                if (idx >= 0 && idx < static_cast<int>(s_names.size())) {
                    std::cout << "S[" << idx << "] -> " 
                                << s_names[idx] << "\n";
                }
            }
        } else if (!options.single_graph) {
            std::cout << "No S graphs survived.\n";
        }
                
                std::string output_name = "pattern.json";

        if (options.single_graph && !s_names.empty()) {
            std::filesystem::path s_file(s_names[0]);
            output_name = "pattern_" + s_file.stem().string() + ".json";
        }

        graph_manager->write_graph(output_name, pattern, options.directed);

        std::cout << "Pattern written to " << output_name << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
