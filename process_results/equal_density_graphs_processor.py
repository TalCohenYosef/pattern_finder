#!/usr/bin/env python3
"""
Script to process G_induced and G_non_induced graphs.
Handles all OUTPUT_input_color_{dist}_deg_k folders and compares only with relevant motif logs.
"""

import os
import re
import glob
from datetime import datetime

PATTERN_FINDER_RESULTS_DIR = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results'

def extract_pattern_finder_results(dist, k, graph_name):
    """Extract S numbers not found in G from pattern_finder results"""
    results_file = os.path.join(PATTERN_FINDER_RESULTS_DIR, f'OUTPUT_input_color_{dist}_deg_{k}', f'RESULTS_G_{graph_name}.log')
    
    if os.path.exists(results_file):
        with open(results_file, 'r') as f:
            content = f.read()
        
        # Extract S numbers not in G
        s_pattern = r'S numbers not in G \(sorted\):\n((?:\d+\n)*)'
        match = re.search(s_pattern, content)
        
        if match:
            s_not_found = [int(x) for x in match.group(1).strip().split('\n') if x]
            return s_not_found
    
    return []

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

def create_comparison_csv(dist, k, graph_name):
    """Create comparison CSV for a special graph with only 3 columns"""
    
    # Get pattern_finder results
    pf_not_found = extract_pattern_finder_results(dist, k, graph_name)
    
    # Get motif log paths
    log = os.path.join(f"/home/cohent59/new-graph-measures/local_tests/{graph_name}/logs/compare_results", f'{graph_name}_color_{dist}_deg_{k}.log')
    
    # Get motif results
    results = extract_sum_motif_results(log)
    
    # Create CSV
    output_dir = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/process_results/CSV'
    os.makedirs(output_dir, exist_ok=True)
    
    # Create clear filename that doesn't override
    output_file = os.path.join(output_dir, f'{graph_name}_{dist}_deg_{k}_.csv')
    
    with open(output_file, 'w') as f:
        # Header - only 3 columns for special graphs
        if graph_name == "induced":
            f.write('S_index,pattern_finder,induced_motifs\n')
        else:
            f.write('S_index,pattern_finder,non_induced_motifs\n')
        
        # Data rows for S_1 to S_1000
        for i in range(1, 1001):
            # pattern_finder: 1 if not found, 0 if found
            pf_result = '1' if i in pf_not_found else '0'
            
            # induced_motifs: 1 if FAIL, 0 if PASS
            motif_result = '1' if results.get(i) == 'FAIL' else '0'
            
            row = f'{i},{pf_result},{motif_result}'
            f.write(row + '\n')
    
    return {
        'graph_name': graph_name,
        'output_file': output_file,
        'pf_not_found': len(pf_not_found),
        'motifs_fail': sum(1 for v in results.values() if v == 'FAIL')
        }

def main():
    # Get all special graphs to process
    all_results = []
    
    print(f"Processing special graphs...")
    print("=" * 50)
    
    # Process each combination
    for dist in ['uniform', 'average', 'rare']:
        for k in [3, 5, 8, 15]:
            for graph_name_inducion in ['induced', 'non_induced']:
                print(f"Processing: {dist}_deg_{k}_{graph_name_inducion}")
                
                result = create_comparison_csv(dist, k, graph_name_inducion)
                all_results.append(result)
                
                print(f"  Pattern finder not found: {result['pf_not_found']}")
                print(f"  motifs FAIL: {result['motifs_fail']}")
                print(f"  Saved to: {result['output_file']}")
                print()
    
    # Create summary
    summary_file = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/process_results/special_graphs_summary.txt'
    with open(summary_file, 'w') as f:
        f.write("Special Graphs Algorithm Comparison Summary\n")
        f.write("=" * 50 + "\n\n")
        
        for result in all_results:
            f.write(f"Graph: {result['graph_name']}\n")
            f.write(f"  Pattern finder not found: {result['pf_not_found']}/1000\n")
            f.write(f"  motifs FAIL: {result['motifs_fail']}/1000\n")
            f.write(f"  Output: {result['output_file']}\n")
            f.write("\n")
    
    print(f"Summary saved to: {summary_file}")
    print(f"Total graphs processed: {len(all_results)}")

if __name__ == "__main__":
    main()
