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
PICKLE_DIR = os.path.join(BASE_DIR, "induced", "cache")
LOG_DIR = os.path.join(BASE_DIR, "induced", "logs")

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


# ---------------- MAIN ---------------- #

def main():
    SUMMARY_LOG = os.path.join(LOG_DIR, "summary_induced_motifs.log")

    summary_logger = logging.getLogger("summary_induced_motifs")
    summary_logger.setLevel(logging.INFO)
    summary_handler = logging.FileHandler(SUMMARY_LOG)
    summary_logger.addHandler(summary_handler)

    for graph_avg_neighs in [3, 8, 15]:
        for color_distribution in ['uniform', 'average', 'rare']:

            #run_name = f"color_{color_distribution}_deg_{graph_avg_neighs}"
            #INPUT_DIR = os.path.join(BASE_DIR, "local_tests", f"input_{run_name}")
            run_name = f"color_{color_distribution}_deg_{graph_avg_neighs}"
            INPUT_DIR = os.path.join(BASE_DIR, f"input_{run_name}")
            PICKLE_FILE = os.path.join(PICKLE_DIR, f"G_motifs_{run_name}.pkl")
            LOG_FILE = os.path.join(LOG_DIR, f"induced_{run_name}.log")

            # -------- configure per-run logger --------
            logging.basicConfig(
                filename=LOG_FILE,
                level=logging.INFO,
                format="%(asctime)s - %(message)s",
                force=True
            )

            logging.info(f"Starting run {run_name}")


            # ---------------- LOGGING ---------------- #

            # ----- Load or compute G motifs -----
            if os.path.exists(PICKLE_FILE):
                with open(PICKLE_FILE, "rb") as f:
                    g_motifs = pickle.load(f)
                print("Loaded cached G motifs")
            else:
                G = read_graph_file(os.path.join(INPUT_DIR, "G_induced.json"))
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

                g_motifs = g_calc.build()

                g_motifs = g_motifs[MotifsNodeCalculator.MOTIF_SUM_KEY]

                with open(PICKLE_FILE, "wb") as f:
                    pickle.dump(g_motifs, f)

                print("Computed and cached G motifs")


            # solver = NodeSelectorLP(g_motifs, MOTIF_SIZE)

            false_pos_sum_only = 0
            # false_pos_sum_and_lp = 0

            # ----- Process S graphs -----
            for i in range(1, 1001):
                S = read_graph_file(os.path.join(INPUT_DIR, f"S_{i}.json"))

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

                # ---------- Stage 1: motif sum check ----------
                feasible_sum = True
                for m, cnt in s_motifs.items():
                    if g_motifs.get(m, 0) < cnt:
                        feasible_sum = False
                        #print(f"SUM FAIL on motif {m} with count {cnt} vs {g_motifs.get(m, 0)}")
                        break

                if not feasible_sum:
                    logging.info(f"SUM FAIL S_{i}")
                if feasible_sum:
                    logging.info(f"SUM PASS S_{i}")
                    false_pos_sum_only += 1


                print(f"Done S_{i}")

            print("False positives (sum only):", false_pos_sum_only)
            summary_logger.info(
               f"{run_name} | sum_only={false_pos_sum_only}"
            )


if __name__ == "__main__":
    main()
