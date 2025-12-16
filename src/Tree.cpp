#include "Tree.h"
#include <stdexcept>
#include <algorithm>

/* ---------- Constructor ---------- */

Tree::Tree(int32_t s_index, ColorHistPtr hist)
    : depth(0), hist(hist)
{
    m_root = std::make_shared<Node>(s_index, 0);
}

/* ---------- Destructor ---------- */

Tree::~Tree()
{
    if (!m_root) return;

    std::stack<NodePtr> stack;
    stack.push(m_root);

    while (!stack.empty()) {
        NodePtr v = stack.top();
        stack.pop();

        NodePtr current_son = v->son;
        while (current_son) {
            stack.push(current_son);
            current_son = current_son->left;
        }

        v->parent.reset();
        v->son.reset();
        v->left.reset();
        v->right.reset();
    }

    m_root.reset();
}

/* ---------- Private methods ---------- */

NodePtr Tree::_add_node(const NodePtr& node_parent, int32_t index_in_s)
{
    if (!node_parent)
        throw std::runtime_error("Parent is null");

    NodePtr new_node = std::make_shared<Node>(index_in_s,
                                              node_parent->depth + 1);
    new_node->parent = node_parent;

    // parent has no children
    if (!node_parent->son) {
        node_parent->son = new_node;
    }
    // parent has exactly one child
    else if (!node_parent->son->left) {
        node_parent->son->left = new_node;
        node_parent->son->right = new_node;
        new_node->left = node_parent->son;
        new_node->right = node_parent->son;
    }
    // parent has two or more children
    else {
        new_node->right = node_parent->son->right;
        new_node->right->left = new_node;
        node_parent->son->right = new_node;
        new_node->left = node_parent->son;
    }

    return new_node;
}

void Tree::_delete_node(const NodePtr& node)
{
    if (node->son)
        throw std::runtime_error("Cannot delete node with children");

    if (node->left) {
        if (node->right == node->left)
            node->left->right.reset();
        else
            node->left->right = node->right;
    }

    if (node->right) {
        if (node->right == node->left)
            node->right->left.reset();
        else
            node->right->left = node->left;
    }

    if (auto parent = node->parent.lock()) {
        if (parent->son == node)
            parent->son = node->left;
    }
}

/* ---------- Public API ---------- */

std::unordered_map<int32_t, int32_t>
Tree::get_tree_path_map(const NodePtr& last_node_in_path)
{
    std::unordered_map<int32_t, int32_t> path;
    NodePtr current = last_node_in_path;

    while (current && !current->parent.expired()) {
        path[current->index] = current->depth;
        current = current->parent.lock();
    }

    return path;
}

NodePtr Tree::get_root()
{
    return m_root;
}

bool Tree::is_empty()
{
    return m_root == nullptr;
}

std::vector<NodePtr>
Tree::add_tree_level(const NodePtr& node_parent,
                     const std::vector<int32_t>& new_indexes,
                     const std::vector<Graph>& s_list)
{
    std::vector<NodePtr> added_nodes;

    if (!new_indexes.empty()) {
        depth = std::max(depth, node_parent->depth + 1);

        for (int32_t idx : new_indexes)
            added_nodes.push_back(_add_node(node_parent, idx));

        auto update_in_hist =
            _get_neighbours_in_tree_path(node_parent, new_indexes, s_list);

        hist->update_hist_decrease_from_neighbours(
            node_parent, new_indexes, s_list, update_in_hist);

        update_in_hist =
            _get_neighbours_not_in_tree_path(node_parent, new_indexes, s_list);

        hist->update_neigbours_add_node_add_neighbours_to_hist(
            node_parent, new_indexes, s_list, update_in_hist);
    }
    else {
        remove_node(node_parent, s_list);
    }

    return added_nodes;
}

void Tree::remove_node(const NodePtr& node,
                       const std::vector<Graph>& s_list)
{
    NodePtr node_to_remove = node;

    while (node_to_remove) {
        auto update_in_hist =
            _get_neighbours_not_in_tree_path(node_to_remove, {}, s_list);

        hist->update_neigbours_remove_node_decrease_neighbours_from_hist(
            node_to_remove, s_list, update_in_hist);

        NodePtr parent = node_to_remove->parent.lock();
        _delete_node(node_to_remove);

        if (parent && !parent->son)
            node_to_remove = parent;
        else
            break;
    }
}

NodePtr Tree::get_node_by_depth(const NodePtr& lowest_node_in_match,
                                int32_t target_depth)
{
    NodePtr current = lowest_node_in_match;

    while (current && current->depth != target_depth)
        current = current->parent.lock();

    return current;
}
