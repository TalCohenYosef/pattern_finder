#include "MultiGraphPatternFinder.h"
#include "GeneralColorHist.h"

#include <unordered_set>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

/* ---------- Statistics ---------- */

uint32_t MultiGraphPatternFinder::find_first_color(
    uint32_t color_number,
    int32_t  s_size,
    const std::vector<Graph>& s_list)
{
    std::vector<std::pair<uint32_t, uint32_t>> color_count(color_number, {0, 0});
    for (uint32_t i = 0; i < color_count.size(); ++i)
        color_count[i].second = i;

    for (int32_t i = 0; i < s_size; ++i)
        for (uint32_t v = 0; v < s_list[i].vertex_count(); ++v)
            color_count[s_list[i].get_vertex_color(v)].first++;

    std::sort(color_count.begin(), color_count.end());
    return color_count[color_count.size() - FIRST_COLOR_ORDER].second;
}


/* ---------- Pattern extension ---------- */

std::pair<int32_t,int32_t>
MultiGraphPatternFinder::extend_pattern_at_node_find_matches_in_s(
    std::vector<std::shared_ptr<Tree>>& trees,
    int32_t s_size,
    const std::vector<Graph>& s_list,
    uint32_t new_node_id,
    uint32_t new_color,
    uint32_t node_to_connect_id,
    std::vector<std::vector<NodePtr>>& last_nodes,
    std::unordered_set<uint32_t>& alive_indexes)
{
    for (int i = 0; i < s_size; ++i) {
        if (!trees[i]) continue;

        std::vector<std::pair<uint32_t, NodePtr>> candidates;

        for (const NodePtr& lowest : last_nodes[i]) {
            auto node_in_tree =
                trees[i]->get_node_by_depth(lowest, node_to_connect_id + 1);

            std::unordered_map<uint32_t, uint32_t> in_match =
                trees[i]->get_tree_path_map(lowest);

            auto [first_neighbour, last_neighbour] =
                s_list[i].get_neighbours(node_in_tree->index);

            for (auto e = first_neighbour; e != last_neighbour; ++e) {
                if (s_list[i].get_vertex_color(*e) == static_cast<int>(new_color) &&
                    in_match.find(static_cast<uint32_t>(*e)) == in_match.end())
                {
                    candidates.push_back({*e, lowest});
                }
            }
        }

        std::vector<NodePtr> new_last_nodes =
            trees[i]->add_tree_level(candidates, s_list);

        for (const NodePtr& node : last_nodes[i]) {
            if (node->son == nullptr)
                trees[i]->remove_node(node, s_list);
        }

        last_nodes[i] = std::move(new_last_nodes);

        if (trees[i]->is_empty()) {
            trees[i].reset();
            alive_indexes.erase(i);
        }
    }

    uint32_t sum_last_nodes_sizes = 0;
    for (const auto& nodes : last_nodes)
        sum_last_nodes_sizes += nodes.size();

    return {static_cast<int32_t>(alive_indexes.size()), sum_last_nodes_sizes};
}

/* ---------- Edge scoring and pruning ---------- */

uint32_t MultiGraphPatternFinder::score_edge_support(
    uint32_t uP, uint32_t vP,
    const std::vector<std::shared_ptr<Tree>>& trees,
    const std::vector<std::vector<NodePtr>>& last_nodes,
    const std::vector<Graph>& s_list,
    uint32_t s_size)
{
    uint32_t score = 0;

    for (uint32_t s = 0; s < s_size; ++s) {
        if (!trees[s]) continue;

        bool supported = false;
        for (const NodePtr& last_node : last_nodes[s]) {
            NodePtr uS = trees[s]->get_node_by_depth(last_node, uP + 1);
            NodePtr vS = trees[s]->get_node_by_depth(last_node, vP + 1);
            if (!uS || !vS) continue;

            auto u = static_cast<BoostGraph::vertex_descriptor>(uS->index);
            auto v = static_cast<BoostGraph::vertex_descriptor>(vS->index);

            if (s_list[s].is_edge(u, v)) {
                supported = true;
                break;
            }
        }
        if (supported) ++score;
    }

    return score;
}

