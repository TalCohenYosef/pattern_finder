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

double SingleGraphPatternFinder::score_state(PatternState& state, double background_density, bool is_direcred) const
{
    const uint32_t vertex_count = boost::num_vertices(state.pattern);
    const uint32_t edge_count   = is_direcred? boost::num_edges(state.pattern) : boost::num_edges(state.pattern) / 2;
    return PatternScorer::score(
        state.pattern_color_logp, edge_count, background_density, vertex_count);
}

/* ---------- expand_one_state ---------- */

void SingleGraphPatternFinder::expand_one_state(
    PatternState&          state,
    const CandidateVertex& cand,
    const Graph&           search_graph,
    bool                   is_directed) const
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
        {
            PatternUtils::add_edge(is_directed, state.pattern, new_pattern_node, i);
        }
    }

    state.hist->absorb_vertex(selected_vertex);
    state.match_path.push_back(selected_vertex);
}

/* ---------- clone_state ---------- */

PatternState SingleGraphPatternFinder::clone_state(const PatternState& src) const
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
std::vector<uint32_t> SingleGraphPatternFinder::select_seed_indices(
    uint32_t total_colors,
    uint32_t initial_count) const
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
std::vector<uint32_t> SingleGraphPatternFinder::allocate_seed_states(
    const std::vector<SeedInfo>& seeds,
    uint32_t                     target_count) const
{
    double total_weight = 0.0;
    for (const SeedInfo& s : seeds) total_weight += s.weight;

    std::vector<uint32_t> alloc(seeds.size());
    uint32_t allocated = 0;
    for (size_t i = 0; i < seeds.size(); ++i) {
        const double raw = static_cast<double>(target_count) * seeds[i].weight / total_weight;
        const uint32_t cap = static_cast<uint32_t>(seeds[i].matches.size());
        alloc[i] = std::max(1u, std::min(static_cast<uint32_t>(std::round(raw)), cap));
        allocated += alloc[i];
    }
    return alloc;
}

/**
 * Improved allocation that ensures target_count is met by redistributing
 * from colors with excess to those with insufficient matches.
 */
std::vector<uint32_t> SingleGraphPatternFinder::allocate_seed_states_improved(
    const std::vector<SeedInfo>& seeds,
    uint32_t                     target_count) const
{
    if (seeds.empty()) return {};

    // First, allocate proportionally like the original function
    double total_weight = 0.0;
    for (const SeedInfo& s : seeds) total_weight += s.weight;

    std::vector<uint32_t> alloc(seeds.size());
    uint32_t allocated = 0;
    std::vector<size_t> deficient_seeds;
    std::vector<size_t> excess_seeds;
    
    for (size_t i = 0; i < seeds.size(); ++i) {
        const double raw = static_cast<double>(target_count) * seeds[i].weight / total_weight;
        const uint32_t cap = static_cast<uint32_t>(seeds[i].matches.size());
        const uint32_t min_alloc = std::max(1u, std::min(static_cast<uint32_t>(std::round(raw)), cap));
        alloc[i] = min_alloc;
        allocated += min_alloc;
        
        // Track seeds that need more or have excess capacity
        if (alloc[i] < cap && allocated < target_count) {
            deficient_seeds.push_back(i);
        } else if (alloc[i] < cap) {
            excess_seeds.push_back(i);
        }
    }

    // If we haven't reached target_count, redistribute from excess capacity
    if (allocated < target_count && !excess_seeds.empty()) {
        uint32_t needed = target_count - allocated;
        
        // Sort deficient seeds by weight (rarer colors get priority)
        std::sort(deficient_seeds.begin(), deficient_seeds.end(),
                  [&](size_t a, size_t b) { return seeds[a].weight > seeds[b].weight; });
        
        for (size_t idx : deficient_seeds) {
            if (needed == 0) break;
            
            uint32_t cap = static_cast<uint32_t>(seeds[idx].matches.size());
            uint32_t can_add = cap - alloc[idx];
            uint32_t add = std::min(can_add, needed);
            
            alloc[idx] += add;
            needed -= add;
        }
    }

    return alloc;
}

/* ---------- select_valid_seeds ---------- */

