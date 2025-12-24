#include "PatternFinder.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <unordered_map>
#include <stdexcept>

// TO REMOVE
#include <iostream>

/* ---------- Color handling ---------- */

void PatternFinder::recolor_s(
    const std::map<int32_t, uint32_t>& old_to_new,
    Graph& s)
{
    for (auto v : boost::make_iterator_range(vertices(s))) {
        s[v].color = old_to_new.at(s[v].color);
    }
}

std::vector<int32_t> PatternFinder::map_colors(
    int32_t s_size,
    std::vector<Graph>& s_list)
{
    std::vector<int32_t> m_color_map;
    std::map<int32_t, uint32_t> old_to_new;

    for (int i = 0; i < s_size; ++i) {
        for (auto v : boost::make_iterator_range(vertices(s_list[i]))) {
            int32_t c = s_list[i][v].color;
            if (!old_to_new.count(c)) {
                old_to_new[c] = m_color_map.size();
                m_color_map.push_back(c);
            }
        }
    }

    for (int i = 0; i < s_size; ++i)
        recolor_s(old_to_new, s_list[i]);

    return m_color_map;
}

/* ---------- Statistics ---------- */

uint32_t PatternFinder::find_first_color(
    uint32_t color_number,
    int32_t s_size,
    const std::vector<Graph>& s_list)
{
    std::vector<std::pair<uint32_t, uint32_t>> color_count(
        color_number, {0, 0});

    for (uint32_t i = 0; i < color_count.size(); ++i)
        color_count[i].second = i;

    for (int i = 0; i < s_size; ++i) {
        for (auto v : boost::make_iterator_range(vertices(s_list[i])))
            color_count[s_list[i][v].color].first++;
    }

    std::sort(color_count.begin(), color_count.end());
    return color_count[color_count.size() - FIRST_COLOR_ORDER].second;
}

/* ---------- Initial matches ---------- */

std::vector<uint32_t> PatternFinder::find_initial_matches(
    const Graph& s,
    uint32_t color)
{
    std::vector<uint32_t> matches;
    for (auto v : boost::make_iterator_range(vertices(s)))
        if (s[v].color == static_cast<int>(color))
            matches.push_back(static_cast<uint32_t>(v));
    return matches;
}

/* ---------- Pattern extension ---------- */

int32_t PatternFinder::extend_pattern_at_node_find_matches_in_s(
    std::vector<std::shared_ptr<Tree>>& trees,
    int32_t s_size,
    const std::vector<Graph>& s_list,
    uint32_t new_node_id,
    uint32_t new_color,
    uint32_t node_to_connect_id,
    std::vector<std::vector<NodePtr>>& last_nodes)
{
    int32_t alive_count = 0;

    for (int i = 0; i < s_size; ++i) {
        if (!trees[i]) continue;

        std::vector<std::pair<uint32_t, NodePtr>> candidates;

        for (const NodePtr& lowest : last_nodes[i]) {
            auto node_in_tree =
                trees[i]->get_node_by_depth(lowest, node_to_connect_id+1);

            std::unordered_map<uint32_t, uint32_t> in_match= trees[i]->get_tree_path_map(lowest);

            bool found_child = false;
            for (auto e :
                 boost::make_iterator_range(
                     out_edges(node_in_tree->index, s_list[i]))) {

                auto neigh = target(e, s_list[i]);
                if (s_list[i][neigh].color == new_color)
                {
                    if (in_match.find(static_cast<uint32_t>(neigh)) == in_match.end())
                    {
                        candidates.push_back({neigh, lowest});
                        found_child = true;
                    }
                }
            }

            if (!found_child) {
                trees[i]->remove_node(lowest, s_list);
            }

        }

        last_nodes[i] = trees[i]->add_tree_level(
            candidates, s_list);;
        
        if (!trees[i]->is_empty())
            alive_count++;
        else
            trees[i].reset();
    }

    return alive_count;
}

uint32_t PatternFinder::score_edge_support(
    uint32_t uP,
    uint32_t vP,
    const std::vector<std::shared_ptr<Tree>>& trees,
    const std::vector<std::vector<NodePtr>>& last_nodes,
    const std::vector<Graph>& s_list,
    uint32_t s_size
) {
    uint32_t score = 0;

    for (uint32_t s = 0; s < s_size; ++s) {
        if (!trees[s]) continue;

        bool supported = false;

        for (const NodePtr& last_node : last_nodes[s]) {
            NodePtr uS = trees[s]->get_node_by_depth(last_node, uP+1);
            NodePtr vS = trees[s]->get_node_by_depth(last_node, vP+1);

            if (!uS || !vS) continue;

            auto u = static_cast<Graph::vertex_descriptor>(uS->index);
            auto v = static_cast<Graph::vertex_descriptor>(vS->index);

            if (boost::edge(u, v, s_list[s]).second) {
                supported = true;
                break;
            }
        }

        if (supported) {
            score++;
        }
    }

    return score;
}

void PatternFinder::apply_edge_and_prune(
    Graph& pattern,
    uint32_t uP,
    uint32_t vP,
    std::vector<std::shared_ptr<Tree>>& trees,
    std::vector<std::vector<NodePtr>>& last_nodes,
    uint32_t& alive_count,
    const std::vector<Graph>& s_list
) {
    boost::add_edge(uP, vP, pattern);

    for (uint32_t s = 0; s < trees.size(); ++s) {
        if (!trees[s]) continue;

        std::vector<NodePtr> updated_matches;

        for (const NodePtr& match : last_nodes[s]) {
            NodePtr uS = trees[s]->get_node_by_depth(match, uP+1);
            NodePtr vS = trees[s]->get_node_by_depth(match, vP+1);

            if (!uS || !vS) continue;

            auto u = static_cast<Graph::vertex_descriptor>(uS->index);
            auto v = static_cast<Graph::vertex_descriptor>(vS->index);

            if (boost::edge(u, v, s_list[s]).second) {
                updated_matches.push_back(match);
            } else {
                trees[s]->remove_node(match, s_list);
            }
        }

        last_nodes[s] = std::move(updated_matches);

        if (trees[s]->is_empty()) {
            trees[s].reset();
            alive_count--;
        }
    }
}

