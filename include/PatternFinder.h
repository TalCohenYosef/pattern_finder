#pragma once

#include "Graph.h"
#include "Tree.h"
#include "ColorHist.h"

#include <vector>
#include <map>
#include <memory>

/**
 * @class PatternFinder
 * @brief Implements the core pattern-growth algorithm over multiple graphs S_i.
 *
 * The algorithm:
 *  - Recolors graphs to compact color IDs
 *  - Chooses an initial color
 *  - Incrementally grows a pattern graph
 *  - Maintains per-S_i match trees with backtracking
 */
class PatternFinder {
private:
    static constexpr int32_t FIRST_COLOR_ORDER = 1;

    /// Maps compact color index → original color
    std::vector<int32_t> m_color_map;

private:
    /* ---------- Helpers ---------- */

    static void recolor_s(
        const std::map<int32_t, uint32_t>& old_to_new,
        Graph& s);

    void map_colors(
        int32_t s_size,
        std::vector<Graph>& s_list);

    static size_t find_max_s(
        int32_t s_size,
        const std::vector<Graph>& s_list);

    uint32_t find_first_color(
        int32_t s_size,
        const std::vector<Graph>& s_list) const;

    static std::vector<uint32_t> find_initial_matches(
        const Graph& s,
        uint32_t color);

    static int32_t extend_pattern_at_node_find_matches_in_s(
        std::vector<std::shared_ptr<Tree>>& trees,
        int32_t s_size,
        const std::vector<Graph>& s_list,
        uint32_t new_node_id,
        uint32_t new_color,
        uint32_t node_to_connect_id,
        std::vector<std::vector<NodePtr>>& last_nodes);

public:
    /**
     * @brief Run the pattern-finding algorithm.
     *
     * @param s_size Number of S graphs
     * @param s_list Input graphs
     * @param alive_threshold Global alive threshold
     * @return Extracted pattern graph
     */
    static Graph find_pattern(
        int32_t s_size,
        std::vector<Graph> s_list,
        double alive_threshold);
};
