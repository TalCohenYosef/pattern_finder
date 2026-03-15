#include "SingleGraphPatternFinder.h"
#include "PatternUtils.h"
#include "PatternScorer.h"
#include "SingleGraphHistogram.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>

/* ---------- Named constants ---------- */

static constexpr uint32_t INITIAL_BEAM_DIVISOR      = 3;
static constexpr uint32_t MIN_SEED_COLORS           = 3;
static constexpr uint32_t STATES_PER_SEED           = 10;
static constexpr uint32_t PRUNE_WARMUP_ITERATIONS   = 5;
static constexpr double   MIN_KEEP_FRACTION          = 0.3;
static constexpr double   MIN_GAP_SCORE_RATIO        = 0.1;
static constexpr uint32_t MIN_STATES_FOR_GAP_PRUNE   = 3;

/* ---------- File-scope types ---------- */

struct SeedInfo {
    uint32_t              color_id;
    double                probability;
    std::vector<uint32_t> matches;
    double                weight;
};

/* ---------- Construction ---------- */

SingleGraphPatternFinder::SingleGraphPatternFinder(
    uint32_t max_active_patterns,
    double   alpha_0,
    double   alpha_decay)
    : m_max_active_patterns(max_active_patterns)
    , m_alpha_0(alpha_0)
    , m_alpha_decay(alpha_decay)
{
}

/* ---------- score_state ---------- */

static double score_state(PatternState& state, double background_density)
{
    const uint32_t vertex_count = boost::num_vertices(state.pattern);
    const uint32_t edge_count   = boost::num_edges(state.pattern);
    return PatternScorer::score(
        state.pattern_color_logp, edge_count, background_density, vertex_count);
}

/* ---------- expand_one_state ---------- */

static void expand_one_state(
    PatternState&          state,
    const CandidateVertex& cand,
    const Graph&           search_graph)
{
    const uint32_t selected_vertex = static_cast<uint32_t>(cand.s_vertex);
    const uint32_t vertex_color =
        static_cast<uint32_t>(search_graph.get_vertex_color(selected_vertex));

    const uint32_t new_pattern_node = boost::add_vertex(
        VertexProperty{static_cast<int32_t>(vertex_color)}, state.pattern);

    state.pattern_color_logp +=
        state.hist->log_prob_of_color(vertex_color);

    // Add all edges between the new vertex and existing match vertices.
    // Pattern is small, so iterating match_path is cheaper than scanning neighbours.
    for (uint32_t i = 0; i < state.match_path.size(); ++i) {
        if (search_graph.is_edge(selected_vertex, state.match_path[i]))
            boost::add_edge(new_pattern_node, i,
                            EdgeProperty{false}, state.pattern);
    }

    state.hist->absorb_vertex(selected_vertex);
    state.match_path.push_back(selected_vertex);
}

/* ---------- clone_state ---------- */

static PatternState clone_state(const PatternState& src)
{
    PatternState dst;
    dst.pattern            = src.pattern;
    dst.hist               = std::make_unique<SingleGraphHistogram>(*src.hist);
    dst.match_path         = src.match_path;
    dst.alive_indexes      = src.alive_indexes;
    dst.beam_score         = src.beam_score;
    dst.pattern_color_logp = src.pattern_color_logp;
    return dst;
}

/* ---------- select_seed_indices ---------- */

/**
 * Pick @p num_colors evenly-spaced indices into a sorted colour array of
 * size @p total_colors.  Index 0 (rarest) is always included.
 * Uses floating-point step with rounding to guarantee exactly @p num_colors
 * unique indices when total_colors >= num_colors.
 */
static std::vector<uint32_t> select_seed_indices(
    uint32_t total_colors,
    uint32_t initial_count)
{
    const uint32_t num_seeds =
        std::min(total_colors, std::max(MIN_SEED_COLORS, initial_count / STATES_PER_SEED));

    std::vector<uint32_t> indices;
    indices.reserve(num_seeds);
    indices.push_back(0);

    if (num_seeds > 1 && total_colors > 1) {
        const double step = static_cast<double>(total_colors - 1)
                          / static_cast<double>(num_seeds - 1);
        for (uint32_t si = 1; si < num_seeds; ++si)
            indices.push_back(static_cast<uint32_t>(std::round(si * step)));
    }

    return indices;
}

/* ---------- allocate_seed_states ---------- */

/**
 * Distribute @p target_count states across seeds proportional to
 * 1/probability (rarer colours get more).  Each seed gets at least 1,
 * capped by its available match count.
 */