void MultiGraphPatternFinder::apply_edge_and_prune(
    BoostGraph& pattern, uint32_t uP, uint32_t vP,
    std::vector<std::shared_ptr<Tree>>& trees,
    std::vector<std::vector<NodePtr>>& last_nodes,
    std::unordered_set<uint32_t>& alive_indexes,
    const std::vector<Graph>& s_list)
{
    boost::add_edge(uP, vP, pattern);

    for (uint32_t s = 0; s < trees.size(); ++s) {
        if (!trees[s]) continue;

        std::vector<NodePtr> updated_matches;
        for (const NodePtr& match : last_nodes[s]) {
            NodePtr uS = trees[s]->get_node_by_depth(match, uP + 1);
            NodePtr vS = trees[s]->get_node_by_depth(match, vP + 1);
            if (!uS || !vS) continue;

            auto u = static_cast<BoostGraph::vertex_descriptor>(uS->index);
            auto v = static_cast<BoostGraph::vertex_descriptor>(vS->index);

            if (s_list[s].is_edge(u, v))
                updated_matches.push_back(match);
            else
                trees[s]->remove_node(match, s_list);
        }

        last_nodes[s] = std::move(updated_matches);

        if (trees[s]->is_empty()) {
            trees[s].reset();
            alive_indexes.erase(s);
        }
    }
}

bool MultiGraphPatternFinder::add_edge(
    BoostGraph& pattern,
    std::vector<std::shared_ptr<Tree>>& trees,
    std::vector<std::vector<NodePtr>>& last_nodes,
    std::unordered_set<uint32_t>& alive_indexes,
    uint32_t s_size,
    const std::vector<Graph>& s_list,
    double threshold,
    double alive_threshold)
{
    if (alive_indexes.empty()) return false;

    uint32_t best_score = 0;
    uint32_t best_u = 0, best_v = 0;
    bool found = false;

    auto vertices_range = boost::make_iterator_range(vertices(pattern));
    for (auto uP : vertices_range) {
        for (auto vP : vertices_range) {
            if (uP >= vP) continue;
            if (boost::edge(uP, vP, pattern).second) continue;

            uint32_t score = score_edge_support(uP, vP, trees, last_nodes, s_list, s_size);
            if (score > best_score) {
                best_score = score;
                best_u = uP;
                best_v = vP;
                found = true;
            }
        }
    }

    if (!found) return false;

    if ((best_score >= alive_threshold * s_size) &&
        (best_score >= threshold * alive_indexes.size()))
    {
        apply_edge_and_prune(pattern, best_u, best_v,
                             trees, last_nodes, alive_indexes, s_list);
        std::cout << "Added edge (" << best_u << ", " << best_v
                  << ") with score " << best_score << "\n";
        return true;
    }

    return false;
}

/* ---------- Main algorithm ---------- */

