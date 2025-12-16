#include "PatternFinder.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>
#include <stdexcept>

/* ---------- Color handling ---------- */

void PatternFinder::recolor_s(
    const std::map<int32_t, uint32_t>& old_to_new,
    Graph& s)
{
    for (auto v : boost::make_iterator_range(vertices(s))) {
        s[v].color = old_to_new.at(s[v].color);
    }
}

void PatternFinder::map_colors(
    int32_t s_size,
    std::vector<Graph>& s_list)
{
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
}

/* ---------- Statistics ---------- */

size_t PatternFinder::find_max_s(
    int32_t s_size,
    const std::vector<Graph>& s_list)
{
    size_t max_size = 0;
    for (int i = 0; i < s_size; ++i)
        max_size = std::max(max_size, num_vertices(s_list[i]));
    return max_size;
}

uint32_t PatternFinder::find_first_color(
    int32_t s_size,
    const std::vector<Graph>& s_list) const
{
    std::vector<std::pair<uint32_t, uint32_t>> color_count(
        m_color_map.size(), {0, 0});

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
    uint32_t /*new_node_id*/,
    uint32_t new_color,
    uint32_t node_to_connect_id,
    std::vector<std::vector<NodePtr>>& last_nodes)
{
    int32_t alive_count = 0;

    for (int i = 0; i < s_size; ++i) {
        if (!trees[i]) continue;

        std::vector<NodePtr> new_last_nodes;

        for (const NodePtr& lowest : last_nodes[i]) {
            auto node_in_tree =
                trees[i]->get_node_by_depth(lowest, node_to_connect_id);

            std::vector<int32_t> candidates;

            for (auto e :
                 boost::make_iterator_range(
                     out_edges(node_in_tree->index, s_list[i]))) {

                auto neigh = target(e, s_list[i]);
                if (s_list[i][neigh].color == static_cast<int>(new_color))
                    candidates.push_back(neigh);
            }

            auto added =
                trees[i]->add_tree_level(
                    lowest, candidates, s_list);

            new_last_nodes.insert(
                new_last_nodes.end(), added.begin(), added.end());
        }

        last_nodes[i] = new_last_nodes;

        if (!trees[i]->is_empty())
            alive_count++;
        else
            trees[i].reset();
    }

    return alive_count;
}

/* ---------- Main algorithm ---------- */

Graph PatternFinder::find_pattern(
    int32_t s_size,
    std::vector<Graph> s_list,
    double alive_threshold)
{
    PatternFinder pf;
    pf.map_colors(s_size, s_list);

    size_t max_s = find_max_s(s_size, s_list);
    auto color_hist =
        std::make_shared<ColorHist>(pf.m_color_map.size(), max_s);

    std::vector<std::shared_ptr<Tree>> trees(s_size);
    for (int i = 0; i < s_size; ++i)
        trees[i] = std::make_shared<Tree>(i, color_hist);

    std::vector<std::vector<NodePtr>> last_nodes(s_size);

    uint32_t first_color =
        pf.find_first_color(s_size, s_list);

    for (int i = 0; i < s_size; ++i) {
        auto matches =
            find_initial_matches(s_list[i], first_color);

        last_nodes[i] =
            trees[i]->add_tree_level(
                trees[i]->get_root(), matches, s_list);
    }

    Graph pattern;
    int32_t alive_s = s_size;

    while (alive_s > alive_threshold * s_size) {
        auto [color_new, node_to_connect] =
            color_hist->get_color_to_add();

        if (color_new < 0) break;

        uint32_t new_node_id =
            boost::add_vertex(
                VertexProperty{static_cast<int>(color_new)}, pattern);

        boost::add_edge(
            node_to_connect, new_node_id,
            EdgeProperty{false}, pattern);

        alive_s =
            extend_pattern_at_node_find_matches_in_s(
                trees, s_size, s_list,
                new_node_id, color_new,
                node_to_connect, last_nodes);
    }

    return pattern;
}
