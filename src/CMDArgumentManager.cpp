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
        ("alive", po::value<double>()->required(), "Alive threshold");

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
    alive_threshold = vm["alive"].as<double>();
}
