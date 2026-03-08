import os
from graphMeasures import FeatureManager
import networkx as nx
import json

from graphMeasures.feature_calculators import MotifsNodeCalculator

CONFIGURATION = {
    "colored_directed_variations_3": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_directed_colored.pkl",
    "colored_undirected_variations_3": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_undirected_colored.pkl",
    "colored_directed_variations_4": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_directed_colored.pkl",
    "colored_undirected_variations_4": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_undirected_colored.pkl",
}


# ---------------- HELPERS ---------------- #

def read_graph_file(filename):
    graph = nx.Graph()
    with open(filename) as f:
        graph_json = json.load(f)

    for node in graph_json["nodes"]:
        graph.add_node(node["id"], color=node["color"])

    for edge in graph_json["links"]:
        graph.add_edge(edge["source"], edge["target"])

    return graph

# set of features to be calculated
feats = ["motif4", "louvain"]

# path to the graph's edgelist or nx.Graph object
# graph = os.path.join("examples", "example_graph.txt")
# graph = "examples\\example_graph.txt"
graph = read_graph_file(r"/home/cohent59/new-graph-measures/local_tests/graphs_by_density_3/g_den_5_embedded_den_3_uniform_0.json")

# The path in which one would like to save the pickled features calculated in the process.
dir_path = "..\\local_tests\\out"

configuration = "configuration\\config.json"
colors = "examples\\example_colors.json"

# More options are shown here. For information about them, refer to the file.
g_calc = MotifsNodeCalculator(
                    graph=graph,
                    colores_loaded=True,
                    configuration=CONFIGURATION,
                    level=4,
                    calc_nodes=False,
                    calc_edges=False,
                    count_motifs=True,
                    logger=None,
                )

g_motifs = g_calc.build()
print(g_motifs["sum"][47244640257])