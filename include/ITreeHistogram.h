#pragma once

#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * @brief Contract between Tree and any histogram strategy.
 *
 * Tree fires exactly three events as the match tree changes shape, and asks
 * one query when it needs to select the next vertex.  All update methods
 * receive raw S-graph vertex indices; each implementation extracts whatever
 * it needs (colour, probability, edge count) without Tree knowing.
 *
 * The three events map 1-to-1 onto the three existing call sites in Tree:
 *
 *   on_neighbours_added    ← add_tree_level    (outside neighbours of new vertex)
 *   on_new_vertex_absorbed ← add_tree_level    (new vertex leaves outside pool)
 *   on_neighbours_removed  ← remove_node       (undo on_neighbours_added)
 */
class ITreeHistogram
{
public:
    virtual ~ITreeHistogram() = default;

    /**
     * @brief New pattern vertex added at @p depth; register its outside neighbours.
     *
     * @param depth                      0-based pattern depth of the new vertex.
     * @param outside_neighbour_vertices S-graph indices of neighbours NOT in path.
     * @param match_vertices             S-graph indices of all vertices in this
     *                                   specific match's path (used to determine
     *                                   which neighbours are inside vs outside).
     */
    virtual void on_neighbours_added(
        uint32_t                           depth,
        const std::vector<uint32_t>&       outside_neighbour_vertices,
        const std::unordered_set<uint32_t>& match_vertices) = 0;

    /**
     * @brief The vertex @p absorbed_s_vertex has just been incorporated into
     *        the pattern; remove its outside-neighbour contributions.
     *
     * @param absorbed_s_vertex  S-graph index of the newly absorbed vertex.
     * @param depths_in_path     Pattern depth of every in-pattern vertex that
     *                           is a direct neighbour of absorbed_s_vertex.
     *                           One entry per such edge (the inside edges it adds).
     */
    virtual void on_new_vertex_absorbed(
        uint32_t                     absorbed_s_vertex,
        const std::vector<uint32_t>& depths_in_path) = 0;

    /**
     * @brief Match node removed (backtrack); undo the matching on_neighbours_added.
     *
     * @param depth                      Pattern depth being removed.
     * @param outside_neighbour_vertices Same S-graph indices passed to the
     *                                   corresponding on_neighbours_added call.
     */
    virtual void on_neighbours_removed(
        uint32_t                     depth,
        const std::vector<uint32_t>& outside_neighbour_vertices) = 0;

    /**
     * @brief Select the next vertex (and the pattern node to attach it to).
     *
     * @return {selection, depth_to_connect}.  {-1,-1} means no valid extension.
     *
     * IndevidualColorHist returns {colour_index, pattern_depth} matching the
     * original GeneralColorHist::get_color_to_add contract.
     * SingleGraphHistogram returns {s_vertex_index, s_vertex_index_of_connect_point}.
     */
    virtual std::pair<int32_t, int32_t> get_next_vertex() = 0;
};
