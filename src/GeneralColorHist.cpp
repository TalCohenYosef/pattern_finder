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
GeneralColorHist::get_color_to_add(uint32_t threshold)
{
    struct Candidate {
        int32_t color;
        int32_t node;
        double  weight;
    };

    std::vector<Candidate> candidates;
    double total_weight = 0.0;

    // 1. Collect all legal candidates
    for (uint32_t c = 0; c < static_cast<uint32_t>(C); ++c) {
        for (uint32_t d = 0; d < m_number_of_trees.size(); ++d) {

            uint32_t support = m_number_of_trees[d][c];
            
            // Debug output for large graphs
            //std::cout << "Color " << c << ", Depth " << d << ", Support: " << support << ", Threshold: " << threshold << std::endl;
            
            if (support <= threshold)
                continue;

            // linear weight (you can change this later)
            double w = static_cast<double>(support);

            candidates.push_back({
                static_cast<int32_t>(c),
                static_cast<int32_t>(d),
                w
            });

            total_weight += w;
        }
    }

    // no legal extension
    if (candidates.empty()) {
        //std::cout <<"No valid candidates found" ", colors=" << C << ", depths=" << m_number_of_trees.size() << std::endl;
        return {-1, -1};
    }

    // 2. Sample
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(0.0, total_weight);

    double r = dist(rng);
    double acc = 0.0;

    for (const auto& cand : candidates) {
        acc += cand.weight;
        if (r <= acc) {
            return {cand.color, cand.node};
        }
    }

    // fallback (numerical safety)
    return {candidates.back().color, candidates.back().node};
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