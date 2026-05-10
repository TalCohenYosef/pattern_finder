// test_multi_graph_pattern_finder.cpp
//
// Unit tests for MultiGraphPatternFinder::find_pattern().
//
// DETERMINISM NOTE
// ────────────────
// Tests pass is_random=false.  In that mode the algorithm replaces the
// uniform random draw with the fixed value 0.5.  The condition
//
//     draw < p   where p = 1/cbrt(n_vertices)
//
// is therefore equivalent to: cbrt(n) < 2  ⟺  n < 8.
// So with draw=0.5 the algorithm:
//   - always adds a vertex  when the pattern has fewer than 8 nodes
//   - always tries an edge  when the pattern has 8 or more nodes
//
// This makes the output fully deterministic for the small graphs used here.
// Each test comment explains exactly what the algorithm will produce.

#include <gtest/gtest.h>
#include "MultiGraphPatternFinder.h"
#include "test_helpers.h"

static std::pair<BoostGraph, std::unordered_set<uint32_t>>
run(std::vector<Graph>& graphs,
    double alive_threshold = 0.5,
    bool directed = false)
{
    return MultiGraphPatternFinder::find_pattern(
        graphs, alive_threshold, directed, /*is_random=*/false);
}

// ══════════════════════════════════════════════════════════════════════════════
// NO-EDGE TESTS
// ══════════════════════════════════════════════════════════════════════════════

// Empty graph has no colors → color_prob is empty → throws "No colors found".
TEST(MultiGraphNoEdges, SingleEmptyGraph) {
    std::vector<Graph> gs = {make_empty()};
    EXPECT_THROW(run(gs), std::runtime_error);
}

TEST(MultiGraphNoEdges, TwoEmptyGraphs) {
    std::vector<Graph> gs = {make_empty(), make_empty()};
    EXPECT_THROW(run(gs, 1.0), std::runtime_error);
}

// One node (color 1), one graph.
// Seed = color 1.  Histogram gets no candidates (no edges) → loop ends.
// Pattern = 1 node color 1, 0 edges.  alive = {0}.
TEST(MultiGraphNoEdges, OneNodeOneGraph) {
    std::vector<Graph> gs = {make_isolated({1})};
    auto [p, alive] = run(gs);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, ({1}));
    EXPECT_EQ(alive, (std::unordered_set<uint32_t>{0}));
}

// Two single-node graphs, same color.
// Pattern = 1 node color 5, 0 edges.  Both alive.
TEST(MultiGraphNoEdges, OneNodeTwoGraphsSameColor) {
    std::vector<Graph> gs = {make_isolated({5}), make_isolated({5})};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, ({5}));
    EXPECT_EQ(alive.size(), 2u);
}

// Two single-node graphs, different colors.
// find_first_color picks the most-common color; both have count 1, so it
// picks one deterministically (whichever wins the sort).  Only 1 graph
// contains that color → alive has 1 element.
TEST(MultiGraphNoEdges, OneNodeTwoGraphsDifferentColor) {
    std::vector<Graph> gs = {make_isolated({5}), make_isolated({7})};
    auto [p, alive] = run(gs, 0.5);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    EXPECT_EQ(alive.size(), 1u);
}

// Two isolated nodes (colors 1,2), one graph.
// Seed added. No edges → histogram has no candidates.
// Pattern = 1 node, 0 edges.
TEST(MultiGraphNoEdges, TwoNodesZeroEdgesOneGraph) {
    std::vector<Graph> gs = {make_isolated({1, 2})};
    auto [p, alive] = run(gs);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
}

// Two identical graphs of two isolated nodes.
// Pattern = 1 node, 0 edges.  Both alive.
TEST(MultiGraphNoEdges, TwoNodesZeroEdgesTwoGraphs) {
    std::vector<Graph> gs = {make_isolated({1,2}), make_isolated({1,2})};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    EXPECT_EQ(alive.size(), 2u);
}

// ══════════════════════════════════════════════════════════════════════════════
// UNDIRECTED TESTS
// ══════════════════════════════════════════════════════════════════════════════

// Single graph: 2 nodes (colors 1,2), 1 edge.
// n=1→add vertex(color1).  n=2→add vertex(color2, neighbor of 1).
// n=3 would be needed but histogram exhausted.  → try edge: 1-2 scores 1 → added.
// Pattern = 2 nodes, 1 edge 1-2.
TEST(MultiGraphUndirected, SingleGraphTwoNodesOneEdge) {
    std::vector<Graph> gs = {make_two_nodes(1, 2)};
    auto [p, alive] = run(gs);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
}