std::pair<BoostGraph, std::unordered_set<uint32_t>>
MultiGraphPatternFinder::find_pattern(
    std::vector<Graph>& s_list,
    double alive_threshold)
{
    if (s_list.empty()) {
        throw std::runtime_error("No input graphs provided to MultiGraphPatternFinder");
    }
    
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<int32_t> m_color_map =
        PatternUtils::map_colors(s_list);

    const std::vector<double> color_prob =
        PatternUtils::compute_color_distribution(
            static_cast<uint32_t>(m_color_map.size()),
            static_cast<int32_t>(s_list.size()),
            s_list);

    GeneralColorHist color_hist(m_color_map.size());

    std::vector<std::shared_ptr<Tree>> trees(s_list.size());
    for (int i = 0; i < static_cast<int>(s_list.size()); ++i)
        trees[i] = std::make_shared<Tree>(i, color_hist);

    std::vector<std::vector<NodePtr>> last_nodes(s_list.size());

    std::vector<std::pair<double, uint32_t>> colors;
    for (uint32_t c = 0; c < color_prob.size(); ++c)
        if (color_prob[c] > 0.0)
            colors.emplace_back(color_prob[c], c);

    std::sort(colors.begin(), colors.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    uint32_t first_color = colors.empty() ? 0u : colors.front().second;
    std::cout << "first_color: " << m_color_map[first_color] << "\n";

    BoostGraph pattern;
    uint32_t alive_s = static_cast<uint32_t>(s_list.size());
    boost::add_vertex(VertexProperty{static_cast<int32_t>(first_color)}, pattern);

    for (int i = 0; i < static_cast<int>(s_list.size()); ++i) {
        std::vector<uint32_t> matches =
            PatternUtils::find_initial_matches(s_list[i], first_color);

        std::vector<std::pair<uint32_t, NodePtr>> initial_indexes;
        for (uint32_t match : matches)
            initial_indexes.push_back({match, trees[i]->get_root()});

        last_nodes[i] = trees[i]->add_tree_level(initial_indexes, s_list);

        if (matches.empty()) {
            trees[i].reset();
            --alive_s;
        }
    }

    std::unordered_set<uint32_t> alive_indexes;
    for (uint32_t i = 0; i < s_list.size(); ++i)
        if (trees[i]) alive_indexes.insert(i);

    bool failed_add_edge      = false;
    bool done_adding_vertices = false;

    std::mt19937_64 rng;
    uint64_t timeSeed = std::chrono::high_resolution_clock::now()
                            .time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};
    rng.seed(ss);
    std::uniform_real_distribution<double> unif(0, 1);

    uint32_t last_color = first_color;

    while (alive_s > alive_threshold * s_list.size()) {
        std::cout << "number of alive: " << alive_indexes.size() << "\n";

        double p = 1.0 / std::cbrt(boost::num_vertices(pattern));

        if (((unif(rng) < p) && !done_adding_vertices) || failed_add_edge) {
            std::pair<int32_t,int32_t> candidates =
                color_hist.get_color_to_add(alive_threshold);

            if (candidates.first == -1 && alive_threshold > 0.1) {
                candidates = color_hist.get_color_to_add(
                    std::max(1u, static_cast<uint32_t>(
                        alive_threshold * s_list.size() * 0.5)));
            }

            if (candidates.first == -1) {
                done_adding_vertices = true;
            } else {
                int32_t color_new      = candidates.first;
                int32_t node_to_connect = candidates.second;

                std::cout << "adding vertex with color number: "
                          << m_color_map[color_new]
                          << " to node: " << node_to_connect << "\n";

                uint32_t new_node_id = boost::add_vertex(
                    VertexProperty{static_cast<int32_t>(color_new)}, pattern);

                boost::add_edge(node_to_connect, new_node_id,
                                EdgeProperty{false}, pattern);

                auto alive_and_matches =
                    extend_pattern_at_node_find_matches_in_s(
                        trees,
                        static_cast<int32_t>(s_list.size()),
                        s_list,
                        new_node_id,
                        color_new,
                        node_to_connect,
                        last_nodes,
                        alive_indexes);

                alive_s = static_cast<uint32_t>(alive_indexes.size());
            }

            failed_add_edge = false;
        } else if (!failed_add_edge) {
            failed_add_edge = !add_edge(
                pattern, trees, last_nodes, alive_indexes,
                static_cast<uint32_t>(s_list.size()), s_list,
                0, alive_threshold);

            alive_s = static_cast<uint32_t>(alive_indexes.size());
        }

        std::cout << "--- Histogram after step ---\n";
        color_hist.dump();

        if (failed_add_edge && done_adding_vertices) break;
    }

    PatternUtils::recolor_pattern(pattern, m_color_map);

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: "
              << std::chrono::duration<double>(end - start).count()
              << " seconds\n";

    return {pattern, alive_indexes};
}
