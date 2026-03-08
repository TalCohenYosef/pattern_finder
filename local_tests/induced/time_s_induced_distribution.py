import json
import os
import pickle
import logging
import shutil
import time

import networkx as nx

from graphMeasures.feature_calculators import MotifsNodeCalculator
from graphMeasures.loggers import PrintLogger


# ---------------- CONFIG ---------------- #

BASE_DIR = os.path.join(os.getcwd(), "local_tests")

GRAPH_DIR = os.path.join(BASE_DIR, "graphs_by_density_3")

LOG_ROOT = os.path.join(BASE_DIR, "induced", "logs")
COMPARE_LOG_DIR = os.path.join(LOG_ROOT, "compare_results")

PICKLE_DIR = os.path.join(BASE_DIR, "induced", "cache")

os.makedirs(PICKLE_DIR, exist_ok=True)
os.makedirs(COMPARE_LOG_DIR, exist_ok=True)

# Temporary S cache (deleted later)
TEMP_S_CACHE = os.path.join(COMPARE_LOG_DIR, "temp_s_cache")
os.makedirs(TEMP_S_CACHE, exist_ok=True)


CONFIGURATION = {

"colored_directed_variations_3":
"graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_directed_colored.pkl",

"colored_undirected_variations_3":
"graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/3_undirected_colored.pkl",

"colored_directed_variations_4":
"graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_directed_colored.pkl",

"colored_undirected_variations_4":
"graphMeasures/feature_calculators/node_features_calculators/calculators/motif_variations/4_undirected_colored.pkl",

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


def compute_S_motifs_cached(s_path):

    cache_name = os.path.basename(s_path).replace(".json",".pkl")

    cache_file = os.path.join(TEMP_S_CACHE, cache_name)

    if os.path.exists(cache_file):

        with open(cache_file,"rb") as f:
            return pickle.load(f)

    # compute
    S = read_graph_file(s_path)

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

    with open(cache_file,"wb") as f:
        pickle.dump(s_motifs,f)

    return s_motifs


# ---------------- MAIN ---------------- #

def main():

    TIMES_LOG_FILE = os.path.join(
        COMPARE_LOG_DIR,
        "times.log"
    )

    SUMMARY_LOG_FILE = os.path.join(
        COMPARE_LOG_DIR,
        "summary.log"
    )


    # ---- Summary logger ----

    summary_logger = logging.getLogger("summary")

    summary_logger.setLevel(logging.INFO)

    summary_handler = logging.FileHandler(SUMMARY_LOG_FILE)

    summary_handler.setFormatter(
        logging.Formatter("%(asctime)s - %(message)s")
    )

    summary_logger.addHandler(summary_handler)


    # ---- Times logger ----

    times_logger = logging.getLogger("times")

    times_logger.setLevel(logging.INFO)

    times_handler = logging.FileHandler(TIMES_LOG_FILE)

    times_handler.setFormatter(
        logging.Formatter("%(asctime)s - %(message)s")
    )

    times_logger.addHandler(times_handler)


    try:

        for graph_avg_neighs in [5,8,10,13,15]:

            for color_distribution in ['uniform','average','rare']:


                INPUT_DIR = os.path.join(
                    BASE_DIR,
                    f"input_color_{color_distribution}_deg_3"
                )


                # ---------- ONLY _0 GRAPH ----------

                graph_file_name = \
f"g_den_{graph_avg_neighs}_embedded_den_3_{color_distribution}_0"


                LOG_FILE = os.path.join(
                    COMPARE_LOG_DIR,
                    f"{graph_file_name}.log"
                )


                logger = logging.getLogger(graph_file_name)

                logger.setLevel(logging.INFO)

                handler = logging.FileHandler(LOG_FILE)

                handler.setFormatter(
                    logging.Formatter("%(asctime)s - %(message)s")
                )

                logger.addHandler(handler)


                total_G_start = time.perf_counter()


                # ---------- LOAD / COMPUTE G ----------

                G_PICKLE = os.path.join(
                    PICKLE_DIR,
                    f"{graph_file_name}.pkl"
                )

                if os.path.exists(G_PICKLE):

                    with open(G_PICKLE,"rb") as f:
                        g_motifs = pickle.load(f)

                    print("Loaded cached G motifs")

                    G_time = 0

                else:

                    G = read_graph_file(
                        os.path.join(
                            GRAPH_DIR,
                            f"{graph_file_name}.json"
                        )
                    )

                    start = time.perf_counter()

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

                    G_time = time.perf_counter() - start

                    with open(G_PICKLE,"wb") as f:
                        pickle.dump(g_motifs,f)

                times_logger.info(
f"{graph_file_name} | G_compute_time={G_time:.4f}s"
                )


                g_sum = g_motifs.get(
                    MotifsNodeCalculator.MOTIF_SUM_KEY
                )

                g_motifs.pop(
                    MotifsNodeCalculator.MOTIF_SUM_KEY
                )


                false_pos_sum_only = 0


                # ---------- PROCESS S ----------

                for i in range(1,101):

                    S_file = os.path.join(
                        INPUT_DIR,
                        f"S_{i}.json"
                    )

                    start = time.perf_counter()

                    s_motifs = compute_S_motifs_cached(
                        S_file
                    )

                    S_time = time.perf_counter() - start

                    times_logger.info(
f"{graph_file_name} | S_{i} compute_time={S_time:.4f}s"
                    )


                    feasible_sum = True

                    for m,cnt in s_motifs.items():

                        if g_sum.get(m,0) < cnt:

                            feasible_sum=False
                            break


                    if i>10 and feasible_sum:
                        false_pos_sum_only+=1


                    logger.info(
f"S_{i} {'PASS' if feasible_sum else 'FAIL'}"
                    )


                total_time = time.perf_counter() - total_G_start

                times_logger.info(
f"{graph_file_name} | TOTAL_BATCH_TIME={total_time:.4f}s"
                )


                summary_logger.info(
f"{graph_file_name} | sum_only={false_pos_sum_only}"
                )


    finally:

        # -------- DELETE TEMP CACHE --------

        if os.path.exists(TEMP_S_CACHE):

            shutil.rmtree(TEMP_S_CACHE)

            print("Temporary S cache deleted.")


if __name__ == "__main__":
    main()