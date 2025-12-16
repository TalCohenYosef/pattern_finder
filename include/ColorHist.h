#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include <unordered_map>

#include "Node.h"
#include "Graph.h"

/**
 * @class ColorHist
 * @brief Maintains a histogram of neighbor appearances by color and pattern depth.
 *
 * The histogram is a 2D structure:
 *
 *     m_number_of_neighbours[color][depth]
 *
 * where:
 *  - color ∈ [0, C)
 *  - depth ∈ [0, MAX_S)
 *
 * This structure is used to decide which color and which depth in the pattern
 * should be extended next, based on the maximum number of compatible neighbors.
 */
class ColorHist {
private:
    /**
     * @brief Histogram matrix.
     *
     * m_number_of_neighbours[c][d] stores how many neighbors of color `c`
     * are compatible with extending the pattern at depth `d`.
     */
    std::vector<std::vector<int32_t>> m_number_of_neighbours;

    /// Number of distinct colors in the graph
    int32_t C;

    /// Maximum pattern size (maximum depth)
    int32_t MAX_S;

public:
    /**
     * @brief Construct a ColorHist object.
     *
     * @param num_colors Number of distinct colors (C)
     * @param max_s Maximum depth / pattern size
     */
    ColorHist(int32_t num_colors, int32_t max_s);

    /**
     * @brief Decrease histogram counts due to removed or invalid neighbors.
     *
     * This function is typically called when extending the tree path causes
     * certain neighbor mappings to become invalid.
     *
     * @param last_node_in_path Last node in the current tree path
     * @param indexes_in_s Indices of candidate nodes in S (unused here, kept for symmetry)
     * @param s_list List of graphs S_i
     * @param update_in_hist Mapping:
     *        - key: index in S
     *        - value: index in pattern P (depth)
     */
    void update_hist_decrease_from_neighbours(
        const NodePtr& last_node_in_path,
        const std::vector<int32_t>& indexes_in_s,
        const std::vector<Graph>& s_list,
        const std::unordered_map<int32_t, int32_t>& update_in_hist
    );

    /**
     * @brief Increase histogram counts when a node is added to the tree.
     *
     * Called after successfully adding a node to the pattern tree,
     * increasing compatibility counts for its neighbors.
     *
     * @param last_node_in_path Newly added node in the tree
     * @param indexes_in_s Indices of candidate nodes in S (unused here)
     * @param s_list List of graphs S_i
     * @param update_in_hist Mapping of neighbors contributing to histogram update
     */
    void update_neigbours_add_node_add_neighbours_to_hist(
        const NodePtr& last_node_in_path,
        const std::vector<int32_t>& indexes_in_s,
        const std::vector<Graph>& s_list,
        const std::unordered_map<int32_t, int32_t>& update_in_hist
    );

    /**
     * @brief Decrease histogram counts when removing a node from the tree.
     *
     * Used during backtracking when a node is removed and its contribution
     * to neighbor compatibility must be reverted.
     *
     * @param node_to_remove Node being removed from the tree
     * @param s_list List of graphs S_i
     * @param update_in_hist Mapping of neighbors affected by removal
     */
    void update_neigbours_remove_node_decrease_neighbours_from_hist(
        const NodePtr& node_to_remove,
        const std::vector<Graph>& s_list,
        const std::unordered_map<int32_t, int32_t>& update_in_hist
    );

    /**
     * @brief Select the best color and depth to extend next.
     *
     * Scans the entire histogram and returns the (color, depth) pair
     * with the maximum number of compatible neighbors.
     *
     * @return std::pair<color, depth>
     *         - color: color index with maximum support
     *         - depth: pattern depth where extension is most promising
     */
    std::pair<int32_t, int32_t> get_color_to_add() const;
};

using ColorHistPtr = std::shared_ptr<ColorHist>;