#include "CMDArgumentManager.h"

#include <boost/program_options.hpp>
#include <iostream>
#include <stdexcept>

namespace po = boost::program_options;

void CMDArgumentManager::read_arguments(int argc, char** argv)
{
    po::options_description desc("Allowed options");
    po::variables_map vm;

    desc.add_options()
        ("help,h", "Show help message")
        ("directed", po::bool_switch(&directed), "Treat the graph as directed")
        ("path", po::value<std::string>()->required(), "Path to S graphs folder")
        ("alive", po::value<double>(), "Alive threshold (required unless --single-graph is used)")
        ("single-graph", po::bool_switch(&single_graph), "Single graph mode (find partial pattern)")
        ("min-density", po::value<double>(&min_density)->default_value(0.7), "Minimum density for single graph mode");

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            std::exit(0);
        }

        po::notify(vm);
    }
    catch (const po::error& e) {
        std::cerr << "Argument error: " << e.what() << "\n\n";
        std::cerr << desc << std::endl;
        throw;
    }

    // Assign parsed values
    s_path = vm["path"].as<std::string>();
    
    // Validate alive threshold logic
    if (!single_graph && !vm.count("alive")) {
        throw std::runtime_error("--alive is required when not using --single-graph");
    }
    
    if (vm.count("alive")) {
        alive_threshold = vm["alive"].as<double>();
    }
    
    // Validate single graph mode
    if (single_graph && vm.count("alive")) {
        std::cerr << "Warning: --alive is ignored in single-graph mode" << std::endl;
    }
}