std::vector<SeedInfo> SingleGraphPatternFinder::select_valid_seeds(
    const std::vector<std::tuple<double, uint32_t, uint32_t>>& valid_colors,
    const std::vector<std::vector<uint32_t>>& all_matches,
    uint32_t initial_count) const
{
    std::vector<SeedInfo> seeds;
    uint32_t total_matches = 0;
    
    // Start with rarest colors and add until we have enough matches
    for (const std::tuple<double, uint32_t, uint32_t>& color_tuple : valid_colors) {
        double prob = std::get<0>(color_tuple);
        uint32_t color_id = std::get<1>(color_tuple);
        uint32_t match_count = std::get<2>(color_tuple);
        
        seeds.push_back({color_id, prob, all_matches[color_id], 1.0 / prob});
        total_matches += match_count;
        
        // Stop if we have enough matches to fill the initial beam
        if (total_matches >= initial_count * 2) break;  // 2x to ensure good distribution
    }
    
    return seeds;  // Return whatever seeds we have, even if fewer than initial_count
}

/* ---------- create_beam_from_seeds ---------- */

std::vector<PatternState> SingleGraphPatternFinder::create_beam_from_seeds(
    const std::vector<SeedInfo>& seeds,
    const std::vector<uint32_t>& alloc,
    const Graph& search_graph,
    const std::vector<double>& color_probability,
    const std::vector<int32_t>& color_map,
    double log_bg_density,
    double alpha_0,
    double alpha_decay) const
{
    std::vector<PatternState> beam;
    for (size_t si = 0; si < seeds.size(); ++si) {
        std::cout << "Seed colour " << color_map[seeds[si].color_id]
                  << " (p=" << seeds[si].probability << ")  matches="
                  << seeds[si].matches.size() << "  keeping=" << alloc[si] << "\n";
        for (uint32_t mi = 0; mi < alloc[si]; ++mi)
            beam.push_back(create_initial_state(
                search_graph, color_probability, log_bg_density,
                alpha_0, alpha_decay, seeds[si].color_id, seeds[si].matches[mi]));
    }
    return beam;
}

/* ---------- create_initial_state ---------- */

