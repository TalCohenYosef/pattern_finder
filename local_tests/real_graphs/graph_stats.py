import networkx as nx
import os


def load_single_graph(folder, graph_name):
    """
    Loads a single graph from:
    - graph_name.edges
    - graph_name.node_labels

    Supports two formats for node_labels:
    1. node_id,label
    2. label only (node IDs start from 1)
    """

    G = nx.Graph()

    edges_path = os.path.join(folder, f"{graph_name}.edges")
    labels_path = os.path.join(folder, f"{graph_name}.node_labels")

    # Add nodes from node_labels file
    with open(labels_path, "r") as f:
        for i, line in enumerate(f):
            line = line.strip()
            if not line:
                continue  # skip empty lines

            # Check if line contains node_id
            if "," in line or " " in line:
                # node_id,label format
                if "," in line:
                    node_id, label = map(int, line.split(","))
                else:
                    node_id, label = map(int, line.split())
            else:
                # Only label, node_id inferred from line number (starting at 1)
                node_id = i + 1
                label = int(line)

            G.add_node(node_id, label=label)

    # Add edges
    with open(edges_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # support both formats: "u v" or "u,v"
            if "," in line:
                u, v = map(int, line.split(","))
            else:
                u, v = map(int, line.split())

            G.add_edge(u, v)

    return G


def compute_degree_stats(G):
    degrees = [d for _, d in G.degree()]
    avg_degree = sum(degrees) / len(degrees) if degrees else 0
    max_degree = max(degrees) if degrees else 0
    return avg_degree, max_degree


def analyze_folder(folder):
    graph_names = [
        "AIDS",
        "COX2",
        "DD242",
        "DHFR",
        "DHFR-MD",
        "Mutagenicity",
        "NCI109",
        "soc-Flickr-ASU"
    ]

    print("\n=== GRAPH DEGREE STATISTICS ===\n")

    for name in graph_names:
        G = load_single_graph(folder, name)
        avg_degree, max_degree = compute_degree_stats(G)
        print(f"{name}:")
        print(f"  Nodes: {G.number_of_nodes()}")
        print(f"  Edges: {G.number_of_edges()}")
        print(f"  Average degree: {avg_degree:.4f}")
        print(f"  Max degree: {max_degree}")
        print()


# run
folder = r"C:\Users\ginzb\Documents\new-graph-measures\local_tests\real_graphs"
analyze_folder(folder)
