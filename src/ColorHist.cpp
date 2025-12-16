#include "ColorHist.h"
#include <algorithm>

/**
 * @brief Constructor initializes the histogram matrix to zeros.
 */
ColorHist::ColorHist(int32_t num_colors, int32_t max_s)
    : C(num_colors), MAX_S(max_s)
{
    m_number_of_neighbours.resize(
        C, std::vector<int32_t>(MAX_S, 0)
    );
}

/**
 * @brief Decreases histogram counts for neighbors invalidated by path extension.
 */
void ColorHist::update_hist_decrease_from_neighbours(
    const NodePtr& last_node_in_path,
    const std::vector<int32_t>& /*indexes_in_s*/,
    const std::vector<Graph>& s_list,
    const std::unordered_map<int32_t, int32_t>& update_in_hist)
{
    int root_index = last_node_in_path->index;

    for (const auto& neighbour : update_in_hist) {
        int index_in_s = neighbour.first;
        int index_in_p = neighbour.second;

        int color =
            s_list[root_index].nodes[index_in_s].color;

        --m_number_of_neighbours[color][index_in_p];
    }
}

/**
 * @brief Increases histogram counts after adding a new node to the pattern.
 */
void ColorHist::update_neigbours_add_node_add_neighbours_to_hist(
    const NodePtr& last_node_in_path,
    const std::vector<int32_t>& /*indexes_in_s*/,
    const std::vector<Graph>& s_list,
    const std::unordered_map<int32_t, int32_t>& update_in_hist)
{
    int root_index = last_node_in_path->index;
    int depth = last_node_in_path->depth + 1;

    for (const auto& neighbour : update_in_hist) {
        int index_in_s = neighbour.first;

        int color =
            s_list[root_index].nodes[index_in_s].color;

        ++m_number_of_neighbours[color][depth];
    }
}

/**
 * @brief Decreases histogram counts during node removal (backtracking).
 */
void ColorHist::update_neigbours_remove_node_decrease_neighbours_from_hist(
    const NodePtr& node_to_remove,
    const std::vector<Graph>& s_list,
    const std::unordered_map<int32_t, int32_t>& update_in_hist)
{
    int root_index = node_to_remove->index;
    int depth = node_to_remove->depth;

    for (const auto& neighbour : update_in_hist) {
        int index_in_s = neighbour.first;

        int color =
            s_list[root_index].nodes[index_in_s].color;

        --m_number_of_neighbours[color][depth];
    }
}

/**
 * @brief Finds the (color, depth) pair with the maximum histogram value.
 */
std::pair<int32_t, int32_t> ColorHist::get_color_to_add() const
{
    int32_t max_count = 0;
    int32_t best_color = -1;
    int32_t best_node_in_p = -1;

    for (int32_t color = 0; color < C; ++color) {
        for (int32_t node_in_p = 0; node_in_p < MAX_S; ++node_in_p) {
            if (m_number_of_neighbours[color][node_in_p] > max_count) {
                max_count = m_number_of_neighbours[color][node_in_p];
                best_color = color;
                best_node_in_p = node_in_p;
            }
        }
    }

    return {best_color, best_node_in_p};
}
