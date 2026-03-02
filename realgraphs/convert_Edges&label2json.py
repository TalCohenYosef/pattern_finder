#!/usr/bin/env python3
"""
Convert real-world graph data from edge list and node labels format to JSON format.

Input format:
- G.edges: edge list (source target)
- G.node_labels: node labels (node_id label)

Output format:
- JSON files with "nodes" and "links" arrays compatible with pattern_finder
"""

import json
import os
from collections import defaultdict

def read_graph_data(edges_file, labels_file):
    """Read graph data from edge and label files."""
    # Read node labels
    node_labels = {}
    with open(labels_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                node_id = int(parts[0])
                label = int(parts[1])
                node_labels[node_id] = label
    
    # Read edges
    edges = []
    max_node_id = max(node_labels.keys()) if node_labels else -1
    
    with open(edges_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                source = int(parts[0])
                target = int(parts[1])
                edges.append((source, target))
                max_node_id = max(max_node_id, source, target)
    
    # Ensure all nodes in edges have labels (default to 0 if not specified)
    for node_id in range(max_node_id + 1):
        if node_id not in node_labels:
            node_labels[node_id] = 0
    
    return node_labels, edges

def convert_to_json(node_labels, edges):
    """Convert to JSON format."""
    # Create nodes array
    nodes = []
    for node_id, color in sorted(node_labels.items()):
        nodes.append({
            "id": node_id,
            "color": color
        })
    
    # Create links array
    links = []
    for source, target in edges:
        links.append({
            "source": source,
            "target": target
        })
    
    return {
        "nodes": nodes,
        "links": links
    }

def main():
    """Main conversion function."""
    # Define paths
    base_dir = os.path.dirname(os.path.abspath(__file__))
    source_dir = os.path.join(base_dir, "G_Realworldgraph", "G")
    target_dir = os.path.join(base_dir, "converted_jsons")
    
    # Create target directory if it doesn't exist
    os.makedirs(target_dir, exist_ok=True)
    
    # Input files
    edges_file = os.path.join(source_dir, "G.edges")
    labels_file = os.path.join(source_dir, "G.node_labels")
    
    print(f"Reading graph data from:")
    print(f"  Edges: {edges_file}")
    print(f"  Labels: {labels_file}")
    
    if not os.path.exists(edges_file):
        print(f"Error: Edges file not found: {edges_file}")
        return
    
    if not os.path.exists(labels_file):
        print(f"Error: Labels file not found: {labels_file}")
        return
    
    # Read and convert data
    print("Reading graph data...")
    node_labels, edges = read_graph_data(edges_file, labels_file)
    
    print(f"Found {len(node_labels)} nodes and {len(edges)} edges")
    
    print("Converting to JSON format...")
    json_data = convert_to_json(node_labels, edges)
    
    # Write output file
    output_file = os.path.join(target_dir, "realworld_graph.json")
    with open(output_file, 'w') as f:
        json.dump(json_data, f, indent=2)
    
    print(f"Conversion completed! Output saved to: {output_file}")
    print(f"Graph contains {len(json_data['nodes'])} nodes and {len(json_data['links'])} edges")

if __name__ == "__main__":
    main()