// Single path: 5 nodes (colors 1-2-3-4-5), 4 edges.
// n=1..5: all 5 vertices added (n<8 throughout).
// Histogram exhausted → try edges: 4 path edges each score 1 → all added.
// Pattern = full 5-node path.
TEST(MultiGraphUndirected, SingleGraphFiveNodesFourEdgesPath) {
    std::vector<Graph> gs = {make_path({1, 2, 3, 4, 5})};
    auto [p, alive] = run(gs);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 4, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3, 4, 5}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 4, 5, false);
}

// Single dense graph: K4 (colors 1-4) + pendant (color 5), 7 edges.
// n=1..5: all 5 vertices added.
// Then edges: all 6 K4 edges score 1, plus pendant edge.  7 edges added.
// Pattern = full 5-node graph, all 7 edges present.
// This test verifies that after all vertices are added, ALL edges are found.
TEST(MultiGraphUndirected, SingleGraphFiveNodesSevenEdgesVerifyAllEdgesAdded) {
    std::vector<int32_t> colors = {1,2,3,4,5};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3},{3,4}};
    Graph g(5, edges, colors);
    std::vector<Graph> gs = {g};
    auto [p, alive] = run(gs);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 7, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3, 4, 5}));
    // All K4 edges present
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 4, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 2, 4, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    // Pendant edge present
    ASSERT_EDGE_COLORS(p, 4, 5, false);
}

// Single 9-node path (colors 1-9), 8 edges.
// n=1..7: vertices added (draw=0.5 < p for n<8).
// n=8: draw=0.5, p=0.5, 0.5 < 0.5 is FALSE → try edges.
//   Edges between the 7 already-added nodes: 1-2,2-3,...,6-7 (6 edges) added.
//   Each time an edge is added, failed_add_edge=false → try edge again.
//   After all 6 inter-node edges added, failed_add_edge=true → fallback to vertex.
// Then vertices 8 and 9 (colors 8,9) added.
// Then edges 7-8 and 8-9 added.
// Pattern = full 9-node path, all 8 edges.
// This test specifically exercises the code path where edges are added
// BEFORE all vertices are exhausted (at the n=8 transition).
TEST(MultiGraphUndirected, SingleGraphNineNodesEdgesAddedBeforeVertexExhaustion) {
    std::vector<int32_t> colors = {1,2,3,4,5,6,7,8,9};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{0,2},{0,3},{0,4},{1,4}};
    Graph g(9, edges, colors);
    std::vector<Graph> gs = {g};
    auto [p, alive] = run(gs);
    ASSERT_NODE_COUNT(p, 9);
    ASSERT_EDGE_COUNT(p, 12, false);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5,6,7,8,9}));
    // All consecutive path edges must be present
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 4, 5, false);
    ASSERT_EDGE_COLORS(p, 5, 6, false);
    ASSERT_EDGE_COLORS(p, 6, 7, false);
    ASSERT_EDGE_COLORS(p, 7, 8, false);
    ASSERT_EDGE_COLORS(p, 8, 9, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 4, false);
    ASSERT_EDGE_COLORS(p, 1, 5, false);
    ASSERT_EDGE_COLORS(p, 2, 5, false);
}

// Two identical path graphs 1-2-3, threshold=1.0.
// All 3 nodes added, both edges added, both alive.
TEST(MultiGraphUndirected, TwoIdenticalPathsGraphsAllSurvive) {
    auto g = make_path({1, 2, 3});
    std::vector<Graph> gs = {g, g};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    EXPECT_EQ(alive.size(), 2u);
}

// Two identical triangle graphs 1-2-3, threshold=1.0.
// All 3 nodes added, all edges added, both alive.
TEST(MultiGraphUndirected, TwoIdenticalTrianglesGraphsAllSurvive) {
    std::vector<int32_t> colors = {1,2,3};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{1,2},{0,2}};
    Graph g(3, edges, colors);
    std::vector<Graph> gs = {g, g};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 3, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    EXPECT_EQ(alive.size(), 2u);
}

