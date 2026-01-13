#include "Graph.h"
#include <algorithm>

/* ---------- Construction ---------- */

Graph::Graph(uint32_t vertex_count, std::vector<std::pair<uint32_t, uint32_t>>& edges, std::vector<int32_t>& colors)
{
    this->colors = colors;
    if (!edges.empty())
    {
        std::sort(edges.begin(), edges.end());
        this->neigbours.reserve(edges.size());
        this->index_of_neighbours.resize(vertex_count, 0);
        uint32_t current_vertex = edges[0].first;
        for (int i = 0; i < edges.size(); ++i)
        {
            if (edges[i].first != current_vertex)
            {
                current_vertex = edges[i].first;
                this->index_of_neighbours[current_vertex] = static_cast<uint32_t>(this->neigbours.size());
            }
            this->neigbours.push_back(edges[i].second);
        }
    }
}

bool Graph::is_edge(uint32_t u, uint32_t v) const
{
    auto [it, end] = this->get_neighbours(u);
    return std::find(it, end, v) != end;
}

std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> Graph::get_neighbours(uint32_t vertex) const
{
    return {
        this->neigbours.begin() + this->index_of_neighbours[vertex],
        (vertex + 1 < this->index_of_neighbours.size()) ?
            this->neigbours.begin() + this->index_of_neighbours[vertex + 1] :
            this->neigbours.end()
    };
}

uint32_t Graph::vertex_count() const
{
    return static_cast<uint32_t>(colors.size());
}

uint32_t Graph::get_vertex_color(uint32_t v) const
{
    return colors[v];
}

void Graph::set_vertex_color(uint32_t vertex, int32_t new_color) { 
    colors[vertex] = new_color; 
}