#include "CMDArgumentManager.h"
#include "JsonGraphManager.h"
#include "PatternFinder.h"

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

/**
 * @brief Load S graphs and record file name for each S[i]
 */
static std::pair<std::vector<Graph>, std::vector<std::string>> load_s_files(const IArgumentManager& options)
{
    std::vector<Graph> s_list;
    std::vector<std::string> names;

    int32_t count = 0;
    for (const auto& entry :
         std::filesystem::directory_iterator(options.s_path)) {

        if (entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            s_list.push_back(JsonGraphManager::read_graph(
                entry.path().string(), options.directed));
            names.push_back(filename);
            ++count;
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

        for(int i = 0; i < 4; i++)
        {
            std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> n = s_list[0].get_neighbours(i);
            for(auto j = n.first; j != n.second; ++j)
            {
                std::cout << "Neighbour of " << i << ": " << *j << std::endl;
            }
        }

        /* ---------- Run pattern finder ---------- */
        auto [pattern, alive_indexes] =
            PatternFinder::find_pattern(
                s_list,
                options.alive_threshold);
    
        
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
        } else {
            std::cout << "No S graphs survived.\n";
        }
                
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
