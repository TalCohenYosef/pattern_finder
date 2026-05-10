#pragma once
// test_helpers.h
// Shared graph-building and pattern-assertion utilities.
// All functions are inline — include in multiple .cpp files without linker issues.

#include "Graph.h"
#include "BoostGraph.h"
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <vector>
#include <utility>
#include <cstdint>
#include <string>
#include <sstream>

// ══════════════════════════════════════════════════════════════════════════════
// Graph factory helpers
// ══════════════════════════════════════════════════════════════════════════════

inline Graph make_empty(bool directed = false) {
    std::vector<std::pair<uint32_t,uint32_t>> edges;
    std::vector<int32_t> colors;
    return Graph(0, edges, colors, directed);
}

inline Graph make_isolated(std::vector<int32_t> colors, bool directed = false) {
    std::vector<std::pair<uint32_t,uint32_t>> edges;
    return Graph(static_cast<uint32_t>(colors.size()), edges, colors, directed);
}

inline Graph make_path(std::vector<int32_t> colors, bool directed = false) {
    uint32_t n = static_cast<uint32_t>(colors.size());
    std::vector<std::pair<uint32_t,uint32_t>> edges;
    for (uint32_t i = 0; i + 1 < n; ++i)
        edges.push_back({i, i + 1});
    return Graph(n, edges, colors, directed);
}

// forward=true → edge 0→1, forward=false → edge 1→0
inline Graph make_two_nodes(int32_t c0, int32_t c1,
                             bool forward = true, bool directed = false) {
    std::vector<int32_t> colors = {c0, c1};
    std::vector<std::pair<uint32_t,uint32_t>> edges = {
        forward ? std::make_pair(0u,1u) : std::make_pair(1u,0u)
    };
    return Graph(2, edges, colors, directed);
}

// Undirected triangle: 0-1, 1-2, 0-2
inline Graph make_triangle(int32_t c0, int32_t c1, int32_t c2,
                            bool directed = false) {
    std::vector<int32_t> colors = {c0, c1, c2};
    std::vector<std::pair<uint32_t,uint32_t>> edges = {{0,1},{1,2},{0,2}};
    return Graph(3, edges, colors, directed);
}

// Directed triangle cycle: 0→1→2→0
inline Graph make_directed_triangle(int32_t c0, int32_t c1, int32_t c2) {
    std::vector<int32_t> colors = {c0, c1, c2};
    std::vector<std::pair<uint32_t,uint32_t>> edges = {{0,1},{1,2},{2,0}};
    return Graph(3, edges, colors, true);
}

// Star: node 0 is hub, edges 0→leaf_i
inline Graph make_star(int32_t center_color, std::vector<int32_t> leaf_colors,
                       bool directed = false) {
    uint32_t n = 1 + static_cast<uint32_t>(leaf_colors.size());
    std::vector<int32_t> colors = {center_color};
    for (int32_t c : leaf_colors) colors.push_back(c);
    std::vector<std::pair<uint32_t,uint32_t>> edges;
    for (uint32_t i = 1; i < n; ++i) edges.push_back({0, i});
    return Graph(n, edges, colors, directed);
}

// ══════════════════════════════════════════════════════════════════════════════
// Pattern inspection
// ══════════════════════════════════════════════════════════════════════════════

inline int pattern_node_count(const BoostGraph& p) {
    return static_cast<int>(boost::num_vertices(p));
}

// BoostGraph is always directedS internally.
// Undirected edges are stored as two arcs (u→v and v→u), so divide by 2.
inline int pattern_edge_count(const BoostGraph& p, bool directed = false) {
    int raw = static_cast<int>(boost::num_edges(p));
    return directed ? raw : raw / 2;
}

// Sorted multiset of all node colors in the pattern.
inline std::vector<int32_t> pattern_colors(const BoostGraph& p) {
    std::vector<int32_t> cs;
    for (auto v : boost::make_iterator_range(boost::vertices(p)))
        cs.push_back(p[v].color);
    std::sort(cs.begin(), cs.end());
    return cs;
}

inline bool pattern_has_color(const BoostGraph& p, int32_t color) {
    for (auto v : boost::make_iterator_range(boost::vertices(p)))
        if (p[v].color == color) return true;
    return false;
}

// Returns true if there is an edge between a vertex of from_color and one of
// to_color. For undirected (directed=false) checks both orientations.
inline bool pattern_has_edge_between_colors(const BoostGraph& p,
                                             int32_t from_color,
                                             int32_t to_color,
                                             bool directed = false) {
    for (auto e : boost::make_iterator_range(boost::edges(p))) {
        int32_t sc = p[boost::source(e, p)].color;
        int32_t tc = p[boost::target(e, p)].color;
        if (sc == from_color && tc == to_color) return true;
        if (!directed && sc == to_color && tc == from_color) return true;
    }
    return false;
}

