import json
import os
import pickle
import logging

import networkx as nx

from graphMeasures.feature_calculators import MotifsNodeCalculator
from graphMeasures.loggers import PrintLogger


# ---------------- CONFIG ---------------- #

BASE_DIR = os.path.join(os.getcwd(), "local_tests")

GRAPH_DIR = os.path.join(BASE_DIR,"graphs_by_density_3")

PICKLE_DIR = os.path.join(BASE_DIR,"induced","cache")

LOG_DIR = os.path.join(BASE_DIR,"induced","logs")

COMPARE_RESULTS = os.path.join(LOG_DIR,"compare_results")

# -------- SHARED TEMP CACHE -------- #

TEMP_DIR = os.path.join(BASE_DIR,"temp")

S_CACHE_DIR = os.path.join(TEMP_DIR,"s_motif_cache")

os.makedirs(PICKLE_DIR,exist_ok=True)
os.makedirs(COMPARE_RESULTS,exist_ok=True)
os.makedirs(S_CACHE_DIR,exist_ok=True)


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
        graph.add_node(node["id"],color=node["color"])

    for edge in graph_json["links"]:
        graph.add_edge(edge["source"],edge["target"])

    return graph


# ---------- S CACHE ---------- #

def get_cached_S_motifs(S_path):

    parent_dir = os.path.basename(
        os.path.dirname(S_path)
    )

    S_name = os.path.basename(S_path)

    cache_name = f"{parent_dir}__{S_name}".replace(
        ".json",".pkl"
    )

    cache_file = os.path.join(
        S_CACHE_DIR,
        cache_name
    )

    # ---- LOAD CACHE ----

    if os.path.exists(cache_file):

        with open(cache_file,"rb") as f:
            return pickle.load(f)


    # ---- COMPUTE ----

    S = read_graph_file(S_path)

    s_calc = MotifsNodeCalculator(

        graph=S,
        colores_loaded=True,
        configuration=CONFIGURATION,
        level=MOTIF_SIZE,
        calc_nodes=False,
        calc_edges=False,
        count_motifs=True,
    )

    s_motifs = s_calc.build()[
        MotifsNodeCalculator.MOTIF_SUM_KEY
    ]

    with open(cache_file,"wb") as f:
        pickle.dump(s_motifs,f)

    return s_motifs


# ---------------- MAIN ---------------- #

def main():

    SUMMARY_LOG = os.path.join(

        COMPARE_RESULTS,

        "summary_induced_different_distributions_3.log"

    )

    summary_logger = logging.getLogger(
        "summary_induced_different_distributions_3"
    )

    summary_logger.setLevel(logging.INFO)

    if not summary_logger.handlers:

        handler = logging.FileHandler(SUMMARY_LOG)

        handler.setFormatter(
            logging.Formatter(
                "%(asctime)s - %(message)s"
            )
        )

        summary_logger.addHandler(handler)


    for graph_avg_neighs in [5,8,10,13,15]:

        for color_distribution in [

            'uniform',
            'average',
            'rare'
        ]:

            INPUT_DIR = os.path.join(

                BASE_DIR,

                f"input_color_{color_distribution}_deg_3"

            )

            avg_false_positives = 0


            for j in range(10):

                graph_file_name = \
f"g_den_{graph_avg_neighs}_embedded_den_3_{color_distribution}_{j}"


                LOG_FILE = os.path.join(

                    COMPARE_RESULTS,

                    f"{graph_file_name}.log"

                )


                logger = logging.getLogger(
                    graph_file_name
                )

                logger.setLevel(logging.INFO)

                if not logger.handlers:

                    handler = logging.FileHandler(
                        LOG_FILE
                    )

                    handler.setFormatter(

                        logging.Formatter(
                            "%(asctime)s - %(message)s"
                        )
                    )

                    logger.addHandler(handler)


                G_PICKLE = os.path.join(

                    PICKLE_DIR,

                    f"{graph_file_name}.pkl"

                )


                # ---------- LOAD OR COMPUTE G ----------

                if os.path.exists(G_PICKLE):

                    with open(G_PICKLE,"rb") as f:

                        g_motifs = pickle.load(f)

                    print(
f"Loaded cached G motifs for {graph_file_name}"
                    )

                else:

                    G = read_graph_file(

                        os.path.join(

                            GRAPH_DIR,

                            f"{graph_file_name}.json"

                        )
                    )

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

                    with open(G_PICKLE,"wb") as f:
                        pickle.dump(g_motifs,f)

                    print(
f"Computed and cached G motifs for {graph_file_name}"
                    )


                g_sum = g_motifs.get(
                    MotifsNodeCalculator.MOTIF_SUM_KEY
                )

                g_motifs.pop(
                    MotifsNodeCalculator.MOTIF_SUM_KEY
                )


                false_pos_sum_only = 0


                # ---------- S PROCESS ----------

                for i in range(1,101):

                    S_path = os.path.join(

                        INPUT_DIR,

                        f"S_{i}.json"

                    )

                    s_motifs = get_cached_S_motifs(
                        S_path
                    )

                    feasible_sum=True

                    for m,cnt in s_motifs.items():

                        if g_sum.get(m,0)<cnt:

                            feasible_sum=False
                            break


                    if i>10 and feasible_sum:

                        false_pos_sum_only+=1


                    logger.info(

f"SUM {'PASS' if feasible_sum else 'FAIL'} S_{i}"

                    )

                    print(f"Done S_{i}")


                summary_logger.info(

f"{graph_file_name} | sum_only={false_pos_sum_only}"

                )

                avg_false_positives+=false_pos_sum_only


            avg_false_positives/=10


            summary_logger.info(

f"g_den_{graph_avg_neighs}_embedded_den_5_{color_distribution}"

f" | AVERAGE sum_only={avg_false_positives}"

            )


if __name__=="__main__":
    main()