#!/usr/bin/env python3
"""
Script to create comparison CSV tables with results from 3 different algorithms:
1. pattern_finder (our current results)
2. induced_motifs (from new-graph-measures/induced)
3. non_induced_motifs (from new-graph-measures/non_induced)
"""

import os
import re
import glob
from datetime import datetime

def extract_pattern_finder_results(results_dir, graph_name):
    """Extract S numbers not found in G from pattern_finder results"""
    results_file = os.path.join(results_dir, f'RESULTS_{graph_name}.log')
    s_not_found = []
    
    if os.path.exists(results_file):
        with open(results_file, 'r') as f:
            content = f.read()
        
        # Extract S numbers not in G
        s_pattern = r'S numbers not in G \(sorted\):\n((?:\d+\n)*)'
        match = re.search(s_pattern, content)
        
        if match:
            s_not_found = [int(x) for x in match.group(1).strip().split('\n') if x]
    
    return s_not_found

def extract_motif_results(log_file):
    """Extract PASS/FAIL results from motif log files"""
    results = {}
    
    if os.path.exists(log_file):
        with open(log_file, 'r') as f:
            lines = f.readlines()
        
        # Process first 1000 lines that contain S{i} PASS/FAIL
        s_count = 0
        for line in lines:
            if s_count >= 1000:
                break
                
            # Look for S{i} PASS or S{i} FAIL pattern
            match = re.search(r'S_(\d+)\s+(PASS|FAIL)', line)
            if match:
                s_num = int(match.group(1))
                status = match.group(2)
                results[s_num] = status
                s_count += 1
    
    return results

def extract_sum_motif_results(log_file):
    """Extract SUM PASS/SUM FAIL results from sum motif log files"""
    results = {}
    
    if os.path.exists(log_file):
        with open(log_file, 'r') as f:
            lines = f.readlines()
        
        # Process first 1000 lines that contain SUM PASS/FAIL
        s_count = 0
        for line in lines:
            if s_count >= 1000:
                break
                
            # Look for SUM PASS S_{i} or SUM FAIL S_{i} pattern
            match = re.search(r'SUM\s+(PASS|FAIL)\s+S_(\d+)', line)
            if match:
                s_num = int(match.group(2))
                status = match.group(1)
                results[s_num] = status
                s_count += 1
    
    return results

def get_graph_list():
    """Get list of all graph files to process"""
    graphs = []
    
    # g_den graphs
    for k in [3, 5]:
        if k == 3:
            i_values = [5, 8, 10, 13, 15]
        else:
            i_values = [8, 10, 13, 15]
        
        for i in i_values:
            for j in ['uniform', 'average', 'rare']:
                graph_name = f'g_den_{i}_embedded_den_{k}_{j}_0'
                graphs.append(graph_name)
    
    # Special graphs
    graphs.extend(['G_induced', 'G_non_induced'])
    
    return graphs

def get_motif_log_paths(graph_name):
    """Get the correct log file paths for induced and non_induced motifs"""
    
    if graph_name in ['G_induced', 'G_non_induced']:
        # Special case for G_induced and G_non_induced
        induced_log = f'/home/cohent59/new-graph-measures/local_tests/induced/logs/compare_results/induced_color_rare_deg_3.log'
        non_induced_log = f'/home/cohent59/new-graph-measures/local_tests/non_induced/logs/compare_results/non_induced_color_rare_deg_3.log'
    else:
        # Extract parameters from graph name
        match = re.match(r'g_den_(\d+)_embedded_den_(\d+)_(\w+)_0', graph_name)
        if match:
            i = match.group(1)
            k = match.group(2)
            j = match.group(3)
            
            induced_log = f'/home/cohent59/new-graph-measures/local_tests/induced/logs/compare_results/g_den_{i}_embedded_den_{k}_{j}_0.log'
            non_induced_log = f'/home/cohent59/new-graph-measures/local_tests/non_induced/logs/compare_results/g_den_{i}_embedded_den_{k}_{j}_0.log'
        else:
            induced_log = None
            non_induced_log = None
    
    return induced_log, non_induced_log

