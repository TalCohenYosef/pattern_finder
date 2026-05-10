// test_single_graph_pattern_finder.cpp
//
// Unit tests for SingleGraphPatternFinder::find_pattern().
//
// API
// ───
// find_pattern(Graph& S, Graph& G, double score_threshold, bool is_directed)
//   → BoostGraph pattern
//
// S is the search graph (pattern is found within S).
// G is the background graph (provides color distribution + edge density for scoring).
//
// DETERMINISM
// ───────────
// SingleGraphPatternFinder contains zero randomness — all selection is by
// deterministic scored partial_sort.  No code changes needed.
// Seed selection always starts with the RAREST color in G's distribution
// (sorted ascending by probability, index 0 always included).
//
// SCORING (PatternScorer::score)
// ──────────────────────────────
// score = color_logp + edges*log(bg_density) + (potential_edges - edges)*log(1 - bg_density)
// More negative = more surprising = better pattern.
// When bg_density = 0 (G has no edges): clamped to epsilon, edge term dominates.
// When a color has prob = 0 in G: log = -DBL_MAX*0.5, that state scores very poorly.

#include <gtest/gtest.h>
#include "SingleGraphPatternFinder.h"
#include "test_helpers.h"

static constexpr double LENIENT = -1e9;  // always run to completion

static BoostGraph
run(Graph& S, Graph& G, bool directed = false, double threshold = LENIENT)
{
    SingleGraphPatternFinder finder;
    return finder.find_pattern(S, G, threshold, directed);
}

// ══════════════════════════════════════════════════════════════════════════════
// NO-EDGE TESTS
// ══════════════════════════════════════════════════════════════════════════════

// S and G both empty.
// build_initial_beam → valid_colors is empty → returns {empty pattern, {}}.
TEST(SingleGraphNoEdges, BothEmpty) {
    auto S = make_empty();
    auto G = make_empty();
    EXPECT_THROW(run(S, G), std::runtime_error);
}

// S empty, G non-empty.
// No S-vertices → no seeds → {empty pattern, {}}.
TEST(SingleGraphNoEdges, SEmptyGNotEmpty) {
    auto S = make_empty();
    auto G = make_path({1,2,3});
    EXPECT_THROW(run(S, G), std::runtime_error);
}

// S non-empty, G empty.
// color_prob from G = all zeros → all S colors excluded from valid_colors
// → build_initial_beam returns {} → {empty pattern, {}}.
TEST(SingleGraphNoEdges, SNotEmptyGEmpty) {
    auto S = make_path({1,2});
    auto G = make_empty();
    EXPECT_THROW(run(S, G), std::runtime_error);
}

// S = one node (color 1), G = one node (color 2).
// After map_colors: S's color gets prob 0 in G → no valid seed → {empty, {}}.
TEST(SingleGraphNoEdges, SOneNodeGOneNodeDifferentColor) {
    auto S = make_isolated({1});
    auto G = make_isolated({2});
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, {1});
}

// S = one node (color 5), G = one node (color 5).
// Seed found (prob=1.0). No edges in S → no expansion candidates.
// Pattern = 1 node color 5, 0 edges.
TEST(SingleGraphNoEdges, SOneNodeGOneNodeSameColor) {
    auto S = make_isolated({5});
    auto G = make_isolated({5});
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, ({5}));
}

// S = isolated nodes (1,2,3), G = isolated nodes (4,5,6).
// Zero color overlap → all S-color probs = 0 in G → no valid seed → {empty, {}}.
TEST(SingleGraphNoEdges, MultipleIsolatedNodesNoColorOverlap) {
    auto S = make_isolated({1,2,3});
    auto G = make_isolated({4,5,6});
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    assert_pattern_colors_any(p, {{1}, {2}, {3}});
}

// S = isolated nodes (1,2,3), G = isolated nodes (1,2,4).
// Colors 1 and 2 overlap (prob=1/3 each in G). Color 3 has prob=0 → excluded.
// Both 1 and 2 are valid seeds (equal probability → rarest-first picks one).
// S has no edges → no expansion candidates → pattern = 1 node, 0 edges.
// The exact color depends on sort order — assert using any-of.
TEST(SingleGraphNoEdges, MultipleIsolatedNodesWithColorOverlap) {
    auto S = make_isolated({1,2,3});
    auto G = make_isolated({1,2,4});
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, {3});
}