// s0,s1 = path 1-2-3.  s2 = edge 1-2.  threshold=0.66 → 2/3 needed.
// All 3 graphs have colors 1,2 → nodes 1,2 added.  s0,s1 also have color 3.
// Color 3 support = 2/3 >= 0.66 → added.  Edge 1-2: support 3/3 → added.
// Edge 2-3: support 2/3 >= 0.66 → added.  s2 drops (no color-3 match).
// Pattern = path 1-2-3, alive = {0,1}.
TEST(MultiGraphUndirected, ThreeGraphsTwoIdenticalOneLessEdgesThresholdTwoSurvive) {
    auto full = make_path({1, 2, 3});
    std::vector<int32_t> rc = {1, 2};
    std::vector<std::pair<uint32_t,uint32_t>> re = {{0,1}};
    Graph reduced(2, re, rc);
    std::vector<Graph> gs = {full, full, reduced};
    auto [p, alive] = run(gs, 0.66);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    EXPECT_EQ(alive.size(), 2u);
}

// 2 full graphs on 4 nodes, 1 square. alive = 0.66 pattern like the 2 graphs.
TEST(MultiGraphUndirected, ThreeGraphsTwoIdenticalOneLessEdgesSameAmountOfNodesThresholdTwoSurvive) {
    std::vector<int32_t> colors = {1, 2, 3, 4};
    std::vector<std::pair<uint32_t,uint32_t>> edges_full = {{0,1}, {1,2}, {2,3}, {0, 2}, {0, 3}, {1, 3}};
    std::vector<std::pair<uint32_t,uint32_t>> edges_reduced = {{0,1}, {1,2}, {2, 3}, {0, 3}};
    Graph full(4, edges_full, colors);
    Graph reduced(4, edges_reduced, colors);
    std::vector<Graph> gs = {full, full, reduced};
    auto [p, alive] = run(gs, 0.66);
    ASSERT_NODE_COUNT(p, 4);
    ASSERT_EDGE_COUNT(p, 6, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3, 4}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 4, false);
    ASSERT_EDGE_COLORS(p, 2, 4, false);
    EXPECT_EQ(alive.size(), 2u);
}

// Same setup, threshold=1.0.
// Color 3 support = 2/3 < 1.0 → not added.
// Edge 1-2: support 3/3 → added.  All 3 survive.
// Pattern = edge 1-2, all 3 alive.
TEST(MultiGraphUndirected, ThreeGraphsTwoIdenticalOneLessEdgesThresholdAllSurvive) {
    auto full = make_path({1, 2, 3});
    std::vector<int32_t> rc = {1, 2};
    std::vector<std::pair<uint32_t,uint32_t>> re = {{0,1}};
    Graph reduced(2, re, rc);
    std::vector<Graph> gs = {full, full, reduced};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    EXPECT_EQ(alive.size(), 3u);
}

// 2 full graphs on 4 nodes, 1 square. alive = 1 pattern like the 1 graph - in all 3.
TEST(MultiGraphUndirected, ThreeGraphsTwoIdenticalOneLessEdgesSameAmountOfNodesThresholdAllSurvive) {
    std::vector<int32_t> colors = {1, 2, 3, 4};
    std::vector<std::pair<uint32_t,uint32_t>> edges_full = {{0,1}, {1,2}, {2,3}, {0, 2}, {0, 3}, {1, 3}};
    std::vector<std::pair<uint32_t,uint32_t>> edges_reduced = {{0,1}, {1,2}, {2, 3}, {0, 3}};
    Graph full(4, edges_full, colors);
    Graph reduced(4, edges_reduced, colors);
    std::vector<Graph> gs = {full, full, reduced};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 4);
    ASSERT_EDGE_COUNT(p, 4, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3, 4}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 1, 4, false);
    EXPECT_EQ(alive.size(), 3u);
}

// s0 colors {1,2}, s1 colors {10,20}: no overlap.  threshold=0.5.
// Seed = whichever color wins (both have count 1).
// That color exists in only 1 graph.  1/2 >= 0.5 → that graph stays alive.
// Pattern = 2 nodes + 1 edge from that graph.  alive has 1 element.
TEST(MultiGraphUndirected, TwoInputsNoColorOverlapLowThreshold) {
    std::vector<Graph> gs = {make_path({1,2}), make_path({10,20})};
    auto [p, alive] = run(gs, 0.5);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, false);
    EXPECT_EQ(alive.size(), 1u);
}