bool PatternFinder::add_edge(
    Graph& pattern,
    std::vector<std::shared_ptr<Tree>>& trees,
    std::vector<std::vector<NodePtr>>& last_nodes,
    uint32_t& alive_count,
    uint32_t s_size,
    const std::vector<Graph>& s_list,
    double threshold,
    double alive_threshold
) {
    if (alive_count == 0) {
        return false;
    }

    uint32_t best_score = 0;
    uint32_t best_u = 0;
    uint32_t best_v = 0;
    bool found = false;

    auto vertices_range = boost::make_iterator_range(vertices(pattern));

    for (auto uP : vertices_range) {
        for (auto vP : vertices_range) {
            if (uP >= vP) continue;
            if (boost::edge(uP, vP, pattern).second) continue;

            uint32_t score = score_edge_support(
                uP, vP, trees, last_nodes, s_list, s_size
            );

            if (score > best_score) {
                best_score = score;
                best_u = uP;
                best_v = vP;
                found = true;
            }
        }
    }

    if (!found) {
        return false;
    }

    if (best_score >= alive_threshold * s_size &&
        best_score >= threshold * alive_count) {

        apply_edge_and_prune(
            pattern, best_u, best_v,
            trees, last_nodes, alive_count, s_list
        );

        return true;
    }

    return false;
}

void PatternFinder::recolor_pattern(Graph& pattern,
    const std::vector<int32_t>& color_map)
{
    using Vertex = Graph::vertex_descriptor;

    for (auto v : boost::make_iterator_range(vertices(pattern))) 
    {
        pattern[v].color = static_cast<uint32_t>(color_map[pattern[v].color]);
    }
}

/* ---------- Main algorithm ---------- */

Graph PatternFinder::find_pattern(
    int32_t s_size,
    std::vector<Graph> s_list,
    double alive_threshold)
{
    auto start = std::chrono::high_resolution_clock::now();
    PatternFinder pf;
    std::vector<int32_t> m_color_map = pf.map_colors(s_size, s_list);

    auto color_hist =
        std::make_shared<ColorHist>(m_color_map.size());

    std::vector<std::shared_ptr<Tree>> trees(s_size);
    for (int i = 0; i < s_size; ++i)
        trees[i] = std::make_shared<Tree>(i, color_hist);

    std::vector<std::vector<NodePtr>> last_nodes(s_size);

    uint32_t first_color =
        pf.find_first_color(m_color_map.size(), s_list.size(), s_list);

    Graph pattern;
    uint32_t alive_s = s_size;

    boost::add_vertex(
        VertexProperty{static_cast<int32_t>(first_color)}, pattern);

    for (int i = 0; i < s_size; ++i) {
        std::vector<uint32_t> matches =
            find_initial_matches(s_list[i], first_color);

        std::vector<std::pair<uint32_t, NodePtr>> initial_indexes;
        for (uint32_t match : matches) {
            initial_indexes.push_back({match, trees[i]->get_root()});
        }

        last_nodes[i] =
            trees[i]->add_tree_level(
                initial_indexes, s_list);
        
        if (matches.empty()) {
            trees[i].reset();
            alive_s--;
        }
    }

    bool failed_add_edge = false;
    bool done_adding_vertices = false;
    
    std::mt19937_64 rng;
    // initialize the random number generator with time-dependent seed
    uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
    rng.seed(ss);
    std::uniform_real_distribution<double> unif(0, 1);

    uint32_t step = 0;
    while (alive_s > alive_threshold * s_size) 
    {
        step++;
        std::cout << alive_s << std::endl;
        std::cout << "neighbor_in_s_calls: " << Tree::neighbor_in_s_calls << std::endl;
        double p = 1/(0.85+ std::log(std::sqrt(boost::num_vertices(pattern))));
        std::cout << "p: " << p << std::endl;

        if ((failed_add_edge or unif(rng) < p) && !(done_adding_vertices))
        {
            auto [color_new, node_to_connect] = color_hist->get_color_to_add();
            if(color_new == -1)
            {
                done_adding_vertices = true;
            }
            else
            {
                uint32_t new_node_id =
                boost::add_vertex(
                VertexProperty{static_cast<int32_t>(color_new)}, pattern);

                auto nv = boost::num_vertices(pattern);

                boost::add_edge(
                node_to_connect, new_node_id,
                EdgeProperty{false}, pattern);

                alive_s = extend_pattern_at_node_find_matches_in_s(trees, s_size, s_list,new_node_id, color_new,node_to_connect, last_nodes);
            
            }
            failed_add_edge = false;
        }
        else if (!failed_add_edge)
        {
                std::cout<< "Trying to add edge." << std::endl;
                failed_add_edge = !add_edge(
                    pattern,
                    trees,
                    last_nodes,
                    alive_s,
                    s_size,
                    s_list,
                    0.8,
                    alive_threshold
                );
    
        }
        if (failed_add_edge && done_adding_vertices)
        {
            break;
        }
    }

    recolor_pattern(pattern, m_color_map);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds\n";
    
    return pattern;
}
