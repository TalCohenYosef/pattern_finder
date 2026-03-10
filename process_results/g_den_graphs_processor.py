#!/usr/bin/env python3
"""
Script to process g_den graphs with density 3 and 5.
Handles both OUTPUT_input_color_{dist}_deg_3 and OUTPUT_input_color_{dist}_deg_5 folders.
"""

import os
import re
import glob
from datetime import datetime

PATTERN_FINDER_RESULTS_DIR = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results'
MOTIFS_INDUCED_RESULTS_DIR = "/home/cohent59/new-graph-measures/local_tests/induced/logs/compare_results"
MOTIFS_NON_INDUCED_RESULTS_DIR = "/home/cohent59/new-graph-measures/local_tests/non_induced/logs/compare_results"

def extract_pattern_finder_results(graph_name_params, graph_name):
    """Extract S numbers not found in G from pattern_finder results"""
    # Try both density 3 and 5 folders
    log_file = os.path.join(PATTERN_FINDER_RESULTS_DIR, f"OUTPUT_input_color_{graph_name_params[2]}_deg_{graph_name_params[1]}", f"RESULTS_{graph_name}.log")
    if os.path.exists(log_file):
        with open(log_file, 'r') as f:
            content = f.read()
        
        # Extract S numbers not in G
        s_pattern = r'S numbers not in G \(sorted\):\n((?:\d+\n)*)'
        match = re.search(s_pattern, content)
        
        if match:
            s_not_found = [int(x) for x in match.group(1).strip().split('\n') if x]
            return s_not_found
    
    raise FileNotFoundError(f"Pattern finder results not found for {log_file}")

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

def get_g_den_graphs():
    """Get list of all g_den graphs to process"""
    graphs = []
    
    # g_den graphs with density 3
    for i in [5, 8, 10, 13, 15]:
        for j in ['uniform', 'average', 'rare']:
            graphs.append((i, 3, j))
    
    # g_den graphs with density 5
    for i in [8, 10, 13, 15]:
        for j in ['uniform', 'average', 'rare']:
            graphs.append((i, 5, j))
    
    return graphs


def create_comparison_csv(graph_name_params):
    """Create comparison CSV for a single g_den graph"""
    i, k, j = graph_name_params
    graph_name = f'g_den_{i}_embedded_den_{k}_{j}_0'
    # Get pattern_finder results
    pf_not_found = extract_pattern_finder_results(graph_name_params, graph_name)
    
    # Get motif log paths
    induced_log= os.path.join(MOTIFS_INDUCED_RESULTS_DIR, f"g_den_{i}_embedded_den_{k}_{j}_0.log")
    non_induced_log = os.path.join(MOTIFS_NON_INDUCED_RESULTS_DIR, f"g_den_{i}_embedded_den_{k}_{j}_0.log")
    
    # Get motif results
    induced_results = extract_motif_results(induced_log) if induced_log else {}
    non_induced_results = extract_motif_results(non_induced_log) if non_induced_log else {}
    
    # Create CSV
    output_dir = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/process_results/CSV'
    os.makedirs(output_dir, exist_ok=True)
    
    output_file = os.path.join(output_dir, f'{graph_name}.csv')
    
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
    # Get all g_den graphs to process
    graphs = get_g_den_graphs()
    
    print(f"Processing {len(graphs)} g_den graphs...")
    print("=" * 50)
    
    all_results = []
    
    for graph_name in graphs:
        print(f"Processing: {graph_name}")
        
        result = create_comparison_csv(graph_name)
        all_results.append(result)
        
        print(f"  Pattern finder not found: {result['pf_not_found']}")
        print(f"  Induced FAIL: {result['induced_fail']}")
        print(f"  Non-induced FAIL: {result['non_induced_fail']}")
        print(f"  Saved to: {result['output_file']}")
        print()
            
    
    # Create summary
    summary_file = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/process_results/g_den_comparison_summary.txt'
    with open(summary_file, 'w') as f:
        f.write("G_Den Graphs Algorithm Comparison Summary\n")
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
