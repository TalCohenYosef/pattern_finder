#pragma once

#include "Node.h"
#include "Graph.h"
#include "IndevidualColorHist.h"

#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>
#include <cstdint>
#include <atomic>

/**
 * @class Tree
 * @brief Dynamic tree structure for pattern expansion and backtracking.
 *
 * Uses a first-child / sibling representation:
 * - son   : first child
 * - left  : left sibling
 * - right : right sibling
 * - parent: weak_ptr to avoid ownership cycles
 */
class Tree {
private:
    std::vector<Node> m_tree;          ///< Root node of the tree
    std::vector<uint32_t> m_children_start_index;
    std::vector<uint32_t> m_parent_index;  ///< Parent index for each node
    IndevidualColorHist m_hist;       ///< Color histogram for heuristic updates

private:

    void _add_node(const uint32_t node_parent, const int32_t index_in_s);

    /**
     * @brief Delete a leaf node from the tree.
     */
    void _delete_node(const uint32_t node);

    /**
    * @brief Get neighbors in S that are already in the tree path.
    * @return Vector of pairs (depth in pattern, color pf vertex)
    */
    void _update_neighbours_in_tree_path(
        std::vector<uint32_t> indexes_in_s, 
        const std::vector<Graph>& s_list,
        std::unordered_map<uint32_t, uint32_t> path_in_tree,
        std::unordered_multimap<uint32_t,uint32_t>& found_neibours_in_tree_path);

    /**
    * @brief Get neighbors in S that are not in the tree path.
    * @return Vector ( color of vertex)
    */
    std::vector<uint32_t> _get_colors_of_neighbours_not_in_tree_path(
        std::vector<uint32_t> indexes_in_s, 
        const std::vector<Graph>& s_list,
        std::unordered_map<uint32_t, uint32_t> path_in_tree);


public:
    /**
     * @brief Construct tree with a root node.
     */
    Tree(int32_t s_index, GeneralColorHist& general_hist);

    /**
     * @brief Destructor clears the entire tree.
     */
    //~Tree();
    ~Tree()=default;

    /**
     * @brief Get map of pattern index → depth along a tree path.
     */
    std::unordered_map<uint32_t, uint32_t>
    get_tree_path_map(const uint32_t last_node_in_path);


    /// @return True if tree is empty
    bool is_empty();

    /**
     * @brief Add a new level under a parent node.
     */
    std::pair<uint32_t, uint32_t>
    add_tree_level(const std::vector<std::pair<uint32_t, uint32_t>>& new_indexes,
                   const std::vector<Graph>& s_list);

    /**
     * @brief Remove a node and backtrack if needed.
     */
    void remove_node(const uint32_t node,
                     const std::vector<Graph>& s_list);

    /**
     * @brief Get ancestor of a node at a given depth.
     */
    uint32_t get_node_by_depth(const uint32_t lowest_node_in_match,
                              int32_t depth);

    bool is_alive(uint32_t node) {
        return this->m_tree[node].is_alive;
    }

    uint32_t get_s_index(uint32_t node) {
        return this->m_tree[node].index;
    }
};