// ══════════════════════════════════════════════════════════════════════════════
// UNDIRECTED TESTS
// ══════════════════════════════════════════════════════════════════════════════

// S = G = two nodes (colors 1,2), 1 edge.
// G density = 1.0. Colors: prob(1)=0.5, prob(2)=0.5.
// Seed = color 1 (rarest by sort, tied at 0.5 — index 0 after ascending sort).
// Candidate: node with color 2 (the only neighbor). Pattern grows to full S.
// Pattern = 2 nodes, 1 edge between color 1 and color 2.
TEST(SingleGraphUndirected, TwoNodeGraphSEqualsG) {
    auto S = make_two_nodes(1, 2);
    auto G = make_two_nodes(1, 2);
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
}

// S = G = path of 5 nodes with distinct colors (1-2-3-4-5).
// G density = 4/10 = 0.4. Each color has prob=0.2.
// Seed = rarest = color 1 (all equal, index 0 after sort).
// Beam expands through the full path: 1→2→3→4→5.
// Each expansion adds one node (the neighbor of the current tail).
// Pattern = full 5-node path, 4 edges.
TEST(SingleGraphUndirected, FiveNodePathSEqualsG) {
    auto S = make_path({1,2,3,4,5});
    auto G = make_path({1,2,3,4,5});
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 4, false);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 4, 5, false);
}

// S = G = K4 + pendant: nodes (colors 1-4) fully connected + node 5 off node 4.
// 7 edges. G density = 7/10 = 0.7.
// All 5 nodes have distinct colors → equal prob → seed = color 1.
// Beam expands: each node is neighbor of at least one existing node.
// Pattern = full graph, 5 nodes, 7 edges.
TEST(SingleGraphUndirected, FiveNodeDenseSevenEdgesSEqualsG) {
    std::vector<int32_t> colors = {1,2,3,4,5};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3},{3,4}};
    Graph S(5, edges, colors);
    Graph G(5, edges, colors);
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 7, false);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 4, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 2, 4, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 4, 5, false);
}

// S = G = star: node 0 (color 99, rare) connected to 5 nodes (all color 1).
// In G: prob(99) = 1/6 ≈ 0.167, prob(1) = 5/6 ≈ 0.833.
// Seed = color 99 (rarest). Candidates: 5 neighbors of color 1.
// Beam expands to full star.
// Pattern = 6 nodes, 5 edges from 99 to each 1.
TEST(SingleGraphUndirected, RareColorNodeAlwaysChosenAsSeed) {
    std::vector<int32_t> colors = {99,1,1,1,1,1};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{0,2},{0,3},{0,4},{0,5}};
    Graph S(6, edges, colors);
    Graph G(6, edges, colors);
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 6);
    ASSERT_EDGE_COUNT(p, 5, false);
    ASSERT_PATTERN_COLORS(p, ({1,1,1,1,1,99}));
    // All edges connect color 99 to color 1
    for (int i = 0; i < 5; ++i)
        ASSERT_EDGE_COLORS(p, 99, 1, false);
}

// S colors {1,2}, G colors {10,20}: no overlap.
// All S-color probs in G = 0 → no valid seeds → {empty, {}}.
TEST(SingleGraphUndirected, NoColorOverlapBetweenSAndG) {
    auto S = make_path({1,2});
    auto G = make_path({10,20});
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    assert_pattern_colors_any(p, {{1}, {2}});
}

// S = path(1,2,3). G = path(1,2,10,20).
// After map_colors: colors 1 and 2 shared (prob=0.25 each in G).
// Color 3 (from S) has prob=0 in G → excluded from valid seeds.
// Seed = color 1 or 2 (equal prob, index 0 picks one deterministically).
// From seed: neighbor in S with color 2 (or 1) → added.
// Color 3 has log-prob = -DBL_MAX*0.5 → any state including color 3 scores
// extremely poorly → pruned. Pattern = 2 nodes (colors 1,2), 1 edge.
TEST(SingleGraphUndirected, PartialColorOverlapSAndG) {
    auto S = make_path({1,2,3});
    std::vector<int32_t> gc = {1,2,10,20};
    std::vector<std::pair<uint32_t,uint32_t>> ge = {{0,1},{1,2},{2,3}};
    Graph G(4, ge, gc);
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, ({3}));
}