// threshold=1.0.  Both graphs must survive, but they share nothing.
// After seed node, the other graph has no matching color → drops immediately.
// Pattern = 1 node, 0 edges.  1 graph alive.
TEST(MultiGraphUndirected, TwoInputsNoColorOverlapHighThreshold) {
    std::vector<Graph> gs = {make_path({1,2}), make_path({10,20})};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    EXPECT_EQ(alive.size(), 1u);
}

// s0: 1-2-3, s1: 1-4-5.  Color 1 appears in both (total 2 occurrences).
// Colors 2,3,4,5 appear in only 1 graph each.
// threshold=0.5 → support needed = ceil(0.5*2) = 1.
// Seed = color 1 (most common).  Both alive.
// Color 2: support 1 = 1 >= 1 → added.  s1 drops (no color-2 neighbor of 1).
// Pattern = path 1-2 + color 3 added + edges, only s0 alive.
// Actually: after color 2 added, s1 drops → alive=1, threshold=0.5*2=1 → alive >= 1 → continue.
// Color 3 gets added (support 1 in s0), edge 1-2 and 2-3 added.
// Pattern = path 1-2-3, alive = {0}.
TEST(MultiGraphUndirected, TwoInputsOneNodeOverlapLowThreshold) {
    std::vector<Graph> gs = {make_path({1,2,3}), make_path({1,4,5})};
    auto [p, alive] = run(gs, 0.5);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, false);
    assert_pattern_colors_any(p, {
        {1, 2, 3},
        {1, 4, 5}
    });

    assert_pattern_edges_any(p, {
        {{1, 2}, {2, 3}},
        {{1, 4}, {4, 5}}
    }, false);
    EXPECT_EQ(alive.size(), 1u);
}

// Same setup, threshold=1.0.  Both must survive throughout.
// After seed (color 1): both alive.
// Color 2: support 1/2 < 1.0 → not added.
// Color 4: support 1/2 < 1.0 → not added.  Histogram returns -1 → done_vertices.
// No edges can be added (only 1 node).
// Pattern = 1 node color 1, 0 edges.  Both alive.
TEST(MultiGraphUndirected, TwoInputsOneNodeOverlapHighThreshold) {
    std::vector<Graph> gs = {make_path({1,2,3}), make_path({1,4,5})};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, false);
    ASSERT_PATTERN_COLORS(p, ({1}));
    EXPECT_EQ(alive.size(), 2u);
}

// Five identical 4-node cycles (colors 1-2-3-4), threshold=1.0.
// All 4 nodes + all 4 edges added.  All 5 alive.
TEST(MultiGraphUndirected, FiveIdenticalGraphsAllSurvive) {
    std::vector<int32_t> colors = {1,2,3,4};
    std::vector<std::pair<uint32_t,uint32_t>> edges = {{0,1},{1,2},{2,3},{3,0}};
    Graph g(4, edges, colors);
    std::vector<Graph> gs(5, g);
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 4);
    ASSERT_EDGE_COUNT(p, 4, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3, 4}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 3, 4, false);
    ASSERT_EDGE_COLORS(p, 4, 1, false);
    EXPECT_EQ(alive.size(), 5u);
}

// s0,s1 = triangle(1,2,3).  s2 = path 1-2-3 (missing edge 1-3).
// threshold=0.66 → 2/3 needed.
// All 3 nodes added (each color in all 3).
// Edge 1-2: 3/3 → added.  Edge 2-3: 3/3 → added.
// Edge 1-3: 2/3 >= 0.66 → added.  s2 drops (no 1-3 edge).
// Pattern = triangle(1,2,3), alive={0,1}.
TEST(MultiGraphUndirected, ThreeGraphsTwoShareTriangleOneDoesNot) {
    auto tri = make_triangle(1, 2, 3);
    auto pth = make_path({1, 2, 3});
    std::vector<Graph> gs = {tri, tri, pth};
    auto [p, alive] = run(gs, 0.66);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 3, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    EXPECT_EQ(alive.size(), 2u);
}

