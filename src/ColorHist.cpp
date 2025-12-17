#include "ColorHist.h"
#include <algorithm>

/**
 * @brief Constructor initializes the histogram matrix to zeros.
 */
ColorHist::ColorHist(int32_t num_colors): C(num_colors)
{
}

/**
 * @brief Decreases histogram counts for neighbors invalidated by path extension.
 */
void ColorHist::update_hist_decrease_from_neighbours(
    const std::vector<std::pair<uint32_t, uint32_t>>& update_in_hist)
{
    for (const auto& neighbour : update_in_hist) {
        uint32_t color = neighbour.second;
        uint32_t index_in_p = neighbour.first;

        --m_number_of_neighbours[color][index_in_p];       
    }
}

/**
 * @brief Increases histogram counts after adding a new node to the pattern.
 */
void ColorHist::update_neigbours_add_node_add_neighbours_to_hist(
    uint32_t new_node_depth,
    const std::vector<uint32_t>& update_in_hist)
{
    this->m_number_of_neighbours.push_back(std::vector<uint32_t>(C, 0));
    for (const auto& neighbour : update_in_hist) {
        int color = neighbour;
        ++m_number_of_neighbours[color][new_node_depth];
    }
}

/**
 * @brief Decreases histogram counts during node removal (backtracking).
 */
void ColorHist::update_neigbours_remove_node_decrease_neighbours_from_hist(
    uint32_t remove_node_depth,
    const std::vector<uint32_t>& update_in_hist)
{

    for (const auto& neighbour : update_in_hist) {
        int color = neighbour;
        --m_number_of_neighbours[color][remove_node_depth];
    }
}

/**
 * @brief Finds the (color, depth) pair with the maximum histogram value.
 */
std::pair<uint32_t, uint32_t> ColorHist::get_color_to_add() const
{
    int32_t max_count = 0;
    int32_t best_color = -1;
    int32_t best_node_in_p = -1;

    for (int32_t color = 0; color < C; ++color) {
        for (int32_t node_in_p = 0; node_in_p < m_number_of_neighbours.size(); ++node_in_p) {
            if (m_number_of_neighbours[node_in_p][color] > max_count) {
                max_count = m_number_of_neighbours[node_in_p][color];
                best_color = color;
                best_node_in_p = node_in_p;
            }
        }
    }

    return {best_color, best_node_in_p};
}
