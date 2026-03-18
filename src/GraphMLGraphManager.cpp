#include "GraphMLGraphManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <regex>

/* =====================
   Read graph from GraphML
   ===================== */

Graph GraphMLGraphManager::read_graph(const std::string& path, bool directed)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("GraphMLGraphManager: cannot open file: " + path);
    }

    std::string line;
    std::vector<std::pair<uint32_t, uint32_t>> edges;
    std::vector<int32_t> colors;
    std::unordered_map<std::string, uint32_t> node_id_to_index;
    uint32_t next_vertex_index = 0;
    bool file_directed = false;

    // Parse the GraphML file
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Check if graph is directed in file
        if (line.find("edgedefault=\"directed\"") != std::string::npos) {
            file_directed = true;
        }

        // Parse node
        if (line.find("<node") != std::string::npos) {
            // Extract node ID
            std::regex node_id_regex(R"(id\s*=\s*\"([^\"]+)\")");
            std::smatch match;
            if (std::regex_search(line, match, node_id_regex)) {
                std::string node_id = match[1].str();
                node_id_to_index[node_id] = next_vertex_index++;
                colors.push_back(0); // Default color
            }
        }

        // Parse node data (color)
        if (line.find("<data") != std::string::npos && line.find("color") != std::string::npos) {
            // This is a simplified approach - in a full implementation, 
            // we'd need to track which node this data belongs to
            std::regex color_regex(R"(<data[^>]*>.*?(\d+).*?</data>)");
            std::smatch match;
            if (std::regex_search(line, match, color_regex)) {
                int32_t color = std::stoi(match[1].str());
                if (!colors.empty()) {
                    colors.back() = color;
                }
            }
        }

        // Parse edge
        if (line.find("<edge") != std::string::npos) {
            std::regex edge_regex(R"(source\s*=\s*\"([^\"]+)\".*?target\s*=\s*\"([^\"]+)\")");
            std::smatch match;
            if (std::regex_search(line, match, edge_regex)) {
                std::string source_id = match[1].str();
                std::string target_id = match[2].str();
                
                auto source_it = node_id_to_index.find(source_id);
                auto target_it = node_id_to_index.find(target_id);
                
                if (source_it != node_id_to_index.end() && target_it != node_id_to_index.end()) {
                    edges.emplace_back(source_it->second, target_it->second);
                }
            }
        }
    }

    file.close();

    if (edges.empty() && colors.empty()) {
        throw std::runtime_error("GraphMLGraphManager: no valid graph data found in file: " + path);
    }

    return Graph(next_vertex_index, edges, colors, directed || file_directed);
}

/* =====================
   Write graph to GraphML
   ===================== */

void GraphMLGraphManager::write_graph(const std::string& path,
                                      const BoostGraph& graph)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("GraphMLGraphManager: cannot create file: " + path);
    }

    // Write GraphML header
    file << R"(<?xml version="1.0" encoding="UTF-8"?>)" << std::endl;
    file << R"(<graphml xmlns="http://graphml.graphdrawing.org/xmlns")" << std::endl;
    file << R"(         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << std::endl;
    file << R"(         xsi:schemaLocation="http://graphml.graphdrawing.org/xmlns)" << std::endl;
    file << R"(         http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd">)" << std::endl;
    
    // Define color attribute
    file << R"(  <key id="color" for="node" attr.name="color" attr.type="int"/>)" << std::endl;

    // Write graph
    file << R"(  <graph id="G" edgedefault=")" << (boost::num_edges(graph) > 0 ? "directed" : "undirected") << R"(">)" << std::endl;

    // Write nodes
    auto vertices = boost::vertices(graph);
    for (auto it = vertices.first; it != vertices.second; ++it) {
        auto vertex = *it;
        auto& vertex_prop = graph[vertex];
        file << R"(    <node id="v)" << vertex << R"(">)" << std::endl;
        file << R"(      <data key="color">)" << vertex_prop.color << R"(</data>)" << std::endl;
        file << R"(    </node>)" << std::endl;
    }

    // Write edges
    auto edges = boost::edges(graph);
    for (auto it = edges.first; it != edges.second; ++it) {
        auto edge = *it;
        auto source = boost::source(edge, graph);
        auto target = boost::target(edge, graph);
        file << R"(    <edge source="v)" << source << R"(" target="v)" << target << R"("/>)" << std::endl;
    }

    file << R"(  </graph>)" << std::endl;
    file << R"(</graphml>)" << std::endl;

    file.close();
}

/* =====================
   Helper functions
   ===================== */

int32_t GraphMLGraphManager::extract_color_from_node(const std::string& node_data)
{
    std::regex color_regex(R"(<data[^>]*color[^>]*>(\d+)</data>)");
    std::smatch match;
    if (std::regex_search(node_data, match, color_regex)) {
        return std::stoi(match[1].str());
    }
    return 0; // Default color
}
