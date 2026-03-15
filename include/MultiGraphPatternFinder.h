#pragma once

#include "Graph.h"
#include "BoostGraph.h"
#include "Tree.h"
#include "IndevidualColorHist.h"
#include "PatternUtils.h"

#include <unordered_set>
#include <utility>
#include <vector>
#include <map>
#include <memory>

/**
 * @class MultiGraphPatternFinder
 * @brief Implements the core pattern-growth algorithm over multiple S-graphs.
 *
 * The algorithm:
 *  - Recolors graphs to compact colour IDs (via PatternUtils)
 *  - Chooses an initial colour
 *  - Incrementally grows a pattern graph
 *  - Maintains per-S_i match trees with backtracking
 */
class MultiGraphPatternFinder {
private:
    static constexpr int32_t FIRST_COLOR_ORDER = 1;

private:
    /* ---------- Helpers ---------- */

    /**
     * Try to add a new edge to the current pattern.
     * Returns true if an edge was added, false otherwise.
     */
    static bool add_edge(BoostGraph& pattern,
                         std::vector<std::shared_ptr<Tree>>& trees,
                         std::vector<std::vector<NodePtr>>& last_nodes,
                         std::unordered_set<uint32_t>& alive_indexes,
                         uint32_t s_size,
                         const std::vector<Graph>& s_list,
                         double threshold,
                         double alive_threshold);

    /**
     * Compute support score for a candidate pattern edge (uP, vP).
     */
    static uint32_t score_edge_support(
        uint32_t uP, uint32_t vP,
        const std::vector<std::shared_ptr<Tree>>& trees,
        const std::vector<std::vector<NodePtr>>& last_nodes,
        const std::vector<Graph>& s_list, uint32_t s_size);

    /**
     * Apply an accepted edge to the pattern and prune unsupported trees.
     */
    static void apply_edge_and_prune(
        BoostGraph& pattern, uint32_t uP, uint32_t vP,
        std::vector<std::shared_ptr<Tree>>& trees,
        std::vector<std::vector<NodePtr>>& last_nodes,
        std::unordered_set<uint32_t>& alive_indexes,
        const std::vector<Graph>& s_list);

    static uint32_t find_first_color(uint32_t color_number,
                                     int32_t  s_size,
                                     const std::vector<Graph>& s_list);

    static std::pair<int32_t,int32_t> extend_pattern_at_node_find_matches_in_s(
        std::vector<std::shared_ptr<Tree>>& trees,
        int32_t s_size,
        const std::vector<Graph>& s_list,
        uint32_t new_node_id,
        uint32_t new_color,
        uint32_t node_to_connect_id,
        std::vector<std::vector<NodePtr>>& last_nodes,
        std::unordered_set<uint32_t>& alive_indexes);

public:
    /**
     * @brief Run the pattern-finding algorithm.
     *
     * @param s_list          Input graphs (modified in-place: colours remapped).
     * @param alive_threshold Fraction of S-graphs that must remain alive.
     * @return {pattern BoostGraph, alive_indexes}
     */
    static std::pair<BoostGraph, std::unordered_set<uint32_t>>
    find_pattern(std::vector<Graph>& s_list,
                 double alive_threshold);
};