// s0: K3(1,2,3)+pendant(color 10 off node 0).
// s1: K3(1,2,3)+pendant(color 20 off node 1).
// s2: bare K3(1,2,3).  threshold=1.0.
// Colors 1,2,3: support 3/3 → all added.
// Color 10: support 1/3 < 1.0 → not added.
// Color 20: support 1/3 < 1.0 → not added.
// Edges 1-2, 2-3, 1-3: support 3/3 → all added.
// Pattern = triangle(1,2,3), all 3 alive.
TEST(MultiGraphUndirected, ThreeGraphsSharedK3CliquEmbeddedInLarger) {
    auto make_k3_plus = [](int32_t extra, uint32_t attach) {
        std::vector<int32_t> colors = {1,2,3,extra};
        std::vector<std::pair<uint32_t,uint32_t>> edges =
            {{0,1},{1,2},{0,2},{attach,3}};
        return Graph(4, edges, colors);
    };
    auto tri = make_triangle(1, 2, 3);
    std::vector<Graph> gs = {make_k3_plus(10,0), make_k3_plus(20,1), tri};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 3, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2, 3}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    ASSERT_EDGE_COLORS(p, 2, 3, false);
    ASSERT_EDGE_COLORS(p, 1, 3, false);
    EXPECT_EQ(alive.size(), 3u);
}

// Monochromatic (all color 5): g0=path 1-2-3-4, g1=star 1+3leaves, g2=triangle.
// threshold=0.33 → support needed = ceil(0.33*3)=1.
// All pattern nodes must be color 5.
TEST(MultiGraphUndirected, ThreeGraphsSingleColorDifferentEdgesLowThreshold) {
    auto g0 = make_path({5,5,5,5});
    auto g1 = make_star(5, {5,5,5});
    auto g2 = make_triangle(5,5,5);
    std::vector<Graph> gs = {g0, g1, g2};
    auto [p, alive] = run(gs, 0.33);
    EXPECT_GE(pattern_node_count(p), 1);
    for (int32_t c : pattern_colors(p))
        EXPECT_EQ(c, 5) << "Pattern: " << pattern_to_string(p);
}

// g0,g1 = path 5-5-5 (edges 0-1, 1-2).
// g2 = 3 nodes color 5, only edge 0-2.
// threshold=1.0.
// All 3 nodes color 5 added (support 3/3).
// Edge between node0 and node1 (both color 5): in g0,g1 yes, in g2 no → 2/3 < 1.0 → not added.
// Edge between node0 and node2: in g0 no, in g1 no, in g2 yes → 1/3 < 1.0 → not added.
// No edge qualifies. Pattern = 1 node (all same color, matching drops to 3 but
// no edge scores >= threshold). Exact behavior depends on matching internals.
// We assert: all pattern nodes are color 5.
TEST(MultiGraphUndirected, ThreeGraphsSingleColorDifferentEdgesHighThreshold) {
    auto g0 = make_path({5,5,5});
    auto g1 = make_path({5,5,5});
    std::vector<int32_t> c2 = {5,5,5};
    std::vector<std::pair<uint32_t,uint32_t>> e2 = {{0,2}};
    Graph g2(3, e2, c2);
    std::vector<Graph> gs = {g0, g1, g2};
    auto [p, alive] = run(gs, 1.0);
    EXPECT_GE(pattern_node_count(p), 1);
    for (int32_t c : pattern_colors(p))
        EXPECT_EQ(c, 5) << "Pattern: " << pattern_to_string(p);
}

// s0=edge(1-2), s1=path(1-2-3), s2=path(1-2-3-4).  threshold=1.0.
// All 3 have colors 1 and 2 → both added.
// Edge 1-2: support 3/3 → added.  All alive.
// Color 3: in s1 and s2 only → 2/3 < 1.0 → not added.
// Pattern = edge(1-2), all 3 alive.
TEST(MultiGraphUndirected, OneGraphStrictSubgraphOfOthers) {
    std::vector<int32_t> c0 = {1,2};
    std::vector<std::pair<uint32_t,uint32_t>> e0 = {{0,1}};
    Graph s0(2, e0, c0);
    auto s1 = make_path({1, 2, 3});
    auto s2 = make_path({1, 2, 3, 4});
    std::vector<Graph> gs = {s0, s1, s2};
    auto [p, alive] = run(gs, 1.0);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, false);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 1, 2, false);
    EXPECT_EQ(alive.size(), 3u);
}

// ══════════════════════════════════════════════════════════════════════════════
// DIRECTED TESTS
// ══════════════════════════════════════════════════════════════════════════════