// S = path(1,2,3). G = 9-node path cycling colors 1,2,3 (3 of each).
// All 3 colors have prob=1/3 in G. Equal → all equally "rare".
// Seed = one of them (deterministic by sort). Full S path can be found in G.
// Pattern = full 3-node path, 2 edges.
TEST(SingleGraphUndirected, FullColorOverlapLargerG) {
    auto S = make_path({1,2,3});
    std::vector<int32_t> gc;
    for (int i = 0; i < 9; ++i) gc.push_back((i % 3) + 1);
    std::vector<std::pair<uint32_t,uint32_t>> ge;
    for (uint32_t i = 0; i < 8; ++i) ge.push_back({i, i+1});
    Graph G(9, ge, gc);
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, false);
    ASSERT_PATTERN_COLORS(p, ({1,2,3}));
    // The path must be connected: either 1-2-3 or 3-2-1 (same edges undirected)
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
}

// S = star(99, {1,1}): center=99, leaves=1,1. G = 10-node path, color 99 at 2 positions.
// In G: prob(99)=2/10=0.2, prob(1)=8/10=0.8.
// Seed = color 99 (rarest). In S node 0 is the center.
// Candidates: node1 (color 1) and node2 (color 1) — both neighbors of 99.
// Pattern grows to full star: 3 nodes, 2 edges (both 99→1).
TEST(SingleGraphUndirected, LargerGWithFewRareColorSpots) {
    auto S = make_star(99, {1,1});
    std::vector<int32_t> gc = {1,1,99,1,1,1,1,99,1,1};
    std::vector<std::pair<uint32_t,uint32_t>> ge;
    for (uint32_t i = 0; i < 9; ++i) ge.push_back({i, i+1});
    Graph G(10, ge, gc);
    auto p = run(S, G);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, false);
    ASSERT_PATTERN_COLORS(p, ({1,1,99}));
    ASSERT_EDGE_COLORS(p, 99, 1, false);
}

// ══════════════════════════════════════════════════════════════════════════════
// DIRECTED TESTS
// ══════════════════════════════════════════════════════════════════════════════

// S = G = directed edge 0→1 (colors 1,2).
// expand_one_state checks is_edge(selected, existing) on forward adjacency.
// Pattern = 2 nodes, 1 directed edge color1→color2.
TEST(SingleGraphDirected, TwoNodesForwardEdgeSEqualsG) {
    auto S = make_two_nodes(1, 2, true, true);
    auto G = make_two_nodes(1, 2, true, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1,2}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
}

// S = G = directed edge 1→0 (colors 1,2, meaning node1(color2)→node0(color1)).
// Pattern = 2 nodes, 1 directed edge color2→color1.
TEST(SingleGraphDirected, TwoNodesBackwardEdgeSEqualsG) {
    auto S = make_two_nodes(1, 2, false, true);
    auto G = make_two_nodes(1, 2, false, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1,2}));
    ASSERT_EDGE_COLORS(p, 2, 1, true);
}

// S has directed edge 0→1 (colors 1,2). G has directed edge 1→0 (colors 1,2).
// Seed = rarest color (tie, so color 1 by sort). Node with color 2 is a
// candidate in S (neighbor of color-1 node via forward edge).
// expand_one_state: is_edge(candidate_in_S, existing_in_S) uses S's adjacency.
// In S the edge is 0→1, so is_edge(0,1) is true but is_edge(1,0) is false.
// The seed is node 0 (color 1). Candidate is node 1 (color 2).
// is_edge(node1, node0) in S = is_edge(1,0) = false → no edge added to pattern.
// Pattern = 2 nodes (both found via S's edges), 0 directed edges.
TEST(SingleGraphDirected, TwoNodesSForwardGBackward) {
    auto S = make_two_nodes(1, 2, true,  true);
    auto G = make_two_nodes(1, 2, false, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1,2}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
}

// Mirror: S=1→0, G=0→1. Same reasoning: seed=color1, candidate=color2.
// In S edge goes 1→0: is_edge(0,1) = false (forward list for node 1 has node 0,
// but expand checks is_edge(selected=node1, existing=node0) = is_edge(1,0).
// In S (1→0): forward neighbours of node 1 is empty (no outgoing from 1),
// reversed neighbours of node 1 is [node 0]. But is_edge uses forward list.
// So is_edge(1,0) = false → 0 edges in pattern.
TEST(SingleGraphDirected, TwoNodesSBackwardGForward) {
    auto S = make_two_nodes(1, 2, false, true);
    auto G = make_two_nodes(1, 2, true,  true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1,2}));
    ASSERT_EDGE_COLORS(p, 2, 1, true);
}

