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
GRAPH_DIR = os.path.join(BASE_DIR, "graphs_by_density_3")
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


# ---------------- LP SOLVER ---------------- #

class NodeSelectorLP:
    def __init__(self, node_motifs: dict[int, dict[int, int]], motif_size: int):
        self.node_motifs = node_motifs
        self.node_ids = list(node_motifs.keys())
        self.node_index = {n: i for i, n in enumerate(self.node_ids)}
        self.num_nodes = len(self.node_ids)
        self.motif_size = motif_size

        self.available_motifs = set()
        for d in node_motifs.values():
            self.available_motifs.update(d.keys())

    def solve(self, required_motifs: dict[int, int], max_nodes: int) -> bool:
        for m in required_motifs:
            if m not in self.available_motifs:
                return False

        motif_ids = list(required_motifs.keys())
        motif_index = {m: i for i, m in enumerate(motif_ids)}

        rows, cols, data = [], [], []

        for node, motifs in self.node_motifs.items():
            i = self.node_index[node]
            for m, cnt in motifs.items():
                if m in motif_index:
                    rows.append(motif_index[m])
                    cols.append(i)
                    data.append(-cnt)

        A_motifs = coo_matrix(
            (data, (rows, cols)),
            shape=(len(motif_ids), self.num_nodes)
        )

        b_motifs = -np.array([
            required_motifs[m] * self.motif_size for m in motif_ids
        ])

        A_nodes = np.ones((1, self.num_nodes))
        b_nodes = np.array([max_nodes])

        A = np.vstack([A_motifs.toarray(), A_nodes])
        b = np.concatenate([b_motifs, b_nodes])

        res = linprog(
            c=np.ones(self.num_nodes),
            A_ub=A,
            b_ub=b,
            bounds=[(0, 1)] * self.num_nodes,
            method="highs"
        )

        return res.success

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
    SUMMARY_LOG = os.path.join(LOG_DIR, "summary_non_induced_different_distributions.log")

    summary_logger = logging.getLogger("summary_non_induced_different_distributions")
    summary_logger.setLevel(logging.INFO)
    summary_handler = logging.FileHandler(SUMMARY_LOG)
    summary_logger.addHandler(summary_handler)

    for color_distribution in ['uniform', 'average', 'rare']:
        for graph_avg_neighs in [5, 8, 10, 13, 15]:

            INPUT_DIR = os.path.join(BASE_DIR, f"input_color_{color_distribution}_deg_3")

            avg_false_positives = 0
            
            for i in range(10):
                # ---------------- LOGGING ---------------- #

                graph_file_name = f'g_den_{graph_avg_neighs}_embedded_den_3_{color_distribution}_{i}'

                LOG_FILE = os.path.join(LOG_DIR, f"{graph_file_name}.log")

                logging.basicConfig(
                    filename=LOG_FILE,
                    level=logging.INFO,
                    format="%(asctime)s - %(message)s",
                )

                G_PICKLE = os.path.join(PICKLE_DIR, f"{graph_file_name}.pkl")
                # ----- Load or compute G motifs -----
                if os.path.exists(G_PICKLE):
                    with open(G_PICKLE, "rb") as f:
                        g_motifs = pickle.load(f)
                    print("Loaded cached G motifs")
                else:
                    G = read_graph_file(os.path.join(GRAPH_DIR, f"{graph_file_name}.json"))
                    g_calc = MotifsNodeCalculator(
                        graph=G,
                        colores_loaded=True,
                        configuration=CONFIGURATION,
                        level=MOTIF_SIZE,
                        calc_nodes=True,
                        calc_edges=False,
                        count_motifs=True,
                        logger=PrintLogger(),
                    )

                    g_motifs = g_calc.build()

                    with open(G_PICKLE, "wb") as f:
                        pickle.dump(g_motifs, f)

                    print("Computed and cached G motifs")

                g_sum = g_motifs.get(MotifsNodeCalculator.MOTIF_SUM_KEY)
                g_motifs.pop(MotifsNodeCalculator.MOTIF_SUM_KEY)

                # solver = NodeSelectorLP(g_motifs, MOTIF_SIZE)

                false_pos_sum_only = 0
                # false_pos_sum_and_lp = 0

                # ----- Process S graphs -----
                for i in range(1, 101):
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
                    max_num_of_edges = 0
                    for m, cnt in s_motifs.items():
                        if g_sum.get(m, 0) < cnt:
                            feasible_sum = False
                            break

                    if i > 10 and feasible_sum:
                        false_pos_sum_only += 1

                    if not feasible_sum:
                        logging.info(f"SUM FAIL S_{i}")
                    else:
                        logging.info(f"SUM PASS S_{i}")

                    # ---------- Stage 2: LP ----------
                    # feasible_lp = solver.solve(s_motifs, max_nodes=len(S.nodes))

                    # if i <= 10 and not feasible_lp:
                    #     logging.info(f"FAILED TRUE POSITIVE S_{i}")
                    #     print(f"FAILED ON {i}")

                    # elif i > 10 and feasible_lp:
                    #     false_pos_sum_and_lp += 1
                    #     logging.info(f"LP FALSE POSITIVE S_{i}")

                    print(f"Done S_{i}")

                # print("False positives (sum only):", false_pos_sum_only)
                # print("False positives (sum + LP):", false_pos_sum_and_lp)
                # summary_logger.info(
                #    f"{graph_file_name} | sum_only={false_pos_sum_only} | sum_plus_lp={false_pos_sum_and_lp}"
                # )
                summary_logger.info(f"{graph_file_name} | sum_only={false_pos_sum_only}")
                avg_false_positives += false_pos_sum_only
            avg_false_positives /= 10
            summary_logger.info(f"g_den_{graph_avg_neighs}_embedded_den_3_{color_distribution} | AVERAGE sum_only={avg_false_positives}")


if __name__ == "__main__":
    main()