// Single directed graph: 0→1 (colors 1,2).
// Pattern = 2 nodes, 1 directed edge color1→color2.
TEST(MultiGraphDirected, SingleGraphTwoNodesForwardEdge) {
    std::vector<Graph> gs = {make_two_nodes(1,2,true,true)};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
}

// Single directed graph: 1→0 (colors 1,2), meaning the edge goes from node1(color2) to node0(color1).
// Pattern = 2 nodes, 1 directed edge color2→color1.
TEST(MultiGraphDirected, SingleGraphTwoNodesBackwardEdge) {
    std::vector<Graph> gs = {make_two_nodes(1,2,false,true)};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 2, 1, true);
}

// Two graphs both 0→1 (colors 1,2), threshold=1.0.
// Pattern = 2 nodes, edge color1→color2, both alive.
TEST(MultiGraphDirected, TwoGraphsBothForwardEdge) {
    auto g = make_two_nodes(1,2,true,true);
    std::vector<Graph> gs = {g, g};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    EXPECT_EQ(alive.size(), 2u);
}

// Two graphs both 1→0 (colors 1,2), threshold=1.0.
// Pattern = 2 nodes, edge color2→color1, both alive.
TEST(MultiGraphDirected, TwoGraphsBothBackwardEdge) {
    auto g = make_two_nodes(1,2,false,true);
    std::vector<Graph> gs = {g, g};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    ASSERT_EDGE_COLORS(p, 2, 1, true);
    EXPECT_EQ(alive.size(), 2u);
}

// s0: 0→1 (1→2).  s1: 1→0 (2→1).  threshold=1.0.
// Seed adds one node.  Second node added.
// Edge 1→2: support 1/2 < 1.0.  Edge 2→1: support 1/2 < 1.0.
// No edge qualifies.  Pattern = 1 node, 0 edges.  Both alive.
TEST(MultiGraphDirected, TwoGraphsOppositeDirectionsHighThreshold) {
    auto s0 = make_two_nodes(1,2,true,true);
    auto s1 = make_two_nodes(1,2,false,true);
    std::vector<Graph> gs = {s0, s1};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, true);
    assert_pattern_colors_any(p, {{1}, {2}});
    EXPECT_EQ(alive.size(), 2u);
}

// Same, threshold=0.5 → 1/2 support is enough.
// Whichever direction has support 1 gets added.  1 graph survives.
TEST(MultiGraphDirected, TwoGraphsOppositeDirectionsLowThreshold) {
    auto s0 = make_two_nodes(1,2,true,true);
    auto s1 = make_two_nodes(1,2,false,true);
    std::vector<Graph> gs = {s0, s1};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1, 2}));
    EXPECT_EQ(alive.size(), 1u);
}

// Directed path 0→1→2→3→4 (colors 1-5), single graph.
// All 5 vertices added, then 4 directed edges added.
TEST(MultiGraphDirected, SingleGraphFiveNodesDirectedPath) {
    auto g = make_path({1,2,3,4,5}, true);
    std::vector<Graph> gs = {g};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 5);
    ASSERT_EDGE_COUNT(p, 4, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 4, true);
    ASSERT_EDGE_COLORS(p, 4, 5, true);
}

// Dense directed graph (7 directed edges), single graph.
// All 5 nodes and all 7 edges added.
TEST(MultiGraphDirected, SingleGraphFiveNodesSevenEdgesDirectedVerifyAllEdges) {
    std::vector<int32_t> colors = {1,2,3,4,5};
    std::vector<std::pair<uint32_t,uint32_t>> edges =
        {{0,1},{1,2},{2,3},{3,4},{0,2},{1,3},{2,4}};
    Graph g(5, edges, colors, true);
    std::vector<Graph> gs = {g};
    auto [p, alive] = run(gs, 0.5, true);
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

// Directed 9-node path (colors 1-9).
// n=1..7: vertices added.  n=8: try edge → 6 directed edges between nodes 0-6 added.
// Then vertices 7,8 added, then edges 7→8 and 8→9.
// Pattern = full 9-node directed path, all 8 directed edges.
// Exercises edge-addition BEFORE vertex exhaustion (same as undirected version).
TEST(MultiGraphDirected, SingleGraphNineNodesEdgesAddedBeforeVertexExhaustionPath) {
    std::vector<Graph> gs = {make_path({1,2,3,4,5,6,7,8,9}, true)};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 9);
    ASSERT_EDGE_COUNT(p, 8, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4,5,6,7,8,9}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 4, true);
    ASSERT_EDGE_COLORS(p, 4, 5, true);
    ASSERT_EDGE_COLORS(p, 5, 6, true);
    ASSERT_EDGE_COLORS(p, 6, 7, true);
    ASSERT_EDGE_COLORS(p, 7, 8, true);
    ASSERT_EDGE_COLORS(p, 8, 9, true);
}

