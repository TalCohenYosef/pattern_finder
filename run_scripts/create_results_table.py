#!/usr/bin/env python3
"""
Script to create a summary table of pattern matching results.
Creates a table with S numbers (1-300) and 0/1 for each graph indicating if pattern was found.
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

def create_binary_vector(s_not_found, total_patterns=300):
    """Create binary vector where 1 = found in G, 0 = not found in G"""
    binary_vector = []
    for i in range(1, total_patterns + 1):
        if i in s_not_found:
            binary_vector.append(0)
        else:
            binary_vector.append(1)
    return binary_vector

def process_graph(results_dir, graph_name, total_patterns=300):
    """Process a single graph and return its results"""
    # Get pattern times
    summary_file = os.path.join(results_dir, 'PATTERN_SUMMARY.log')
    total_pattern_time = extract_pattern_times(summary_file)
    
    # Get subgraph time and S numbers
    results_file = os.path.join(results_dir, f'RESULTS_{graph_name}.log')
    subgraph_time = extract_subgraph_time(results_file)
    s_not_found = extract_s_numbers(results_file)
    
    # Create binary vector
    binary_vector = create_binary_vector(s_not_found, total_patterns)
    
    return {
        'graph_name': graph_name,
        'total_pattern_time': total_pattern_time,
        'subgraph_time': subgraph_time,
        'binary_vector': binary_vector,
        's_not_found': s_not_found
    }

def main():
    results_dir = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_avg_deg_3 '
    
    # Find all RESULTS_*.log files
    results_files = glob.glob(os.path.join(results_dir, 'RESULTS_*.log'))
    
    print(f"Looking in: {results_dir}")
    print(f"Found files: {results_files}")
    
    # Extract graph names
    graph_names = []
    for file in results_files:
        basename = os.path.basename(file)
        graph_name = basename.replace('RESULTS_', '').replace('.log', '')
        graph_names.append(graph_name)
    
    print("Found graphs:", graph_names)
    
    # Process each graph
    all_results = []
    for graph_name in graph_names:
        try:
            result = process_graph(results_dir, graph_name)
            all_results.append(result)
            print(f"Processed {graph_name}: {len(result['s_not_found'])} S not found")
        except Exception as e:
            print(f"Error processing {graph_name}: {e}")
    
    # Create CSV output
    output_file = '/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results_summary_table.csv'
    
    with open(output_file, 'w') as f:
        # Header
        header = ['Graph_Name', 'Total_Pattern_Time_sec', 'Subgraph_Time_sec'] + \
                [f'S_{i}' for i in range(1, 301)]
        f.write(','.join(header) + '\n')
        
        # Data rows
        for result in all_results:
            row = [
                result['graph_name'],
                str(result['total_pattern_time']),
                str(result['subgraph_time'])
            ] + [str(x) for x in result['binary_vector']]
            f.write(','.join(row) + '\n')
    
    print(f"\nSummary table saved to: {output_file}")
    print(f"Total graphs processed: {len(all_results)}")
    
    # Print summary statistics
    for result in all_results:
        found_count = sum(result['binary_vector'])
        print(f"{result['graph_name']}: {found_count}/300 patterns found in G")

if __name__ == "__main__":
    main()