static std::vector<uint32_t> allocate_seed_states(
    const std::vector<SeedInfo>& seeds,
    uint32_t                     target_count)
{
    double total_weight = 0.0;
    for (const auto& s : seeds) total_weight += s.weight;

    std::vector<uint32_t> alloc(seeds.size());
    uint32_t allocated = 0;
    for (size_t i = 0; i < seeds.size(); ++i) {
        const double raw = static_cast<double>(target_count) * seeds[i].weight / total_weight;
        const uint32_t cap = static_cast<uint32_t>(seeds[i].matches.size());
        alloc[i] = std::max(1u, std::min(static_cast<uint32_t>(std::round(raw)), cap));
        allocated += alloc[i];
    }

    while (allocated > target_count) {
        for (size_t i = seeds.size(); i-- > 0 && allocated > target_count; )
            if (alloc[i] > 1) { --alloc[i]; --allocated; }
    }
    while (allocated < target_count) {
        bool grew = false;
        for (size_t i = 0; i < seeds.size() && allocated < target_count; ++i) {
            if (alloc[i] < static_cast<uint32_t>(seeds[i].matches.size()))
                { ++alloc[i]; ++allocated; grew = true; }
        }
        if (!grew) break;
    }

    return alloc;
}

/* ---------- create_initial_state ---------- */

static PatternState create_initial_state(
    const Graph&               search_graph,
    const std::vector<double>& color_probability,
    double                     log_bg_density,
    double                     alpha_0,
    double                     alpha_decay,
    uint32_t                   color_id,
    uint32_t                   match_vertex)
{
    auto hist = std::make_unique<SingleGraphHistogram>(
        search_graph, color_probability, log_bg_density, alpha_0, alpha_decay);
    hist->absorb_vertex(match_vertex);

    BoostGraph pattern;
    boost::add_vertex(VertexProperty{static_cast<int32_t>(color_id)}, pattern);

    PatternState state;
    state.pattern            = std::move(pattern);
    state.hist               = std::move(hist);
    state.match_path         = {match_vertex};
    state.alive_indexes      = {0u};
    state.beam_score         = 0.0;
    state.pattern_color_logp = state.hist->log_prob_of_color(color_id);
    return state;
}

/* ---------- find_gap_cut ---------- */

/**
 * Given a sorted-ascending score vector, find a cut point at the largest
 * gap that is above the minimum-keep floor and statistically significant
 * relative to the total score range.
 * Returns scored.size() if no significant gap is found.
 */
static uint32_t find_gap_cut(
    const std::vector<std::pair<double, uint32_t>>& scored)
{
    const auto n = static_cast<uint32_t>(scored.size());
    if (n < MIN_STATES_FOR_GAP_PRUNE) return n;

    const uint32_t min_keep =
        std::max(MIN_STATES_FOR_GAP_PRUNE,
                 static_cast<uint32_t>(std::ceil(n * MIN_KEEP_FRACTION)));
    if (min_keep >= n) return n;

    const double score_range = scored.back().first - scored.front().first;
    if (score_range <= 0.0) return n;

    double max_gap = 0.0;
    uint32_t max_gap_pos = n;
    for (uint32_t i = min_keep; i < n; ++i) {
        const double gap = scored[i].first - scored[i - 1].first;
        if (gap > max_gap) { max_gap = gap; max_gap_pos = i; }
    }

    if (max_gap >= MIN_GAP_SCORE_RATIO * score_range)
        return max_gap_pos;

    return n;
}

/* ---------- select_best_state ---------- */

static PatternState* select_best_state(
    std::vector<PatternState>& beam,
    double                     background_density)
{
    PatternState* best = nullptr;
    double best_score = std::numeric_limits<double>::max();
    for (PatternState& state : beam) {
        if (state.alive_indexes.empty()) continue;
        const double s = score_state(state, background_density);
        if (s < best_score) { best_score = s; best = &state; }
    }
    return best;
}

/* ---------- any_state_below_threshold ---------- */

static bool any_state_below_threshold(
    std::vector<PatternState>& beam,
    double bg_density, double threshold, uint32_t iteration)
{
    for (auto& state : beam) {
        if (state.alive_indexes.empty()) continue;
        const double s = score_state(state, bg_density);
        if (s < threshold) {
            std::cout << "Score " << s << " < threshold " << threshold
                      << " at iteration " << iteration << " -- stopping.\n";
            return true;
        }
    }
    return false;
}

/* ---------- build_initial_beam ---------- */

std::vector<PatternState> SingleGraphPatternFinder::build_initial_beam(
    const Graph&                search_graph,
    const std::vector<double>&  color_probability,
    const std::vector<int32_t>& color_map,
    double                      background_density) const
{
    const double log_bg_density =
        (background_density > 0.0) ? std::log(background_density) : 0.0;
    const uint32_t initial_count = m_max_active_patterns / INITIAL_BEAM_DIVISOR;

    std::vector<std::pair<double, uint32_t>> all_colors;
    for (uint32_t c = 0; c < static_cast<uint32_t>(color_probability.size()); ++c)
        if (color_probability[c] > 0.0) all_colors.emplace_back(color_probability[c], c);
    std::sort(all_colors.begin(), all_colors.end());
    if (all_colors.empty()) return {};

    auto seed_indices = select_seed_indices(
        static_cast<uint32_t>(all_colors.size()), initial_count);

    std::vector<SeedInfo> seeds;
    for (uint32_t idx : seed_indices) {
        auto matches = PatternUtils::find_initial_matches(search_graph, all_colors[idx].second);
        if (!matches.empty())
            seeds.push_back({all_colors[idx].second, all_colors[idx].first,
                             std::move(matches), 1.0 / all_colors[idx].first});
    }
    if (seeds.empty()) return {};

    auto alloc = allocate_seed_states(seeds, initial_count);

    std::vector<PatternState> beam;
    for (size_t si = 0; si < seeds.size(); ++si) {
        std::cout << "Seed colour " << color_map[seeds[si].color_id]
                  << " (p=" << seeds[si].probability << ")  matches="
                  << seeds[si].matches.size() << "  keeping=" << alloc[si] << "\n";
        for (uint32_t mi = 0; mi < alloc[si]; ++mi)
            beam.push_back(create_initial_state(
                search_graph, color_probability, log_bg_density,
                m_alpha_0, m_alpha_decay, seeds[si].color_id, seeds[si].matches[mi]));
    }
    return beam;
}