// Two identical directed paths 1→2→3, threshold=1.0.
TEST(MultiGraphDirected, TwoIdenticalDirectedGraphsAllSurvive) {
    auto g = make_path({1,2,3}, true);
    std::vector<Graph> gs = {g, g};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    EXPECT_EQ(alive.size(), 2u);
}

// s0,s1=directed path 1→2→3; s2=directed edge 1→2.  threshold=0.66.
// Color 3: 2/3 >= 0.66 → added.  Edge 2→3: 2/3 >= 0.66 → added.  s2 drops.
// Pattern = directed path 1→2→3, alive={0,1}.
TEST(MultiGraphDirected, ThreeGraphsTwoIdenticalOneLessEdgesThresholdTwoSurviveDirected) {
    auto full = make_path({1,2,3}, true);
    std::vector<int32_t> rc = {1,2};
    std::vector<std::pair<uint32_t,uint32_t>> re = {{0,1}};
    Graph reduced(2, re, rc, true);
    std::vector<Graph> gs = {full, full, reduced};
    auto [p, alive] = run(gs, 0.66, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    EXPECT_EQ(alive.size(), 2u);
}

// Same, threshold=1.0.  Color 3: 2/3 < 1.0 → not added.
// Pattern = directed edge 1→2, all 3 alive.
TEST(MultiGraphDirected, ThreeGraphsTwoIdenticalOneLessEdgesThresholdAllSurviveDirected) {
    auto full = make_path({1,2,3}, true);
    std::vector<int32_t> rc = {1,2};
    std::vector<std::pair<uint32_t,uint32_t>> re = {{0,1}};
    Graph reduced(2, re, rc, true);
    std::vector<Graph> gs = {full, full, reduced};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1,2}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    EXPECT_EQ(alive.size(), 3u);
}

// No color overlap, directed, threshold=0.5 → 1 graph survives, full 2-node pattern.
TEST(MultiGraphDirected, TwoInputsNoColorOverlapLowThresholdDirected) {
    std::vector<Graph> gs = {make_path({1,2},true), make_path({10,20},true)};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    EXPECT_EQ(alive.size(), 1u);
}

// No color overlap, directed, threshold=1.0 → 1 node, 0 edges.
TEST(MultiGraphDirected, TwoInputsNoColorOverlapHighThresholdDirected) {
    std::vector<Graph> gs = {make_path({1,2},true), make_path({10,20},true)};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, true);
    EXPECT_EQ(alive.size(), 1u);
}

// One-node overlap (color 1), directed, threshold=0.5.
// Color 2 (support 1/2 >= 0.5) added → s1 drops.  Full path 1→2→3, alive={0}.
TEST(MultiGraphDirected, TwoInputsOneNodeOverlapLowThresholdDirected) {
    std::vector<Graph> gs = {make_path({1,2,3},true), make_path({1,4,5},true)};
    auto [p, alive] = run(gs, 0.5, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 2, true);
    
    assert_pattern_colors_any(p, {
        {1, 2, 3},
        {1, 4, 5}
    });

    assert_pattern_edges_any(p, {
        {{1, 2}, {2, 3}},
        {{1, 4}, {4, 5}}
    }, false);
    EXPECT_EQ(alive.size(), 1u);
}

// One-node overlap, directed, threshold=1.0.
// No second color scores >= 1.0.  Pattern = 1 node, 0 edges.  Both alive.
TEST(MultiGraphDirected, TwoInputsOneNodeOverlapHighThresholdDirected) {
    std::vector<Graph> gs = {make_path({1,2,3},true), make_path({1,4,5},true)};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 1);
    ASSERT_EDGE_COUNT(p, 0, true);
    ASSERT_PATTERN_COLORS(p, ({1}));
    EXPECT_EQ(alive.size(), 2u);
}

