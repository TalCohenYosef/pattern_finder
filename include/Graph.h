#pragma once

#include <vector>
#include <cstdint>
/**
 * @class Graph
 * @brief Concrete graph class implementing the IGraph interface.
 *
 * This class represents an undirected graph with integer vertex IDs.
 */
class Graph {
public:
    Graph(uint32_t vertex_count, std::vector<std::pair<uint32_t, uint32_t>>& edges, std::vector<int32_t>& colors, bool directed = false);
    ~Graph() = default;

    // ---- IGraph interface ----
    std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> get_neighbours(uint32_t vertex, bool reversed = false) const;
    bool is_edge(uint32_t u, uint32_t v) const;
    uint32_t vertex_count() const;
    uint32_t get_vertex_color(uint32_t v) const;
    void set_vertex_color(uint32_t vertex, int32_t new_color);

private:
    static void initiate_graph(const uint32_t vertex_count, std::vector<std::pair<uint32_t, uint32_t>>& edges, const std::vector<int32_t>& colors,
        std::vector<uint32_t>& neigbours, std::vector<uint32_t>& index_of_neighbours);

private:
    std::vector<uint32_t> neigbours;
    std::vector<uint32_t> index_of_neighbours;

    std::vector<uint32_t> reversed_neigbours;
    std::vector<uint32_t> reversed_index_of_neighbours;

    std::vector<int32_t> colors;
    bool directed;
};
