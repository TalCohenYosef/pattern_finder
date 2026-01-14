#include "PatternFinder.h"
#include "GeneralColorHist.h"
#include <unordered_set>   // needed for unordered_set
#include <utility>         // needed for std::pair
#include <vector>          // needed for std::vector
#include <map>             // needed for std::map
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <iostream>


// TO REMOVE
#include <iostream>

/* ---------- Color handling ---------- */

void PatternFinder::recolor_s(
    const std::map<int32_t, uint32_t>& old_to_new,
    Graph& s)
{
    for (uint32_t vertex = 0; vertex < s.vertex_count(); ++vertex) {
        s.set_vertex_color(vertex, old_to_new.at(s.get_vertex_color(vertex)));
    }
}

std::vector<int32_t> PatternFinder::map_colors(
    int32_t s_size,
    std::vector<Graph>& s_list)
{
    std::vector<int32_t> m_color_map;
    std::map<int32_t, uint32_t> old_to_new;

    for (uint32_t i = 0; i < s_size; ++i) {
        for (uint32_t v = 0; v < s_list[i].vertex_count(); ++v) {
            int32_t c = s_list[i].get_vertex_color(v);
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

    for (uint32_t i = 0; i < s_size; ++i) {
        for (uint32_t v = 0; v < s_list[i].vertex_count(); ++v)
            color_count[s_list[i].get_vertex_color(v)].first++;
    }

    std::sort(color_count.begin(), color_count.end());

    return color_count[color_count.size() - FIRST_COLOR_ORDER].second;
}

void PatternFinder::compute_global_color_distribution(
    uint32_t color_number,
    int32_t s_size,
    const std::vector<Graph>& s_list)
{
    global_color_count.assign(color_number, 0);
    total_color_mass = 0;

    for (uint32_t i = 0; i < s_size; ++i) {
        for (uint32_t v = 0; v < s_list[i].vertex_count(); ++v) {
            uint32_t c = s_list[i].get_vertex_color(v);
            global_color_count[c]++;
            total_color_mass++;
        }
    }

    global_color_prob.resize(color_number);
    for (uint32_t c = 0; c < color_number; ++c) {
        global_color_prob[c] =
            (global_color_count[c] > 0)
                ? double(global_color_count[c]) / double(total_color_mass)
                : 0.0;
    }
}


/* ---------- Initial matches ---------- */

std::vector<uint32_t> PatternFinder::find_initial_matches(
    const Graph& s,
    uint32_t color)
{
    std::vector<uint32_t> matches;
    for (uint32_t v = 0; v < s.vertex_count(); ++v)
        if (s.get_vertex_color(v) == static_cast<int>(color))
            matches.push_back(static_cast<uint32_t>(v));
    return matches;
}

/* ---------- Pattern extension ---------- */

std::pair<int32_t,int32_t> PatternFinder::extend_pattern_at_node_find_matches_in_s(
    std::vector<std::shared_ptr<Tree>>& trees,
    int32_t s_size,
    const std::vector<Graph>& s_list,
    uint32_t new_node_id,
    uint32_t new_color,
    uint32_t node_to_connect_id,
    std::vector<std::pair<uint32_t,uint32_t>>& last_nodes,
    std::unordered_set<uint32_t>& alive_indexes)
{

    for (int i = 0; i < s_size; ++i) {
        if (!trees[i]) continue;

        std::vector<std::pair<uint32_t, uint32_t>> candidates;

        for (uint32_t lowest = last_nodes[i].first; lowest < last_nodes[i].second; ++lowest) {
            if (!trees[i]->is_alive(lowest)) {
                continue;
            }

            auto node_in_tree =
                trees[i]->get_node_by_depth(lowest, node_to_connect_id+1);

            std::unordered_map<uint32_t, uint32_t> in_match= trees[i]->get_tree_path_map(lowest);
           
            bool found_child = false;
            auto [first_neigbhour, last_neighbour] = s_list[i].get_neighbours(trees[i]->get_s_index(node_in_tree));
            for (auto e = first_neigbhour; e != last_neighbour; ++e) {

                if (s_list[i].get_vertex_color(*e) == new_color)
                {
                    if (in_match.find(static_cast<uint32_t>(*e)) == in_match.end())
                    {
                        std::cout << "Adding candidate node " << *e << " under parent node " << trees[i]->get_s_index(node_in_tree) << " in tree " << i << "\n";
                        candidates.push_back({*e, lowest});
                        found_child = true;
                    }
                }
            }

            if (!found_child) {
                std::cout << "Removing node " << trees[i]->get_s_index(lowest) << " from tree " << i << "\n";
                trees[i]->remove_node(lowest, s_list);
            }

        }

        last_nodes[i] = trees[i]->add_tree_level(candidates, s_list);
  

        if (trees[i]->is_empty())
        {
            trees[i].reset();
            alive_indexes.erase(i);
        }
    }
    uint32_t sum_last_nodes_sizes = 0;

    for (const auto& nodes : last_nodes) {
        sum_last_nodes_sizes += nodes.second - nodes.first;
    }

    return {alive_indexes.size(),sum_last_nodes_sizes};
}

uint32_t PatternFinder::score_edge_support(
    uint32_t uP,
    uint32_t vP,
    const std::vector<std::shared_ptr<Tree>>& trees,
    const std::vector<std::pair<uint32_t,uint32_t>>& last_nodes,
    const std::vector<Graph>& s_list,
    uint32_t s_size) {
    uint64_t key = (static_cast<uint64_t>(std::min(uP, vP)) << 32) | std::max(uP, vP);

    uint32_t score = 0;

    for (uint32_t s = 0; s < s_size; ++s) {
        if (!trees[s]) continue;

        bool supported = false;

        for (uint32_t last_node = last_nodes[s].first; last_node < last_nodes[s].second; ++last_node) {
            if (!trees[s]->is_alive(last_node)) {
                continue;
            }

            uint32_t uS = trees[s]->get_node_by_depth(last_node, uP+1);
            uint32_t vS = trees[s]->get_node_by_depth(last_node, vP+1);

            if (!uS || !vS)
            {
                    // std::cout << "  [DEBUG] S " << s
                    //         << " missing mapping: "
                    //         << "uS=" << (uS ? "ok" : "null")
                    //         << ", vS=" << (vS ? "ok" : "null")
                    //         << "\n";
                continue;
            }


            if (!s_list[s].is_edge(trees[s]->get_s_index(uS), trees[s]->get_s_index(vS))) {
                supported = true;
                break;
            }
        }

        if (supported) 
        {
            score++;
        }
    }

    return score;
}

void PatternFinder::apply_edge_and_prune(
    BoostGraph& pattern,
    uint32_t uP,
    uint32_t vP,
    std::vector<std::shared_ptr<Tree>>& trees,
    std::vector<std::pair<uint32_t,uint32_t>>& last_nodes,
    std::unordered_set<uint32_t>& alive_indexes,
    const std::vector<Graph>& s_list
) {
    boost::add_edge(uP, vP, pattern);

    for (uint32_t s = 0; s < trees.size(); ++s) {
        if (!trees[s]) continue;

        for (uint32_t& match = last_nodes[s].first; match < last_nodes[s].second; ++match) {
            if (trees[s]->is_alive(match) == false) {
                continue;
            }

            uint32_t uS = trees[s]->get_node_by_depth(match, uP+1);
            uint32_t vS = trees[s]->get_node_by_depth(match, vP+1);

            if (!uS || !vS) continue;

            if (!s_list[s].is_edge(trees[s]->get_s_index(uS), trees[s]->get_s_index(vS))) {
                trees[s]->remove_node(match, s_list);
            }
        }

        if (trees[s]->is_empty()) {
            trees[s].reset();
            alive_indexes.erase(s);
        }
    }
}

bool PatternFinder::add_edge(
    BoostGraph& pattern,
    std::vector<std::shared_ptr<Tree>>& trees,
    std::vector<std::pair<uint32_t,uint32_t>>& last_nodes,
    std::unordered_set<uint32_t>& alive_indexes,
    uint32_t s_size,
    const std::vector<Graph>& s_list,
    double threshold,
    double alive_threshold
) {
    if (alive_indexes.size() == 0) {
        return false;
    }

    uint32_t best_score = 0;
    uint32_t best_u = 0;
    uint32_t best_v = 0;
    bool found = false;

    auto vertices_range = boost::make_iterator_range(vertices(pattern));

    for (auto uP : vertices_range) 
    {
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

    if ((best_score >= alive_threshold * s_size) &&
        (best_score >= threshold * alive_indexes.size())) 
    {
       

        apply_edge_and_prune(
            pattern, best_u, best_v,
            trees, last_nodes, alive_indexes, s_list
        );
        std::cout << "Added edge (" << best_u << ", " << best_v << ") with score " << best_score << std::endl;
        return true;
    }

    return false;
}

void PatternFinder::recolor_pattern(BoostGraph& pattern,
    const std::vector<int32_t>& color_map)
{
    using Vertex = BoostGraph::vertex_descriptor;

    for (auto v : boost::make_iterator_range(vertices(pattern))) 
    {
        pattern[v].color = static_cast<uint32_t>(color_map[pattern[v].color]);
    }
}



/* ---------- Main algorithm ---------- */

std::pair<BoostGraph, std::unordered_set<u_int32_t>>
PatternFinder::find_pattern(
    int32_t s_size,
    std::vector<Graph>& s_list,
    double alive_threshold)
{
    auto start = std::chrono::high_resolution_clock::now();
    PatternFinder pf;
    std::vector<int32_t> m_color_map = pf.map_colors(s_size, s_list);
    pf.compute_global_color_distribution(
        static_cast<uint32_t>(m_color_map.size()),
        s_size,
        s_list
    );
    
    GeneralColorHist color_hist(m_color_map.size());

    std::vector<std::shared_ptr<Tree>> trees(s_size);
    for (int i = 0; i < s_size; ++i)
        trees[i] = std::make_shared<Tree>(i, color_hist);

    std::vector<std::pair<uint32_t,uint32_t>> last_nodes(s_size);

    
    std::vector<std::pair<double, uint32_t>> colors; // (probability, color)

    for (uint32_t c = 0; c < pf.global_color_prob.size(); ++c) {
        if (pf.global_color_prob[c] > 0.0) {
            colors.emplace_back(pf.global_color_prob[c], c);
        }
    }
    
    // Sort by frequency DESCENDING
    std::sort(colors.begin(), colors.end(),
              [](const auto& a, const auto& b) {
                  return a.first > b.first;
              });
    
    uint32_t first_color;
    if (colors.size() >= 2) {
        first_color = colors[0].second;  // second most common
    } else {
        first_color = colors[0].second;  // fallback
    }
    
    std::cout << "first_color: " << m_color_map[first_color] << std::endl;

    BoostGraph pattern;
    uint32_t alive_s = s_size;

    boost::add_vertex(
        VertexProperty{static_cast<int32_t>(first_color)}, pattern);

    for (int i = 0; i < s_size; ++i) {        
        std::vector<uint32_t> matches =
            find_initial_matches(s_list[i], first_color);

        std::vector<std::pair<uint32_t, uint32_t>> initial_indexes;
        for (uint32_t match : matches) {
            initial_indexes.push_back({match, 0});
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

    std:: int32_t number_of_mathces = 0;
    uint32_t step = 0;
    uint32_t last_color = first_color;
    uint32_t last_color_amount_of_pcicks = 0;

    std::unordered_set<uint32_t> alive_indexes;
    for (uint32_t i = 0; i < s_size; ++i)
    {
        if (trees[i])
        {
            alive_indexes.insert(i);
        }
    }

    int i = 0;
    while (alive_s > alive_threshold * s_size) 
    {
        i++;
    
//        std::cout << "number of alive: " << alive_indexes.size() << std::endl;
    
        double p = 1.0 / std::cbrt(boost::num_vertices(pattern));
        
    
        if ((unif(rng) < p) && !done_adding_vertices && failed_add_edge)
        {
            std::pair<int32_t,int32_t> candidates = color_hist.get_color_to_add(alive_threshold);
    
            if (candidates.first == -1)
            {
                done_adding_vertices = true;
            }
            else
            {
                int32_t color_new = candidates.first;
                int32_t node_to_connect = candidates.second;
    
                std::cout << "adding vertex with color number: " << m_color_map[color_new] << " to node: "<< node_to_connect<<std::endl;
                    uint32_t new_node_id =
                        boost::add_vertex(
                            VertexProperty{static_cast<int32_t>(color_new)},
                            pattern);
    
                    boost::add_edge(
                        node_to_connect,
                        new_node_id,
                        EdgeProperty{false},
                        pattern);
    
                    auto alive_and_matches =
                        extend_pattern_at_node_find_matches_in_s(
                            trees,
                            s_size,
                            s_list,
                            new_node_id,
                            color_new,
                            node_to_connect,
                            last_nodes,
                            alive_indexes);
    
                    alive_s = alive_indexes.size();
                    number_of_mathces = alive_and_matches.second;
            }
    
            failed_add_edge = false;
        }

        else if (!failed_add_edge)
        {
    
            failed_add_edge = !add_edge(
                pattern,
                trees,
                last_nodes,
                alive_indexes,
                s_size,
                s_list,
                0,
                alive_threshold
            );
    
            alive_s = alive_indexes.size();
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


    return std::make_pair(pattern, alive_indexes);
    
}
