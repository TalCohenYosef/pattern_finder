import os
import pickle
from collections import defaultdict
from itertools import combinations, permutations

import networkx as nx
from bitstring import BitArray


OUT_FOLDER_PATH = r""

class IsomorphismDAGGenerator:
    def __init__(self, group_size, is_directed):
        self._group_size = group_size
        self._is_directed = is_directed
        self._generate_isomorphisms()
        self._build_dag()

    def _generate_isomorphisms(self):
        num_edges = int(self._group_size * (self._group_size - 1) / 2.)
        num_bits = num_edges * 2 if self._is_directed else num_edges
        edge_iter = permutations if self._is_directed else combinations
        graph_type = nx.DiGraph if self._is_directed else nx.Graph

        generated = [False] * (2 ** num_bits)
        graphs = {}
        for num in range(2 ** num_bits):
            
            if generated[num]:
                continue

            # generate smallest_isomorphism graph
            g = graph_type()
            g.add_nodes_from(range(self._group_size))
            all_pairs = list(edge_iter(range(self._group_size), 2))
            g.add_edges_from((x, y) for i, (x, y) in enumerate(all_pairs) if ((2 ** (len(all_pairs)-1)) >> i) & num)

            if not nx.is_connected(g.to_undirected()):
                continue

            isomorphism_permutations = defaultdict(list)

            # generate all isomorphs
            for permutation in permutations(g.nodes()):
                func = permutations if self._is_directed else combinations
                # Reversing is a technical issue. We saved our node variations files
                bit_form = BitArray(g.has_edge(n1, n2) for n1, n2 in func(permutation, 2))
                motif_permutation_number = bit_form.uint
                generated[motif_permutation_number] = True
                isomorphism_permutations[motif_permutation_number].append(permutation)

            graphs[num] = [(motif, perms) for motif, perms in isomorphism_permutations.items()]

        self._graphs = graphs


    def _build_dag(self):
        motifs_graph = nx.DiGraph()

        # add nodes
        for min_motif_isomorphism in self._graphs.keys():
            motifs_graph.add_node(min_motif_isomorphism)

        # add edges
        print(self._graphs.keys())
        for node_destination in self._graphs.keys():
            for node_src in self._graphs.keys():
                if node_destination.bit_count() >= node_src.bit_count():
                    continue
                color_permutations = []
                for dest_permutation, min_to_rep_color_perm in self._graphs[node_destination]:
                    if dest_permutation & node_src == dest_permutation:
                        color_permutations += min_to_rep_color_perm
                if len(color_permutations) != 0:
                    motifs_graph.add_edge(node_src, node_destination, permutations=color_permutations)
        self.motifs_graphs = motifs_graph


def main(level, is_directed):
    fname = os.path.join(OUT_FOLDER_PATH, "%d_%sdirected_colored_dag" % (level, "" if is_directed else "un"))
    print("Calculating ", fname)
    gs = IsomorphismDAGGenerator(level, is_directed)
    with open(fname, "wb") as f:
        pickle.dump(gs.motifs_graphs, f)
    print("Finished calculating ", fname)


if __name__ == "__main__":
    #main(3, False)
    #main(3, True)
    main(4, False)
    #main(4, True)
