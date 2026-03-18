#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import re

def parse_matches_file(filename):
    """Parse the all_matches.txt file and extract match counts."""
    matches = []
    zero_patterns = []
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('P_') and 'matches' in line:
                # Extract pattern number and match count
                match = re.search(r'P_(\d+)\.json: (\d+) matches', line)
                if match:
                    pattern_num = int(match.group(1))
                    match_count = int(match.group(2))
                    matches.append((pattern_num, match_count))
                    if match_count == 0:
                        zero_patterns.append(pattern_num)
            elif line == 'SUMMARY':
                break  # Stop at summary section
    
    # Sort by pattern number
    matches.sort()
    return matches, zero_patterns

def parse_summary_time(summary_file):
    """Parse the summary file to extract timing information."""
    with open(summary_file, 'r') as f:
        content = f.read()
        
    # Extract total processing time
    processing_match = re.search(r'Total processing time: (\d+)s', content)
    if processing_match:
        processing_time = int(processing_match.group(1))
    else:
        processing_time = 0
        
    return processing_time

def parse_search_time(matches_file):
    """Parse the all_matches.txt file to extract search time."""
    with open(matches_file, 'r') as f:
        content = f.read()
        
    # Extract total time from summary section
    search_match = re.search(r'Total time: (\d+)s', content)
    if search_match:
        search_time = int(search_match.group(1))
    else:
        search_time = 0
        
    return search_time

def create_focused_plots():
    """Create focused plots for zero match rates and timing comparison."""
    
    # File paths
    datasets = [
        {
            'name': 'Rare (-100)',
            'matches_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G_15_RARE_score_m_100/all_matches.txt',
            'pattern_time': 1995,  # Special case from user
            'color': 'red'
        },
        {
            'name': 'Uniform (-100)', 
            'matches_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G_15_uniform_score_m100/all_matches.txt',
            'summary_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G_15_uniform_score_m100/summary.txt',
            'color': 'blue'
        },
        {
            'name': 'Uniform (-150)',
            'matches_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G_15_uniform_m150/all_matches.txt', 
            'summary_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G_15_uniform_m150/summary.txt',
            'color': 'orange'
        },
        {
            'name': 'Rare (-150)',
            'matches_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G15RARE_score_m150/all_matches.txt', 
            'summary_file': '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/OUTPUT_G15RARE_score_m150/summary.txt',
            'color': 'green'
        }
    ]
    
    # Parse data
    zero_rates = []
    processing_times = []
    search_times = []
    names = []
    colors = []
    
    for dataset in datasets:
        # Parse matches
        matches, zeros = parse_matches_file(dataset['matches_file'])
        zero_rate = len(zeros) / len(matches) * 100
        zero_rates.append(zero_rate)
        
        # Parse times
        if 'pattern_time' in dataset:
            proc_time = dataset['pattern_time']
        elif dataset['summary_file']:
            proc_time = parse_summary_time(dataset['summary_file'])
        else:
            proc_time = 0
        processing_times.append(proc_time)
        
        search_time = parse_search_time(dataset['matches_file'])
        search_times.append(search_time)
        
        names.append(dataset['name'])
        colors.append(dataset['color'])
    
    # Create figure with 2 subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # 1. Zero Match Rate Comparison
    ax1.set_title('Zero Match Rate Comparison', pad=20)
    bars1 = ax1.bar(names, zero_rates, color=colors, alpha=0.7)
    ax1.set_ylabel('Zero Match Rate (%)')
    ax1.set_ylim(0, 100)
    
    # Add percentage labels on bars
    for bar, rate in zip(bars1, zero_rates):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1, 
                f'{rate:.1f}%', ha='center', va='bottom', fontweight='bold')
    
    # Add grid
    ax1.grid(axis='y', alpha=0.3)
    
    # 2. Timing Comparison
    x = np.arange(len(names))
    width = 0.25
    
    # Calculate total times
    total_times = [p + s for p, s in zip(processing_times, search_times)]
    
    # Use same color for each category across all datasets
    pattern_colors = ['steelblue'] * 4  # Same color for all Pattern_finder_one_graph
    velcro_colors = ['orange'] * 4      # Same color for all Velcro
    total_colors = ['green'] * 4        # Same color for all Total
    
    bars2 = ax2.bar(x - width, processing_times, width, label='Pattern_finder_one_graph', alpha=0.8)
    bars3 = ax2.bar(x, search_times, width, label='Velcro', alpha=0.8)
    bars4 = ax2.bar(x + width, total_times, width, label='Total', alpha=0.8)
    
    # Color the bars with 3 different colors per category
    for i, (bar2, bar3, bar4) in enumerate(zip(bars2, bars3, bars4)):
        bar2.set_color(pattern_colors[i])
        bar3.set_color(velcro_colors[i])
        bar4.set_color(total_colors[i])
    
    ax2.set_ylabel('Time (seconds)')
    ax2.set_title('Processing Time Comparison')
    ax2.set_xticks(x)
    ax2.set_xticklabels(names)
    ax2.legend()
    
    # Add time labels on bars
    for bar, time in zip(bars2, processing_times):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 50, 
                f'{time}s', ha='center', va='bottom', fontweight='bold')
    
    for bar, time in zip(bars3, search_times):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 50, 
                f'{time}s', ha='center', va='bottom', fontweight='bold')
    
    for bar, time in zip(bars4, total_times):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 50, 
                f'{time}s', ha='center', va='bottom', fontweight='bold')
    
    # Add grid
    ax2.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/focused_analysis.png', 
                dpi=300, bbox_inches='tight')
    plt.show()
    
    # Print summary
    print("=" * 60)
    print("FOCUSED ANALYSIS SUMMARY")
    print("=" * 60)
    for i, name in enumerate(names):
        print(f"{name}:")
        print(f"  Zero match rate: {zero_rates[i]:.1f}%")
        print(f"  Pattern building time: {processing_times[i]}s")
        print(f"  Graph search time: {search_times[i]}s")
        print(f"  Total time: {processing_times[i] + search_times[i]}s")
        print()
    print("=" * 60)
    print("Visualization saved to: focused_analysis.png")

if __name__ == "__main__":
    create_focused_plots()
