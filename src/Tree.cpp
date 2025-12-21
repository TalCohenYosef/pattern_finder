#include "Tree.h"
#include <stdexcept>
#include <algorithm>
#include <vector>

/* ---------- Constructor ---------- */

Tree::Tree(int32_t s_index, ColorHistPtr hist)
    : depth(0), hist(hist)
{
    m_root = std::make_shared<Node>(s_index, 0);
}

/* ---------- Destructor ---------- */

// Tree::~Tree()
// {
//     if (!m_root) return;

//     std::stack<NodePtr> stack;
//     stack.push(m_root);

//     while (!stack.empty()) {
//         NodePtr v = stack.top();
//         stack.pop();

//         NodePtr current_son = v->son;
//         while (current_son) {
//             stack.push(current_son);
//             current_son = current_son->left;
//         }

//         v->parent.reset();
//         v->son.reset();
//         v->left.reset();
//         v->right.reset();
//     }

//     m_root.reset();
// }

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

    if( m_root == node ) {
        m_root.reset();
    }
}

std::vector<uint32_t> Tree::_get_neighbours_in_tree_path(
    std::vector<uint32_t> indexes_in_s, 
    const std::vector<Graph>& s_list,
    std::unordered_map<uint32_t, uint32_t> path_in_tree)
{
    // return all the neighbours of the indexes in s that are also in the tree path
    std::vector<uint32_t> neighbours_in_s_in_tree_path;

    const Graph& graph = s_list[this->m_root->index];

    std::unordered_set<int32_t> neighbours;
    for (uint32_t index_in_s : indexes_in_s)
    {
        auto src_vertex = static_cast<Graph::vertex_descriptor>(index_in_s);

        for (auto edge : boost::make_iterator_range(boost::out_edges(src_vertex, graph))) 
        {
            auto neighbour = boost::target(edge, graph);
            uint32_t neighbour_index = static_cast<uint32_t>(neighbour);
            neighbours.insert(neighbour_index);        
        }
    }

    for (auto index_neighbours = neighbours.begin(); index_neighbours != neighbours.end(); index_neighbours++)
    {
        if (path_in_tree.find((*index_neighbours)) != path_in_tree.end())
        {
            neighbours_in_s_in_tree_path.push_back(path_in_tree[(*index_neighbours)]-1);
        }
    }
    return neighbours_in_s_in_tree_path;
}


std::vector<uint32_t> Tree::_get_colors_of_neighbours_not_in_tree_path(
     std::vector<uint32_t> indexes_in_s, 
     const std::vector<Graph>& s_list,
     std::unordered_map<uint32_t, uint32_t> path_in_tree)
{
    // return all the neighbours of the indexes in s that are also in the tree path
    std::vector<uint32_t> neighbours_in_s_not_in_tree_path;

    std::vector<std::pair<uint32_t, uint32_t>> neighbours;
    for (uint32_t index_in_s : indexes_in_s)
    {
        auto src_vertex = static_cast<Graph::vertex_descriptor>(index_in_s);

        for (auto edge : boost::make_iterator_range(boost::out_edges(src_vertex, s_list[this->m_root->index]))) 
        {
            auto neighbour = boost::target(edge, s_list[this->m_root->index]);
            uint32_t neighbour_index = static_cast<uint32_t>(neighbour);
            neighbours.push_back(std::make_pair(neighbour_index, s_list[this->m_root->index][neighbour].color));
        }
    } 

    for (auto index_neighbours = neighbours.begin(); index_neighbours != neighbours.end(); index_neighbours++)
    {
        if (path_in_tree.find((*index_neighbours).first) == path_in_tree.end())
        {
            neighbours_in_s_not_in_tree_path.push_back((*index_neighbours).second);
        }
    }
    
    return neighbours_in_s_not_in_tree_path;
}


/* ---------- Public API ---------- */

std::unordered_map<uint32_t, uint32_t>
Tree::get_tree_path_map(const NodePtr& last_node_in_path)
{
    std::unordered_map<uint32_t, uint32_t> path;
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
Tree::add_tree_level(const std::vector<std::pair<uint32_t, NodePtr>>& new_indexes,
                     const std::vector<Graph>& s_list)
{
    std::vector<NodePtr> added_nodes;

    if (!new_indexes.empty()) {
        // initial path in tree
        std::unordered_map<uint32_t, uint32_t> path_in_tree =
            get_tree_path_map(new_indexes[0].second);

        depth = depth + 1;

        for (std::pair<uint32_t, NodePtr> idx : new_indexes)
            added_nodes.push_back(_add_node(idx.second, idx.first));

        // update histogram
        int new_child_index = 0;
        NodePtr last_parent_node = nullptr;
        while (new_child_index < new_indexes.size())
        {
            // get all children of the same parent
            std::vector<uint32_t> new_indexes_same_parent = {new_indexes[new_child_index].first};
            NodePtr current_parent = new_indexes[new_child_index].second;
            while(new_child_index + 1 < new_indexes.size() && new_indexes[new_child_index + 1].second == current_parent)
            {
                new_indexes_same_parent.push_back(new_indexes[new_child_index + 1].first);
                new_child_index++;
            }
            new_child_index++;

            // update tree path for parent
            if (last_parent_node != nullptr)
            {
                NodePtr last_parent_iterate = last_parent_node;
                NodePtr current_parent_iterate = current_parent;
                while(last_parent_iterate != current_parent_iterate)
                {
                    path_in_tree[current_parent_iterate->index] = current_parent_iterate->depth;
                    path_in_tree.erase(last_parent_iterate->index);
                    current_parent_iterate = current_parent_iterate->parent.lock();
                    last_parent_iterate = last_parent_iterate->parent.lock();
                }
                last_parent_node = current_parent;
            }

            const std::vector<uint32_t> update_in_hist1 = 
                _get_neighbours_in_tree_path(new_indexes_same_parent, s_list, path_in_tree);
            
            uint32_t color = s_list[this->m_root->index][new_indexes[0].first].color;
            hist->update_hist_decrease_from_neighbours(
                color,update_in_hist1);

            const std::vector<uint32_t> update_in_hist2 =
                _get_colors_of_neighbours_not_in_tree_path(new_indexes_same_parent, s_list, path_in_tree);

            hist->update_neigbours_add_node_add_neighbours_to_hist(
                this->depth-1, update_in_hist2);
        }
    }

    return added_nodes;
}

void Tree::remove_node(const NodePtr& node,
                       const std::vector<Graph>& s_list)
{
    NodePtr node_to_remove = node;
    std::unordered_map<uint32_t, uint32_t> path_in_tree =
        get_tree_path_map(node_to_remove);

    while (node_to_remove) {
        if (node_to_remove->depth != 0)
        {
            const std::vector<uint32_t> update_in_hist =
                _get_colors_of_neighbours_not_in_tree_path({node_to_remove->index}, s_list, path_in_tree);

            hist->update_neigbours_remove_node_decrease_neighbours_from_hist(
                node_to_remove->depth-1, update_in_hist);
        }
        NodePtr parent = node_to_remove->parent.lock();
        _delete_node(node_to_remove);

        if (parent && !parent->son)
        {
            path_in_tree.erase(node_to_remove->index);
            node_to_remove = parent;
        }
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
