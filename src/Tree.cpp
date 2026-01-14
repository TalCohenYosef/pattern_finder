#include "Tree.h"
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <iostream>
#include <atomic>
#include <unordered_set>
#include <queue>

/* ---------- Constructor ---------- */

Tree::Tree(int32_t s_index, GeneralColorHist& general_hist): m_hist(IndevidualColorHist(general_hist))
{
    this->m_tree.push_back(Node(s_index, 0));
    this->m_parent_index.push_back(UINT32_MAX); // root has no parent
    this->m_children_start_index.push_back(UINT32_MAX);
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

void Tree::_add_node(const uint32_t node_parent, const int32_t index_in_s)
{
    if (this->m_children_start_index[node_parent] == UINT32_MAX)
    {
        this->m_children_start_index[node_parent] = this->m_tree.size();
    }
    this->m_tree.push_back(Node(index_in_s, this->m_tree[node_parent].depth + 1));
    this->m_parent_index.push_back(node_parent);
    this->m_children_start_index.push_back(UINT32_MAX);
}

void Tree::_delete_node(const uint32_t node)
{
    this->m_tree[node].is_alive = false;
}

void Tree::_update_neighbours_in_tree_path(
    std::vector<uint32_t> indexes_in_s, 
    const std::vector<Graph>& s_list,
    std::unordered_map<uint32_t, uint32_t> path_in_tree,
    std::unordered_multimap<uint32_t,uint32_t>& found_neibours_in_tree_path)
{
    // return all the neighbours of the indexes in s that are also in the tree path
    const Graph& graph = s_list[this->m_tree[0].index];
    for (uint32_t index_in_s : indexes_in_s)
    {
        auto src_vertex = index_in_s;

        auto[first_neigbhour, last_neighbour] = graph.get_neighbours(src_vertex);
        for (auto edge = first_neigbhour; edge != last_neighbour; ++edge) 
        {
            uint32_t neighbour = *edge;
            uint32_t neighbour_index = static_cast<uint32_t>(neighbour);
            if (path_in_tree.find(neighbour_index) != path_in_tree.end())
            {
                found_neibours_in_tree_path.insert({neighbour_index, path_in_tree[neighbour_index]-1});
            }    
        }
    }

}


std::vector<uint32_t> Tree::_get_colors_of_neighbours_not_in_tree_path(
     std::vector<uint32_t> indexes_in_s, 
     const std::vector<Graph>& s_list,
     std::unordered_map<uint32_t, uint32_t> path_in_tree)
{
    // return all the neighbours of the indexes in s that are also in the tree path
    std::vector<uint32_t> neighbours_in_s_not_in_tree_path;

    for (uint32_t index_in_s : indexes_in_s)
    {
        auto src_vertex = index_in_s;

        auto[first_neigbhour, last_neighbour] = s_list[this->m_tree[0].index].get_neighbours(src_vertex);
        for (auto edge = first_neigbhour; edge != last_neighbour; ++edge) {
            uint32_t neighbour_index = *edge;
            if (path_in_tree.find(neighbour_index) == path_in_tree.end())
            {
                neighbours_in_s_not_in_tree_path.push_back(s_list[this->m_tree[0].index].get_vertex_color(neighbour_index));
            }
        }
    } 

    
    return neighbours_in_s_not_in_tree_path;
}


/* ---------- Public API ---------- */

std::unordered_map<uint32_t, uint32_t>
Tree::get_tree_path_map(const uint32_t last_node_in_path)
{
    std::unordered_map<uint32_t, uint32_t> path;
    path.reserve(this->m_tree[last_node_in_path].depth + 1); // Pre-allocate
    
    uint32_t current = last_node_in_path;

    while (current && (this->m_parent_index[current] != UINT32_MAX)) {
        path[m_tree[current].index] = m_tree[current].depth;
        current = m_parent_index[current];
    }

    return path;
}


bool Tree::is_empty()
{
    return this->m_tree[0].is_alive == false;
}

std::pair<uint32_t, uint32_t>
Tree::add_tree_level(const std::vector<std::pair<uint32_t, uint32_t>>& new_indexes,
                     const std::vector<Graph>& s_list)
{
    // IMPORTANT DISCLAIMER:
    // new_indexes is a vector of pairs (index in s, parent node in tree)
    // has to be ordered by parent_node and all the parent nodes have to be ordered by their adding order to the tree

    uint32_t start_index_new_nodes = this->m_tree.size();

    if (!new_indexes.empty()) {
        // initial path in tree
        std::unordered_map<uint32_t, uint32_t> path_in_tree =
            get_tree_path_map(new_indexes[0].second);


        for (std::pair<uint32_t, uint32_t> idx : new_indexes)
        {
            _add_node(idx.second, idx.first);
        }

        // update histogram
        int new_child_index = 0;
        int32_t last_parent_node = UINT32_MAX;
        std::unordered_multimap<uint32_t,uint32_t> decrease_neighbours_in_hist_map;
        std::unordered_set<uint32_t> empty_previous_children;

        while (new_child_index < new_indexes.size())
        {
            // get all children of the same parent
            std::vector<uint32_t> new_indexes_same_parent = {new_indexes[new_child_index].first};
            uint32_t current_parent = new_indexes[new_child_index].second;
            while(new_child_index + 1 < new_indexes.size() && new_indexes[new_child_index + 1].second == current_parent)
            {
                new_indexes_same_parent.push_back(new_indexes[new_child_index + 1].first);
                new_child_index++;
            }
            new_child_index++;

            // update tree path for parent
            if (last_parent_node != UINT32_MAX)
            {
                uint32_t last_parent_iterate = last_parent_node;
                uint32_t current_parent_iterate = current_parent;
                std::unordered_set<uint32_t> replaced_value_in_key;
                while(last_parent_iterate != current_parent_iterate)
                {
                    path_in_tree[this->m_tree[current_parent_iterate].index] = this->m_tree[current_parent_iterate].depth;
                    replaced_value_in_key.insert(this->m_tree[current_parent_iterate].index);
                    if (replaced_value_in_key.find(this->m_tree[last_parent_iterate].index) == replaced_value_in_key.end())
                    {
                        path_in_tree.erase(this->m_tree[last_parent_iterate].index);
                    }
                    
                    current_parent_iterate = this->m_parent_index[current_parent_iterate];
                    last_parent_iterate = this->m_parent_index[last_parent_iterate];
                }
            }

            last_parent_node = current_parent;
            _update_neighbours_in_tree_path(new_indexes_same_parent, s_list, path_in_tree, decrease_neighbours_in_hist_map);

           

            const std::vector<uint32_t> update_in_hist2 =
                _get_colors_of_neighbours_not_in_tree_path(new_indexes_same_parent, s_list, path_in_tree);

            m_hist.update_neigbours_add_node_add_neighbours_to_hist(
                this->m_tree.back().depth-1, update_in_hist2);
        }

        std::vector<uint32_t> decrease_neighbours_in_hist;
        for (const auto& pair : decrease_neighbours_in_hist_map) {
            decrease_neighbours_in_hist.push_back(pair.second);
        }

        uint32_t color = s_list[this->m_tree[0].index].get_vertex_color(new_indexes[0].first);
        m_hist.update_hist_decrease_from_neighbours(
            color, decrease_neighbours_in_hist);
    }

    // Print all tree vectors
    std::cout << "Tree Nodes:" << std::endl;
    for (const auto& node : m_tree) {
        std::cout << "Index: " << node.index << ", Depth: " << node.depth << ", Alive: " << node.is_alive << std::endl;
    }

    std::cout << "Parent Indices:" << std::endl;
    for (const auto& parent : m_parent_index) {
        std::cout << parent << " ";
    }
    std::cout << std::endl;

    std::cout << "Children Start Indices:" << std::endl;
    for (const auto& child_start : m_children_start_index) {
        std::cout << child_start << " ";
    }
    std::cout << std::endl;
    return {start_index_new_nodes, this->m_tree.size()};
}

void Tree::remove_node(const uint32_t node,
                       const std::vector<Graph>& s_list)
{
    uint32_t node_to_remove = node;
    std::unordered_map<uint32_t, uint32_t> path_in_tree =
        get_tree_path_map(node_to_remove);

    std::queue<uint32_t> bfs_queue;
    bfs_queue.push(node_to_remove);

    while (!bfs_queue.empty()) {
        uint32_t current_node = bfs_queue.front();
        bfs_queue.pop();

        for (uint32_t i = this->m_children_start_index[current_node];
             i < (current_node+1 < this->m_tree.size()? this->m_children_start_index[current_node+1]:m_tree.size()); ++i) {
            if (!this->m_tree[i].is_alive) {
                path_in_tree[this->m_tree[i].index] = this->m_tree[i].depth;
                bfs_queue.push(i);
            }
        }
    }

    while (node_to_remove) {
        uint32_t parent = UINT32_MAX;

        if (this->m_tree[node_to_remove].depth != 0)
        {
            const std::vector<uint32_t> update_in_hist =
                _get_colors_of_neighbours_not_in_tree_path({this->m_tree[node_to_remove].index}, s_list, path_in_tree);

            m_hist.update_neigbours_remove_node_decrease_neighbours_from_hist(
                this->m_tree[node_to_remove].depth-1, update_in_hist);
            parent = this->m_parent_index[node_to_remove];
            _delete_node(node_to_remove);
        }
        else{
            m_tree[0].is_alive = false;
            //std::cout << "deleted_root" << std::endl;
        }


        // Check if the parent has any alive children
        bool has_alive_children = false;
        for (uint32_t i = this->m_children_start_index[parent];
             i < (parent + 1 < this->m_tree.size() ? this->m_children_start_index[parent + 1] : m_tree.size()); ++i) {
                //std::cout << "Checking child node " << i << " alive status: " << this->m_tree[i].is_alive << "s_index " << m_tree[i].index << std::endl;
            if (this->m_tree[i].is_alive) {
            has_alive_children = true;
            break;
            }
        }
        std::cout << "!!! " << (has_alive_children? "true":"false") << std::endl;

        if (parent && !has_alive_children)
        {
            node_to_remove = parent;
        }
        else
            break;
    }
}

uint32_t Tree::get_node_by_depth(const uint32_t lowest_node_in_match,
                                int32_t target_depth)
{
    uint32_t current = lowest_node_in_match;

    while (current && this->m_tree[current].depth != target_depth)
        current = this->m_parent_index[current];

    return current;
}
