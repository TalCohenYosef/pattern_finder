#include "GeneralColorHist.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <limits>


/**
 * @brief Constructor initializes the histogram matrix to zeros.
 */
GeneralColorHist::GeneralColorHist(int32_t num_colors): C(num_colors)
{
}

void GeneralColorHist::update_hist_increase_tree_count(
    const uint32_t pattern_depth,
    const uint32_t current_vertex_color)
{
    while(pattern_depth >= m_number_of_trees.size())
    {
        this->m_number_of_trees.push_back(std::vector<uint32_t>(C, 0));
    }
    ++m_number_of_trees[pattern_depth][current_vertex_color];
}

void GeneralColorHist::update_hist_decrease_tree_count(
    const uint32_t pattern_depth,
    const uint32_t current_vertex_color)
{
    --m_number_of_trees[pattern_depth][current_vertex_color];
}

std::pair<int32_t, int32_t>
GeneralColorHist::get_color_to_add(double threshold_percent)
{
    struct Candidate {
        int32_t color;
        int32_t node;
        double  weight;
    };

    std::vector<Candidate> candidates;

    if (m_number_of_trees.empty())
        return {-1, -1};

    const uint32_t num_trees = m_number_of_trees.size();

    /* ---------------------------
       1. Collect legal candidates
       --------------------------- */
    for (uint32_t c = 0; c < static_cast<uint32_t>(C); ++c) {
        for (uint32_t d = 0; d < m_number_of_trees.size(); ++d) {

            uint32_t support = m_number_of_trees[d][c];

            // B: percentage threshold
            const uint32_t min_support =
            static_cast<uint32_t>(
                std::ceil(threshold_percent * num_trees)
            );
            if (support < min_support)
                continue;

            candidates.push_back({
                static_cast<int32_t>(c),
                static_cast<int32_t>(d),
                static_cast<double>(support) // raw value for now
            });
        }
    }

    if (candidates.empty())
        return {-1, -1};

    /* ---------------------------
       2. Softmax over supports
       --------------------------- */
    std::vector<double> supports;
    supports.reserve(candidates.size());

    for (const auto& c : candidates)
        supports.push_back(c.weight);

    std::vector<double> probs = compute_softmax(
        std::vector<uint32_t>(supports.begin(), supports.end())
    );

    /* ---------------------------
       3. Sample using softmax
       --------------------------- */
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    double r = dist(rng);
    double acc = 0.0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        acc += probs[i];
        if (r <= acc) {
            return {candidates[i].color, candidates[i].node};
        }
    }

    // Numerical fallback
    return {
        candidates.back().color,
        candidates.back().node
    };
}


std::vector<double> GeneralColorHist::compute_softmax(const std::vector<uint32_t>& input) const
{
    std::vector<double> softmax_values(input.size());
    double max_val = *std::max_element(input.begin(), input.end());
    double sum_exp = 0.0;

    for (const auto& val : input) {
        sum_exp += std::exp(static_cast<double>(val) - max_val);
    }

    for (size_t i = 0; i < input.size(); ++i) {
        softmax_values[i] = std::exp(static_cast<double>(input[i]) - max_val) / sum_exp;
    }

    return softmax_values;
}