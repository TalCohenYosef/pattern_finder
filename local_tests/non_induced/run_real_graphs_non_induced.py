import json
import os
import pickle
import logging

import networkx as nx

from graphMeasures.feature_calculators import MotifsNodeCalculator
from graphMeasures.loggers import PrintLogger


# ---------------- PATH CONFIG ---------------- #

BASE_DIR = "/home/cohent59/new-graph-measures/local_tests"

REAL_GRAPHS_DIR = os.path.join(BASE_DIR, "real_graphs")

G_NAME = "DHFR-MD"

S_DIR = os.path.join(REAL_GRAPHS_DIR, "NCI109_subgraphs")

CACHE_DIR = os.path.join(BASE_DIR, "non_induced", "real_graphs", "cache")

LOG_FILE = os.path.join(BASE_DIR, "non_induced", "logs", "DHFR-MD_summary.log")

os.makedirs(CACHE_DIR, exist_ok=True)

PICKLE_FILE = os.path.join(CACHE_DIR, f"{G_NAME}_motifs.pkl")


CONFIGURATION = {
    "colored_directed_variations_3": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_directed_colored.pkl",
    "colored_undirected_variations_3": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_undirected_colored.pkl",
    "colored_directed_variations_4": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_directed_colored.pkl",
    "colored_undirected_variations_4": "graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_undirected_colored.pkl",
}

MOTIF_SIZE = 4


# ---------------- LOGGING ---------------- #

logging.basicConfig(
    filename=LOG_FILE,
    level=logging.INFO,
    format="%(asctime)s - %(message)s",
    force=True
)


# ---------------- LOADERS ---------------- #

def load_single_graph(folder, graph_name):

    G = nx.Graph()

    edges_path = os.path.join(folder, f"{graph_name}.edges")
    labels_path = os.path.join(folder, f"{graph_name}.node_labels")

    with open(labels_path, "r") as f:
        for i, line in enumerate(f):

            line = line.strip()

            if not line:
                continue

            if "," in line or " " in line:

                if "," in line:
                    node_id, label = map(int, line.split(","))

                else:
                    node_id, label = map(int, line.split())

            else:

                node_id = i + 1
                label = int(line)

            G.add_node(node_id, color=label)

    with open(edges_path, "r") as f:

        for line in f:

            line = line.strip()

            if not line:
                continue

            if "," in line:
                u, v = map(int, line.split(","))

            else:
                u, v = map(int, line.split())

            G.add_edge(u, v)

    return G


def read_graph_file(filename):

    graph = nx.Graph()

    with open(filename) as f:
        graph_json = json.load(f)

    # IMPORTANT: your format uses "node" not "nodes"
    for node in graph_json["nodes"]:

        graph.add_node(node["id"], color=node["color"])

    for edge in graph_json["links"]:

        graph.add_edge(edge["source"], edge["target"])

    return graph


# ---------------- NON-INDUCED PREPROCESS ---------------- #

def preprocess_motifs_for_non_induced(motif_size, motifs, motif_graph):

    keys_to_add = {}

    for motif in list(motifs.keys()):

        motif_number = motif >> (8 * motif_size)

        colors_bits = motif % (1 << (8 * motif_size))

        color_array = [
            ((colors_bits >> (8 * (motif_size - 1 - i))) % (1 << 8))
            for i in range(motif_size)
        ]

        for _, v, data in motif_graph.out_edges(motif_number, data=True):

            for permutation in data["permutations"]:

                color_perm = 0

                for i in range(len(permutation)):

                    color_perm += color_array[i] << (
                        (motif_size - 1 - permutation[i]) * 8
                    )

                perm_motif_num = (v << (8 * motif_size)) + color_perm

                if perm_motif_num not in motifs:

                    keys_to_add[perm_motif_num] = keys_to_add.get(perm_motif_num, 0) + motifs[motif]

                else:

                    motifs[perm_motif_num] += motifs[motif]

    motifs.update(keys_to_add)


# ---------------- MAIN ---------------- #

def main():

    logging.info("==== START REAL GRAPH RUN ====")

    # ---------- LOAD OR COMPUTE G ----------

    if os.path.exists(PICKLE_FILE):

        with open(PICKLE_FILE, "rb") as f:

            g_motifs = pickle.load(f)

        logging.info("Loaded cached G motifs")

    else:

        logging.info("Computing G motifs")

        G = load_single_graph(REAL_GRAPHS_DIR, G_NAME)

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

        # load motif graph from configuration
        with open("/home/cohent59/new-graph-measures/local_tests/non_induced/create_inclusion_motifs_dag/4_undirected_colored_dag", "rb") as f:
            motif_graph = pickle.load(f)


        preprocess_motifs_for_non_induced(
            MOTIF_SIZE,
            g_motifs,
            motif_graph
        )

        with open(PICKLE_FILE, "wb") as f:

            pickle.dump(g_motifs, f)

        logging.info("Computed and cached G motifs")

    # ---------- PROCESS ALL S ----------

    false_pos_sum_only = 0
    processed = 0

    for i in range(0, 1000):

        S_PATH = os.path.join(S_DIR, f"S_{i}.json")

        if not os.path.exists(S_PATH):

            logging.warning(f"Missing S_{i}.json")

            continue

        logging.info(f"Processing S_{i}")

        S = read_graph_file(S_PATH)

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

        feasible_sum = True

        for m, cnt in s_motifs.items():

            if g_motifs.get(m, 0) < cnt:

                feasible_sum = False
                break

        if feasible_sum:

            false_pos_sum_only += 1

            logging.info(f"SUM PASS S_{i}")

        else:

            logging.info(f"SUM FAIL S_{i}")

        processed += 1

        if processed % 50 == 0:

            logging.info(f"Progress: {processed}/1000")

    logging.info("==== FINISHED ====")

    logging.info(f"Processed: {processed}")

    logging.info(f"False positives: {false_pos_sum_only}")

    print("Done.")
    print("Processed:", processed)
    print("False positives:", false_pos_sum_only)


# ---------------- ENTRY ---------------- #

if __name__ == "__main__":

    main()