/* ---------- expand_beam ---------- */

bool SingleGraphPatternFinder::expand_beam(
    std::vector<PatternState>& beam,
    const Graph&               search_graph,
    double                     background_density) const
{
    bool any_expanded = false;
    const uint32_t current_size = static_cast<uint32_t>(beam.size());
    const uint32_t branching_factor = std::max(1u, m_max_active_patterns / std::max(1u, current_size));

    std::vector<PatternState> new_beam;
    new_beam.reserve(current_size * branching_factor);

    for (auto& state : beam) {
        if (state.alive_indexes.empty()) {
            new_beam.push_back(std::move(state));
            continue;
        }

        auto candidates = state.hist->get_top_k_vertices(branching_factor);
        if (candidates.empty()) {
            state.alive_indexes.clear();
            new_beam.push_back(std::move(state));
            continue;
        }

        any_expanded = true;
        for (size_t ci = 0; ci + 1 < candidates.size(); ++ci) {
            PatternState cloned = clone_state(state);
            expand_one_state(cloned, candidates[ci], search_graph);
            new_beam.push_back(std::move(cloned));
        }
        expand_one_state(state, candidates.back(), search_graph);
        new_beam.push_back(std::move(state));
    }

    beam = std::move(new_beam);
    return any_expanded;
}

/* ---------- prune_beam ---------- */

void SingleGraphPatternFinder::prune_beam(
    std::vector<PatternState>& beam,
    double                     background_density,
    uint32_t                   iteration) const
{
    if (beam.size() <= 1) return;

    std::vector<std::pair<double, uint32_t>> scored;
    scored.reserve(beam.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(beam.size()); ++i) {
        const double s = !beam[i].alive_indexes.empty()
            ? score_state(beam[i], background_density)
            : std::numeric_limits<double>::max();
        scored.emplace_back(s, i);
    }
    std::sort(scored.begin(), scored.end());

    uint32_t keep_count = static_cast<uint32_t>(scored.size());
    if (iteration >= PRUNE_WARMUP_ITERATIONS)
        keep_count = std::min(keep_count, find_gap_cut(scored));
    keep_count = std::min(keep_count, m_max_active_patterns);

    if (keep_count >= static_cast<uint32_t>(beam.size())) return;

    std::vector<PatternState> kept;
    kept.reserve(keep_count);
    for (uint32_t i = 0; i < keep_count; ++i)
        kept.push_back(std::move(beam[scored[i].second]));
    beam = std::move(kept);
}

/* ---------- find_pattern ---------- */

std::pair<BoostGraph, std::unordered_set<uint32_t>>
SingleGraphPatternFinder::find_pattern(
    Graph&  search_graph,
    Graph&  background_graph,
    double  score_threshold)
{
    const auto time_start = std::chrono::high_resolution_clock::now();

    const auto color_map = PatternUtils::map_colors(search_graph, background_graph);

    const auto color_probability = PatternUtils::compute_color_distribution(
        static_cast<uint32_t>(color_map.size()), background_graph);
    const double bg_density = PatternUtils::compute_density(
        background_graph.vertex_count(), background_graph.edge_count());

    auto beam = build_initial_beam(
        search_graph, color_probability, color_map, bg_density);
    if (beam.empty()) {
        std::cerr << "SingleGraphPatternFinder: no valid seed.\n";
        return {BoostGraph{}, {}};
    }
    std::cout << "Initial beam size: " << beam.size() << "\n";

    uint32_t iteration = 0;
    while (true) {
        if (any_state_below_threshold(beam, bg_density, score_threshold, iteration))
            break;
        if (!expand_beam(beam, search_graph, bg_density)) break;
        prune_beam(beam, bg_density, iteration);
        std::cout << "Iteration " << iteration << "  beam_size=" << beam.size() << "\n";
        ++iteration;
    }

    auto* best_state = select_best_state(beam, bg_density);
    if (!best_state) {
        std::cerr << "SingleGraphPatternFinder: beam exhausted.\n";
        return {BoostGraph{}, {}};
    }

    PatternUtils::recolor_pattern(best_state->pattern, color_map);
    const auto time_end = std::chrono::high_resolution_clock::now();
    std::cout << "Total pattern finding time: "
              << std::chrono::duration<double>(time_end - time_start).count()
              << " seconds\n";

    return {std::move(best_state->pattern), std::move(best_state->alive_indexes)};
}
