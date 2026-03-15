#include "Graph.h"
#include <algorithm>

/* ---------- Construction ---------- */

Graph::Graph(uint32_t vertex_count, std::vector<std::pair<uint32_t, uint32_t>>& edges, std::vector<int32_t>& colors, bool is_directed) : directed(is_directed)
{
    this->colors = colors;
    if (!edges.empty())
    {
        m_edge_count = static_cast<uint32_t>(edges.size());

        if (!directed)
        {
            // add oppiste edges
            size_t edges_number = static_cast<int32_t>(edges.size());
            for(size_t i = 0; i < edges_number; i++)
            {
                edges.emplace_back(edges[i].second, edges[i].first);
            }
            initiate_graph(vertex_count, edges, colors, this->neigbours, this->index_of_neighbours);
        }
        else
        {
            // run function twice for edges and reversed edges
            initiate_graph(vertex_count, edges, colors, this->neigbours, this->index_of_neighbours);
            std::vector<std::pair<uint32_t, uint32_t>> reveresed_edges;
            reveresed_edges.reserve(edges.size());
            for (const auto& edge : edges)
            {
                reveresed_edges.emplace_back(edge.second, edge.first);
            }
            initiate_graph(vertex_count, reveresed_edges, colors, this->reversed_neigbours, this->reversed_index_of_neighbours);
        }

    }
}

void Graph::initiate_graph(const uint32_t vertex_count, std::vector<std::pair<uint32_t, uint32_t>>& edges, const std::vector<int32_t>& colors,
    std::vector<uint32_t>& neigbours, std::vector<uint32_t>& index_of_neighbours) 
{
    std::sort(edges.begin(), edges.end());
    neigbours.reserve(edges.size());
    index_of_neighbours.resize(vertex_count, 0);
    uint32_t current_vertex = edges[0].first;
    for (int i = 0; i < edges.size(); ++i)
    {
        if (edges[i].first != current_vertex)
        {
            for (uint32_t v = current_vertex + 1; v <= edges[i].first; ++v)
            {
                index_of_neighbours[v] = static_cast<uint32_t>(neigbours.size());
            }
            current_vertex = edges[i].first;
        }
        neigbours.push_back(edges[i].second);
    }
    for (uint32_t v = current_vertex + 1; v < vertex_count; ++v)
    {
        index_of_neighbours[v] = static_cast<uint32_t>(neigbours.size());
    }
}

bool Graph::is_edge(uint32_t u, uint32_t v) const
{
    auto [it, end] = this->get_neighbours(u,false);
    return std::find(it, end, v) != end;
}

std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> Graph::get_neighbours(uint32_t vertex, bool reversed) const
{
    if (reversed && directed)
    {
        return {
            this->reversed_neigbours.begin() + this->reversed_index_of_neighbours[vertex],
            (vertex + 1 < this->reversed_index_of_neighbours.size()) ?
                this->reversed_neigbours.begin() + this->reversed_index_of_neighbours[vertex + 1] :
                this->reversed_neigbours.end()
        };
    }
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

uint32_t Graph::edge_count() const
{
    return m_edge_count;
}

uint32_t Graph::get_vertex_color(uint32_t v) const
{
    return colors[v];
}

void Graph::set_vertex_color(uint32_t vertex, int32_t new_color) { 
    colors[vertex] = new_color; 
}