PatternState SingleGraphPatternFinder::create_initial_state(
    const Graph&               search_graph,
    const std::vector<double>& color_probability,
    double                     log_bg_density,
    double                     alpha_0,
    double                     alpha_decay,
    uint32_t                   color_id,
    uint32_t                   match_vertex) const
{
    std::unique_ptr<SingleGraphHistogram> hist = std::make_unique<SingleGraphHistogram>(
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
uint32_t SingleGraphPatternFinder::find_gap_cut(
    const std::vector<std::pair<double, uint32_t>>& scored) const
{
    const uint32_t n = static_cast<uint32_t>(scored.size());
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

PatternState* SingleGraphPatternFinder::select_best_state(
    std::vector<PatternState>& beam,
    double                     background_density,
    bool                       is_directed) const
{
    PatternState* best = nullptr;
    double best_score = std::numeric_limits<double>::max();
    for (PatternState& state : beam) {
        if (state.alive_indexes.empty()) continue;
        const double s = score_state(state, background_density, is_directed);
        if (s < best_score) { best_score = s; best = &state; }
    }
    return best;
}

/* ---------- any_state_below_threshold ---------- */

bool SingleGraphPatternFinder::any_state_below_threshold(
    std::vector<PatternState>& beam,
    double bg_density, double threshold, uint32_t iteration, bool is_directed) const
{
    for (PatternState& state : beam) {
        if (state.alive_indexes.empty()) continue;
        const double s = score_state(state, bg_density, is_directed);
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

    // Get all matches for all colors in ONE pass (efficient!)
    std::vector<std::vector<uint32_t>> all_matches = PatternUtils::get_all_color_matches(search_graph, 
                                                                                          static_cast<uint32_t>(color_probability.size()));

    // Create valid colors list with their match counts
    std::vector<std::tuple<double, uint32_t, uint32_t>> valid_colors;
    for (uint32_t c = 0; c < static_cast<uint32_t>(color_probability.size()); ++c) {
        if (color_probability[c] > 0.0 && !all_matches[c].empty()) {
            valid_colors.emplace_back(color_probability[c], c, static_cast<uint32_t>(all_matches[c].size()));
        }
    }
    
    if (valid_colors.empty()) return {};

    // Sort by probability (ascending = rarest first)
    std::sort(valid_colors.begin(), valid_colors.end(),
              [](const std::tuple<double, uint32_t, uint32_t>& a, const std::tuple<double, uint32_t, uint32_t>& b) { 
                  return std::get<0>(a) < std::get<0>(b); 
              });

    // Select seeds with available matches (may be fewer than ideal)
    std::vector<SeedInfo> seeds = select_valid_seeds(valid_colors, all_matches, initial_count);
    if (seeds.empty()) return {};  // Only return empty if truly no valid colors found

    // Allocate states to meet target count
    std::vector<uint32_t> alloc = allocate_seed_states_improved(seeds, initial_count);

    return create_beam_from_seeds(seeds, alloc, search_graph, color_probability, 
                                 color_map, log_bg_density, m_alpha_0, m_alpha_decay);
}

/* ---------- expand_beam ---------- */

bool SingleGraphPatternFinder::expand_beam(
    std::vector<PatternState>& beam,
    const Graph&               search_graph,
    double                     background_density,
    bool                       is_directed) const
{
    bool any_expanded = false;
    const uint32_t current_size = static_cast<uint32_t>(beam.size());
    const uint32_t branching_factor = std::max(1u, m_max_active_patterns / std::max(1u, current_size));

    std::vector<PatternState> new_beam;
    new_beam.reserve(current_size * branching_factor);

    for (PatternState& state : beam) {
        if (state.alive_indexes.empty()) {
            new_beam.push_back(std::move(state));
            continue;
        }

        std::vector<CandidateVertex> candidates = state.hist->get_top_k_vertices(branching_factor);
        if (candidates.empty()) {
            new_beam.push_back(std::move(state));
            continue;
        }

        any_expanded = true;
        for (size_t ci = 0; ci + 1 < candidates.size(); ++ci) {
            PatternState cloned = clone_state(state);
            expand_one_state(cloned, candidates[ci], search_graph, is_directed);
            new_beam.push_back(std::move(cloned));
        }
        expand_one_state(state, candidates.back(), search_graph, is_directed);
        new_beam.push_back(std::move(state));
    }

    beam = std::move(new_beam);
    return any_expanded;
}

/* ---------- prune_beam ---------- */

void SingleGraphPatternFinder::prune_beam(
    std::vector<PatternState>& beam,
    double                     background_density,
    uint32_t                   iteration,
    bool                       is_directed) const
{
    if (beam.size() <= 1) return;

    std::vector<std::pair<double, uint32_t>> scored;
    scored.reserve(beam.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(beam.size()); ++i) {
        const double s = !beam[i].alive_indexes.empty()
            ? score_state(beam[i], background_density, is_directed)
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
    double  score_threshold,
    bool    is_directed)
{
    const std::chrono::high_resolution_clock::time_point time_start = std::chrono::high_resolution_clock::now();

    const std::vector<int32_t> color_map = PatternUtils::map_colors(search_graph, background_graph);

    const std::vector<double> color_probability = PatternUtils::compute_color_distribution(
        static_cast<uint32_t>(color_map.size()), background_graph);
    const double bg_density = PatternUtils::compute_density(
        background_graph.vertex_count(), background_graph.edge_count());

    std::vector<PatternState> beam = build_initial_beam(
        search_graph, color_probability, color_map, bg_density);
    if (beam.empty()) {
        std::cerr << "SingleGraphPatternFinder: no valid seed.\n";
        return {BoostGraph{}, {}};
    }

    // Main expansion loop
    uint32_t iteration = 0;
    const uint32_t MAX_ITERATIONS = 50;  // Safety limit
    bool threshold_reached = false;
    
    while (static_cast<uint32_t>(beam.size()) < m_max_active_patterns && iteration < MAX_ITERATIONS) {
        if (!expand_beam(beam, search_graph, bg_density, is_directed)) {
            std::cout << "No more expansions possible at iteration " << iteration << "\n";
            break;
        }
        
        // Check if any state reached the threshold
        for (PatternState& state : beam) {
            if (!state.alive_indexes.empty()) {
                double score = score_state(state, bg_density, is_directed);
                if (score <= score_threshold) {
                    threshold_reached = true;
                    std::cout << "Score " << score << " reached threshold " << score_threshold 
                              << " at iteration " << iteration << "\n";
                    break;
                }
            }
        }
        
        if (threshold_reached) break;
        ++iteration;
    }

    PatternState* best_state = select_best_state(beam, bg_density, is_directed);
    if (!best_state) {
        std::cerr << "SingleGraphPatternFinder: beam exhausted.\n";
        return {BoostGraph{}, {}};
    }
    
    if (!threshold_reached) {
        std::cout << "No pattern reached threshold, returning best pattern found (score: " 
                  << score_state(*best_state, bg_density,is_directed) << ")\n";
    }

    PatternUtils::recolor_pattern(best_state->pattern, color_map);
    const std::chrono::high_resolution_clock::time_point time_end = std::chrono::high_resolution_clock::now();
    std::cout << "Total pattern finding time: "
              << std::chrono::duration<double>(time_end - time_start).count()   
              << " seconds\n";

    return {std::move(best_state->pattern), std::move(best_state->alive_indexes)};
}
