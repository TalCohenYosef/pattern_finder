import json
import os
import pickle
import logging
from collections import defaultdict

import networkx as nx
import numpy as np
from scipy.sparse import coo_matrix
from scipy.optimize import linprog

from graphMeasures.feature_calculators import MotifsNodeCalculator
from graphMeasures.loggers import PrintLogger

# ---------------- CONFIG ---------------- #

BASE_DIR = os.path.join(os.getcwd(), "local_tests")
PICKLE_DIR = os.path.join(BASE_DIR, "non_induced", "cache")
LOG_DIR = os.path.join(BASE_DIR, "non_induced", "logs")

os.makedirs(PICKLE_DIR, exist_ok=True)

CONFIGURATION = {
    "colored_directed_variations_3": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_directed_colored.pkl",
    "colored_undirected_variations_3": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_undirected_colored.pkl",
    "colored_directed_variations_4": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_directed_colored.pkl",
    "colored_undirected_variations_4": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_undirected_colored.pkl",
}

MOTIF_SIZE = 4


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


def preprocess_motifs_for_non_induced(motif_size, motifs, motif_graph):
    keys_to_add = {}
    for motif in motifs:
        motif_number = motif >> (8 * motif_size)
        colors_bits = motif % (1 << (8 * motif_size))
        color_array = [((colors_bits >> (8 * (motif_size - 1 - i))) % (1 << 8)) for i in range(motif_size)]

        for _, v, data in motif_graph.out_edges(motif_number, data=True):
            for permutation in data["permutations"]:
                color_perm = 0
                    
                for i in range(len(permutation)):
                    color_perm += color_array[i] << ((motif_size - 1 - permutation[i]) * 8)
                        
                perm_motif_num = (v << (8 * motif_size)) + color_perm

                if perm_motif_num not in motifs:
                    if perm_motif_num not in keys_to_add:
                        keys_to_add[perm_motif_num] = 0
                    keys_to_add[perm_motif_num] += 1
                else:
                    motifs[perm_motif_num] += motifs[motif]
    motifs.update(keys_to_add)


# ---------------- MAIN ---------------- #

def main():
    SUMMARY_LOG = os.path.join(LOG_DIR, "summary_non_induced_motifs.log")

    summary_logger = logging.getLogger("summary_non_induced_motifs")
    summary_logger.setLevel(logging.INFO)
    summary_handler = logging.FileHandler(SUMMARY_LOG)
    summary_logger.addHandler(summary_handler)

    with open(f"local_tests/non_induced/create_inclusion_motifs_dag/{MOTIF_SIZE}_undirected_colored_dag", 'rb') as motif_graph_file:
        motif_graph = pickle.load(motif_graph_file)

    # Folder that contains S.json and G_1.json ... G_n.json
    INPUT_DIR = os.path.join(BASE_DIR, "example_non_induced_2")

    LOG_FILE = os.path.join(LOG_DIR, "non_induced_example.log")
    logging.basicConfig(
        filename=LOG_FILE,
        level=logging.INFO,
        format="%(asctime)s - %(message)s",
        force=True
    )

    logging.info("Starting example_non_induced_2 run")

    # ---------------- Load and compute S motifs ---------------- #

    S = read_graph_file(os.path.join(INPUT_DIR, "S.json"))

    s_calc = MotifsNodeCalculator(
        graph=S,
        colores_loaded=True,
        configuration=CONFIGURATION,
        level=MOTIF_SIZE,
        calc_nodes=False,
        calc_edges=False,
        count_motifs=True,
    )

    s_motifs = s_calc.build()[MotifsNodeCalculator.MOTIF_SUM_KEY]
    print(f"s_motifs = {s_motifs}")

    print("Computed S motifs")

    # ---------------- Loop over all G_i ---------------- #

    false_negatives = 0
    total = 0

    # Find all G_*.json files
    g_files = sorted(
        f for f in os.listdir(INPUT_DIR)
        if f.startswith("G_") and f.endswith(".json")
    )

    for g_file in g_files:
        total += 1
        g_index = g_file.replace(".json", "")

        G = read_graph_file(os.path.join(INPUT_DIR, g_file))

        g_calc = MotifsNodeCalculator(
            graph=G,
            colores_loaded=True,
            configuration=CONFIGURATION,
            level=MOTIF_SIZE,
            calc_nodes=False,
            calc_edges=False,
            count_motifs=True,
            logger=PrintLogger(),
        )

        g_motifs = g_calc.build()[MotifsNodeCalculator.MOTIF_SUM_KEY]

        print(f"g_motifs = {g_motifs}")

        # Preprocess G motifs for non-induced
        preprocess_motifs_for_non_induced(MOTIF_SIZE, g_motifs, motif_graph)



        # ---------- Stage 1: motif sum check (S inside G) ----------

        feasible_sum = True
        for m, cnt in s_motifs.items():
            if m.bit_count() < 4:
                continue
            if g_motifs.get(m, 0) < cnt:
                feasible_sum = False
                break

        if feasible_sum:
            print(f"✔ S found in {g_file}")
            logging.info(f"S found in {g_file}")
        else:
            print(f"✘ S NOT found in {g_file}")
            logging.info(f"S NOT found in {g_file}")
            false_negatives += 1

    print("===================================")
    print("Total G graphs checked:", total)
    print("Misses (should be 0 if correct):", false_negatives)

    summary_logger.info(
        f"example_non_induced_2 | total={total} misses={false_negatives}"
    )


if __name__ == "__main__":
    main()