inline bool pattern_colors_in(
    const BoostGraph& p,
    const std::vector<std::vector<int32_t>>& options)
{
    auto got = pattern_colors(p);

    for (auto opt : options) {
        std::sort(opt.begin(), opt.end());
        if (got == opt) return true;
    }
    return false;
}

inline bool pattern_edges_match(
    const BoostGraph& p,
    const std::vector<std::pair<int32_t,int32_t>>& edges,
    bool directed = false)
{
    for (const auto& [u, v] : edges) {
        if (!pattern_has_edge_between_colors(p, u, v, directed))
            return false;
    }
    return true;
}

inline bool pattern_edges_in(
    const BoostGraph& p,
    const std::vector<std::vector<std::pair<int32_t,int32_t>>>& options,
    bool directed = false)
{
    for (const auto& opt : options) {
        if (pattern_edges_match(p, opt, directed))
            return true;
    }
    return false;
}

// Human-readable dump of pattern structure for failure messages.
inline std::string pattern_to_string(const BoostGraph& p, bool directed = false) {
    std::ostringstream oss;
    oss << "nodes=[";
    for (auto v : boost::make_iterator_range(boost::vertices(p)))
        oss << "v" << v << ":c" << p[v].color << " ";
    oss << "] edges=[";
    for (auto e : boost::make_iterator_range(boost::edges(p))) {
        auto s = boost::source(e, p), t = boost::target(e, p);
        if (directed || s < t)
            oss << "c" << p[s].color << "->" << "c" << p[t].color << " ";
    }
    oss << "]";
    return oss.str();
}

inline void assert_pattern_colors_any(
    const BoostGraph& pattern,
    const std::vector<std::vector<int32_t>>& options)
{
    auto got = pattern_colors(pattern);

    for (auto opt : options) {
        std::sort(opt.begin(), opt.end());
        if (got == opt) return;
    }

    ADD_FAILURE() << "Pattern colors didn't match any option. Pattern: "
                  << pattern_to_string(pattern);
}

inline bool edges_match_option(
    const BoostGraph& p,
    const std::vector<std::pair<int32_t,int32_t>>& edges,
    bool directed)
{
    for (auto [u, v] : edges) {
        if (!pattern_has_edge_between_colors(p, u, v, directed))
            return false;
    }
    return true;
}

inline void assert_pattern_edges_any(
    const BoostGraph& pattern,
    const std::vector<std::vector<std::pair<int32_t,int32_t>>>& options,
    bool directed)
{
    for (const auto& opt : options) {
        if (edges_match_option(pattern, opt, directed))
            return;
    }

    ADD_FAILURE() << "Pattern edges didn't match any option. Pattern: "
                  << pattern_to_string(pattern, directed);
}

// ══════════════════════════════════════════════════════════════════════════════
// Exact structural assertion macros
//
// Tests use these instead of EXPECT_LE / EXPECT_GE so that every property
// of the pattern is checked precisely.  The pattern_to_string() is printed
// on failure so it is easy to see what was actually returned.
// ══════════════════════════════════════════════════════════════════════════════

// Assert exact sorted multiset of node colors.
// Usage: ASSERT_PATTERN_COLORS(p, ({1, 2, 3}));
#define ASSERT_PATTERN_COLORS(pattern, expected_init)                        \
    do {                                                                      \
        auto _got = pattern_colors(pattern);                                  \
        std::vector<int32_t> _exp expected_init;                              \
        std::sort(_exp.begin(), _exp.end());                                  \
        EXPECT_EQ(_got, _exp)                                                 \
            << "Pattern: " << pattern_to_string(pattern);                    \
    } while (0)

// Assert exact node count.
#define ASSERT_NODE_COUNT(pattern, n)                                        \
    EXPECT_EQ(pattern_node_count(pattern), (n))                              \
        << "Pattern: " << pattern_to_string(pattern)

// Assert exact edge count (directed=false divides raw count by 2).
#define ASSERT_EDGE_COUNT(pattern, n, directed)                              \
    EXPECT_EQ(pattern_edge_count(pattern, directed), (n))                    \
        << "Pattern: " << pattern_to_string(pattern, directed)

// Assert that an edge between vertices of the given colors exists.
#define ASSERT_EDGE_COLORS(pattern, from_c, to_c, directed)                 \
    EXPECT_TRUE(pattern_has_edge_between_colors(pattern,                     \
                    from_c, to_c, directed))                                  \
        << "Expected edge c" << (from_c) << "->c" << (to_c)                 \
        << "  Pattern: " << pattern_to_string(pattern, directed)