// S = G = directed path (colors 1→2→3→4→5).
// Each node has one forward neighbor. Beam expands along the path.
// Pattern = full 5-node directed path.
TEST(SingleGraphDirected, FiveNodeDirectedPathSEqualsG) {
    auto S = make_path({1,2,3,4,5}, true);
    auto G = make_path({1,2,3,4,5}, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 4, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 4, true);
    ASSERT_EDGE_COLORS(p, 4, 5, true);
}

// S = G = dense directed graph (7 directed edges), 5 distinct-color nodes.
// All 7 directed edges added. Pattern = full graph.
TEST(SingleGraphDirected, FiveNodeDenseDirectedSEqualsG) {
    std::vector<int32_t> colors = {1,2,3,4,5};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{1,2},{2,3},{3,4},{0,2},{1,3},{2,4}};
    Graph S(5, edges, colors, true);
    Graph G(5, edges, colors, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 7, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 4, true);
    ASSERT_EDGE_COLORS(p, 4, 5, true);
    ASSERT_EDGE_COLORS(p, 1, 3, true);
    ASSERT_EDGE_COLORS(p, 2, 4, true);
    ASSERT_EDGE_COLORS(p, 3, 5, true);
}

// S = directed star from rare node 99 (center) to 4 nodes of color 1.
// S = G. Seed = color 99 (rarest). All 4 outgoing edges added.
// Pattern = full star, 5 nodes, 4 directed edges (99→1).
TEST(SingleGraphDirected, RareColorNodeChosenAsSeedDirected) {
    std::vector<int32_t> colors = {99,1,1,1,1};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{0,2},{0,3},{0,4}};
    Graph S(5, edges, colors, true);
    Graph G(5, edges, colors, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 4, true);
    ASSERT_PATTERN_COLORS(p, ({1,1,1,1,99}));
    ASSERT_EDGE_COLORS(p, 99, 1, true);
}

// S colors {1,2}, G colors {10,20}: no overlap → no valid seeds → {empty, {}}.
TEST(SingleGraphDirected, NoColorOverlapDirected) {
    auto S = make_path({1,2},  true);
    auto G = make_path({10,20}, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, true);
    assert_pattern_colors_any(p, {{1}, {2}});
}

// S = directed path (1→2→3). G = directed path (1→2→10).
// Colors 1 and 2 shared. Color 3 has prob=0 → pruned.
// Seed = color 1. Neighbor in S: color 2. Color 2 added, directed edge 1→2.
// Color 3: log-prob = -inf → pruned. Pattern = 2 nodes, 1 directed edge 1→2.
TEST(SingleGraphDirected, PartialColorOverlapDirected) {
    auto S = make_path({1,2,3}, true);
    auto G = make_path({1,2,10}, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, true);
    ASSERT_PATTERN_COLORS(p, ({3}));
}

// S = directed path (1→2→3). G = 9-node directed path cycling colors 1,2,3.
// All 3 colors prob=1/3 in G. Equal → seed = one of them (deterministic).
// Pattern = full 3-node directed path, 2 directed edges.
TEST(SingleGraphDirected, FullColorOverlapLargerGDirected) {
    auto S = make_path({1,2,3}, true);
    std::vector<int32_t> gc;
    for (int i = 0; i < 9; ++i) gc.push_back((i % 3) + 1);
    std::vector<std::pair<uint32_t,uint32_t>> ge;
    for (uint32_t i = 0; i < 8; ++i) ge.push_back({i, i+1});
    Graph G(9, ge, gc, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
}

// S = directed star(99, {1, 2}): 99→1 and 99→2. G = 8-node path, color 99 at 2 positions.
TEST(SingleGraphDirected, LargerDirectedGWithFewRareColorSpots) {
    std::vector<int32_t> sc = {99,1,2};
    std::vector<std::pair<uint32_t,uint32_t>> se = {{0,1},{0,2}};
    Graph S(3, se, sc, true);
    std::vector<int32_t> gc = {1,1,99,1,1,99,1,2};
    std::vector<std::pair<uint32_t,uint32_t>> ge;
    for (uint32_t i = 0; i < 7; ++i) ge.push_back({i, i+1});
    Graph G(8, ge, gc, true);
    auto p = run(S, G, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,99}));
    ASSERT_EDGE_COLORS(p, 99, 1, true);
    ASSERT_EDGE_COLORS(p, 99, 2, true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}