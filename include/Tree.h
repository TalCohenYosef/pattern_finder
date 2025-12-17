#pragma once

#include "Node.h"
#include "Graph.h"
#include "ColorHist.h"

#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>
#include <cstdint>


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
    NodePtr m_root;          ///< Root node of the tree
    int32_t depth;           ///< Maximum depth reached
    ColorHistPtr hist;       ///< Color histogram for heuristic updates

private:

    /**
     * @brief Add a node as a child of a given parent.
     */
    NodePtr _add_node(const NodePtr& node_parent, int32_t index_in_s);

    /**
     * @brief Delete a leaf node from the tree.
     */
    void _delete_node(const NodePtr& node);

    /**
    * @brief Get neighbors in S that are already in the tree path.
    * @return Vector of pairs (depth in pattern, color pf vertex)
    */
    std::vector<std::pair<uint32_t, uint32_t>> _get_neighbours_in_tree_path(NodePtr last_node_in_path, 
        std::vector<uint32_t> indexes_in_s, std::vector<Graph> s_list);

    /**
    * @brief Get neighbors in S that are not in the tree path.
    * @return Vector ( color of vertex)
    */
    std::vector<uint32_t> _get_neighbours_not_in_tree_path(NodePtr last_node_in_path, 
        std::vector<uint32_t> indexes_in_s, std::vector<Graph> s_list);


public:
    /**
     * @brief Construct tree with a root node.
     */
    Tree(int32_t s_index, ColorHistPtr hist);

    /**
     * @brief Destructor clears the entire tree.
     */
    ~Tree();

    /**
     * @brief Get map of pattern index → depth along a tree path.
     */
    std::unordered_map<int32_t, int32_t>
    get_tree_path_map(const NodePtr& last_node_in_path);

    /// @return Root node
    NodePtr get_root();

    /// @return True if tree is empty
    bool is_empty();

    /**
     * @brief Add a new level under a parent node.
     */
    std::vector<NodePtr>
    add_tree_level(const NodePtr& node_parent,
                   const std::vector<uint32_t>& new_indexes,
                   const std::vector<Graph>& s_list);

    /**
     * @brief Remove a node and backtrack if needed.
     */
    void remove_node(const NodePtr& node,
                     const std::vector<Graph>& s_list);

    /**
     * @brief Get ancestor of a node at a given depth.
     */
    NodePtr get_node_by_depth(const NodePtr& lowest_node_in_match,
                              int32_t depth);
};