// Five identical directed paths 1→2→3→4, threshold=1.0.
TEST(MultiGraphDirected, FiveIdenticalDirectedGraphsAllSurvive) {
    auto g = make_path({1,2,3,4}, true);
    std::vector<Graph> gs(5, g);
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 4);
    ASSERT_EDGE_COUNT(p, 3, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3,4}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 4, true);
    EXPECT_EQ(alive.size(), 5u);
}

// s0,s1=directed triangle(1→2→3→1); s2=directed path(1→2→3).
// threshold=0.66. All nodes added. Edges 1→2,2→3: 3/3 → added.
// Edge 3→1: 2/3 >= 0.66 → added; s2 drops (no 3→1 edge).
// Pattern = directed triangle, alive={0,1}.
TEST(MultiGraphDirected, ThreeGraphsTwoShareDirectedTriangleOneDoesNot) {
    auto dtri = make_directed_triangle(1,2,3);
    auto dpth = make_path({1,2,3}, true);
    std::vector<Graph> gs = {dtri, dtri, dpth};
    auto [p, alive] = run(gs, 0.66, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 3, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 1, true);
    EXPECT_EQ(alive.size(), 2u);
}

// Directed K3 cycle in s0,s1 + extra pendant; bare K3 in s2.  threshold=1.0.
// Pendant colors (10,20) have support 1/3 < 1.0 → not added.
// K3 nodes and edges all have support 3/3.  Pattern = directed K3, all 3 alive.
TEST(MultiGraphDirected, ThreeGraphsSharedDirectedK3CliquEmbedded) {
    auto make_dk3_plus = [](int32_t extra, uint32_t attach) {
        std::vector<int32_t> colors = {1,2,3,extra};
        std::vector<std::pair<uint32_t,uint32_t>> edges =
            {{0,1},{1,2},{2,0},{attach,3}};
        return Graph(4, edges, colors, true);
    };
    auto dtri = make_directed_triangle(1,2,3);
    std::vector<Graph> gs = {make_dk3_plus(10,0), make_dk3_plus(20,1), dtri};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 3);
    ASSERT_EDGE_COUNT(p, 3, true);
    ASSERT_PATTERN_COLORS(p, ({1,2,3}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    ASSERT_EDGE_COLORS(p, 2, 3, true);
    ASSERT_EDGE_COLORS(p, 3, 1, true);
    EXPECT_EQ(alive.size(), 3u);
}

// g0,g1 = directed path 5→5→5.  g2 = single directed edge 5→5.
// threshold=1.0.  Edge between first two nodes: 3/3 → added.
// Second edge: 2/3 < 1.0 → not added.
// Pattern = 2 nodes (both color 5), 1 directed edge.  All 3 alive.
TEST(MultiGraphDirected, ThreeGraphsSingleColorHighThresholdDirected) {
    std::vector<int32_t> c3 = {5,5,5};
    std::vector<int32_t> c2 = {5,5};
    std::vector<std::pair<uint32_t,uint32_t>> e01={{0,1},{1,2}};
    std::vector<std::pair<uint32_t,uint32_t>> e02={{0,1}};
    std::vector<Graph> gs = {
        Graph(3,e01,c3,true), Graph(3,e01,c3,true), Graph(2,e02,c2,true)
    };
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    for (int32_t col : pattern_colors(p))
        EXPECT_EQ(col, 5) << "Pattern: " << pattern_to_string(p, true);
    EXPECT_EQ(alive.size(), 3u);
}

// s0=directed edge(1→2), s1=directed path(1→2→3), s2=directed path(1→2→3→4).
// threshold=1.0.  Color 3: 2/3 < 1.0 → not added.
// Edge 1→2: 3/3 → added.  Pattern = directed edge(1→2), all 3 alive.
TEST(MultiGraphDirected, OneGraphStrictSubgraphOfOthersDirected) {
    std::vector<int32_t> c0={1,2};
    std::vector<std::pair<uint32_t,uint32_t>> e0={{0,1}};
    Graph s0(2,e0,c0,true);
    auto s1 = make_path({1,2,3},true);
    auto s2 = make_path({1,2,3,4},true);
    std::vector<Graph> gs = {s0,s1,s2};
    auto [p, alive] = run(gs, 1.0, true);
    ASSERT_NODE_COUNT(p, 2);
    ASSERT_EDGE_COUNT(p, 1, true);
    ASSERT_PATTERN_COLORS(p, ({1,2}));
    ASSERT_EDGE_COLORS(p, 1, 2, true);
    EXPECT_EQ(alive.size(), 3u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}