def create_comparison_csv(graph_name, pattern_finder_dir):
    """Create comparison CSV for a single graph"""
    
    # Get pattern_finder results
    pf_not_found = extract_pattern_finder_results(pattern_finder_dir, graph_name)
    
    # Get motif log paths
    induced_log, non_induced_log = get_motif_log_paths(graph_name)
    
    # Get motif results
    if graph_name in ['G_induced', 'G_non_induced']:
        induced_results = extract_sum_motif_results(induced_log) if induced_log else {}
        non_induced_results = extract_sum_motif_results(non_induced_log) if non_induced_log else {}
    else:
        induced_results = extract_motif_results(induced_log) if induced_log else {}
        non_induced_results = extract_motif_results(non_induced_log) if non_induced_log else {}
    
    # Create CSV
    output_dir = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_average_deg_3 /CSV'
    os.makedirs(output_dir, exist_ok=True)
    
    output_file = os.path.join(output_dir, f'{graph_name}_comparison.csv')
    
    with open(output_file, 'w') as f:
        # Header
        f.write('S_index,pattern_finder,induced_motifs,non_induced_motifs\n')
        
        # Data rows for S_1 to S_1000
        for i in range(1, 1001):
            # pattern_finder: 1 if not found, 0 if found
            pf_result = '1' if i in pf_not_found else '0'
            
            # induced_motifs: 1 if FAIL, 0 if PASS
            induced_result = '1' if induced_results.get(i) == 'FAIL' else '0'
            
            # non_induced_motifs: 1 if FAIL, 0 if PASS
            non_induced_result = '1' if non_induced_results.get(i) == 'FAIL' else '0'
            
            row = f'{i},{pf_result},{induced_result},{non_induced_result}'
            f.write(row + '\n')
    
    return {
        'graph_name': graph_name,
        'output_file': output_file,
        'pf_not_found': len(pf_not_found),
        'induced_fail': sum(1 for v in induced_results.values() if v == 'FAIL'),
        'non_induced_fail': sum(1 for v in non_induced_results.values() if v == 'FAIL')
    }

def main():
    pattern_finder_dir = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_average_deg_3'
    
    # Get all graphs to process
    graphs = get_graph_list()
    
    print(f"Processing {len(graphs)} graphs...")
    print("=" * 50)
    
    all_results = []
    
    for graph_name in graphs:
        print(f"Processing: {graph_name}")
        
        try:
            result = create_comparison_csv(graph_name, pattern_finder_dir)
            all_results.append(result)
            
            print(f"  Pattern finder not found: {result['pf_not_found']}")
            print(f"  Induced FAIL: {result['induced_fail']}")
            print(f"  Non-induced FAIL: {result['non_induced_fail']}")
            print(f"  Saved to: {result['output_file']}")
            print()
            
        except Exception as e:
            print(f"  Error: {e}")
            print()
    
    # Create summary
    summary_file = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_average_deg_3 /CSV/comparison_summary.txt'
    with open(summary_file, 'w') as f:
        f.write("Algorithm Comparison Summary\n")
        f.write("=" * 50 + "\n\n")
        
        for result in all_results:
            f.write(f"Graph: {result['graph_name']}\n")
            f.write(f"  Pattern finder not found: {result['pf_not_found']}/1000\n")
            f.write(f"  Induced FAIL: {result['induced_fail']}/1000\n")
            f.write(f"  Non-induced FAIL: {result['non_induced_fail']}/1000\n")
            f.write(f"  Output: {result['output_file']}\n")
            f.write("\n")
    
    print(f"Summary saved to: {summary_file}")
    print(f"Total graphs processed: {len(all_results)}")

if __name__ == "__main__":
    main()
