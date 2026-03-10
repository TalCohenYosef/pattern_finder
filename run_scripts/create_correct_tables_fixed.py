#!/usr/bin/env python3
"""
Script to create correct tables for each graph with pattern matching results.
Creates individual CSV files for each graph showing 1000 S numbers with 0/1 matches.
"""

import os
import re
import glob

def extract_pattern_times(summary_file):
    """Extract pattern generation times from PATTERN_SUMMARY.log"""
    with open(summary_file, 'r') as f:
        content = f.read()
    
    # Extract total pattern finding time
    total_pattern_pattern = r'Total pattern finding time: ([\d.]+) seconds'
    match = re.search(total_pattern_pattern, content)
    if match:
        total_time = float(match.group(1))
        return total_time
    
    return 0.0

def extract_subgraph_time(results_file):
    """Extract subgraph testing time from RESULTS_*.log file"""
    with open(results_file, 'r') as f:
        content = f.read()
    
    # Extract subgraph testing time
    time_pattern = r'Total subgraph testing time: ([\d.]+) seconds'
    match = re.search(time_pattern, content)
    if match:
        return float(match.group(1))
    return 0.0

def extract_s_numbers(results_file):
    """Extract S numbers that were NOT found in G from RESULTS_*.log file"""
    with open(results_file, 'r') as f:
        content = f.read()
    
    # Extract S numbers not in G
    s_pattern = r'S numbers not in G \(sorted\):\n((?:\d+\n)*)'
    match = re.search(s_pattern, content)
    
    if match:
        s_numbers = [int(x) for x in match.group(1).strip().split('\n') if x]
        return s_numbers
    return []

def create_graph_table(results_dir, graph_name, total_s=1000):
    """Create a separate table for one graph with exactly 1000 S numbers"""
    # Get pattern times
    summary_file = os.path.join(results_dir, 'PATTERN_SUMMARY.log')
    total_pattern_time = extract_pattern_times(summary_file)
    
    # Get subgraph time and S numbers
    results_file = os.path.join(results_dir, f'RESULTS_{graph_name}.log')
    subgraph_time = extract_subgraph_time(results_file)
    s_not_found = extract_s_numbers(results_file)
    
    # Create individual CSV file for this graph
    output_file = f'/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_avg_deg_3 /CSV/{graph_name}_results.csv'
    
    with open(output_file, 'w') as f:
        # Header
        header = ['S_Number', 'Found_in_G']
        f.write(','.join(header) + '\n')
        
        # Add timing info as comments
        f.write(f'# Pattern Generation Time: {total_pattern_time} seconds\n')
        f.write(f'# Subgraph Testing Time: {subgraph_time} seconds\n')
        f.write(f'# Total S not found: {len(s_not_found)}\n')
        f.write(f'# Total S found: {total_s - len(s_not_found)}\n')
        f.write('# Note: 1 = S in "not found" list (definitely not in G), 0 = S not in list (may be in G)\n')
        f.write('#\n')
        
        # Data rows - exactly 1000 S numbers
        for i in range(1, total_s + 1):
            if i in s_not_found:
                row = [str(i), '1']  # S definitely not found in G
            else:
                row = [str(i), '0']  # S may be found in G
            f.write(','.join(row) + '\n')
    
    return {
        'graph_name': graph_name,
        'total_pattern_time': total_pattern_time,
        'subgraph_time': subgraph_time,
        's_not_found_count': len(s_not_found),
        's_found_count': total_s - len(s_not_found),
        'output_file': output_file
    }

def main():
    results_dir = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_avg_deg_3 '
    folder_name = 'OUTPUT_input_avg_deg_3'
    
    # Find all RESULTS_*.log files
    results_files = glob.glob(os.path.join(results_dir, 'RESULTS_*.log'))
    
    print(f"Looking in: {results_dir}")
    
    # Extract graph names
    graph_names = []
    for file in results_files:
        basename = os.path.basename(file)
        graph_name = basename.replace('RESULTS_', '').replace('.log', '')
        graph_names.append(graph_name)
    
    print("Found graphs:", graph_names)
    
    # Process each graph and create separate table
    all_results = []
    for graph_name in graph_names:
        try:
            result = create_graph_table(results_dir, graph_name)
            all_results.append(result)
            print(f"Created table for {graph_name}: {result['s_found_count']}/1000 patterns found")
            print(f"  Saved to: {result['output_file']}")
        except Exception as e:
            print(f"Error processing {graph_name}: {e}")
    
    # Create summary file with folder name
    summary_file = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_avg_deg_3 /CSV/graphs_summary.txt'
    with open(summary_file, 'w') as f:
        f.write(f"Graph Processing Summary for {folder_name}\n")
        f.write("=" * 50 + "\n\n")
        
        for result in all_results:
            f.write(f"Graph: {result['graph_name']}\n")
            f.write(f"  Pattern Generation Time: {result['total_pattern_time']} seconds\n")
            f.write(f"  Subgraph Testing Time: {result['subgraph_time']} seconds\n")
            f.write(f"  S Found in G: {result['s_found_count']}/1000\n")
            f.write(f"  S Not Found in G: {result['s_not_found_count']}/1000\n")
            f.write(f"  Output File: {result['output_file']}\n")
            f.write("\n")
    
    print(f"\nSummary saved to: {summary_file}")
    print(f"Total graphs processed: {len(all_results)}")

if __name__ == "__main__":
    main()
