import json
import random
import os
import networkx as nx

INPUT_DIR = "/home/sapir/pattern_finder/Graph-Search/inputs"
G_JSON = os.path.join(INPUT_DIR, "G.json")

START_IDX = 51
END_IDX = 100
REFERENCE_RANGE = range(1, 51)  # use S_1 ... S_50 for sizes


# ---------- Load Graph from JSON ----------
def load_graph_from_json(filename):
    with open(filename, "r") as f:
        data = json.load(f)

    G = nx.Graph()

    for node in data["nodes"]:
        G.add_node(node["id"], color=node["color"])

    for edge in data["links"]:
        G.add_edge(edge["source"], edge["target"])

    return G


# ---------- Load size of existing S_i ----------
def load_subgraph_size(i):
    filename = os.path.join(INPUT_DIR, f"S_{i}.json")
    with open(filename, "r") as f:
        data = json.load(f)
    return len(data["nodes"])


# ---------- Extract induced subgraph ----------
def sample_induced_subgraph(G, k):
    nodes = random.sample(list(G.nodes()), k)
    S = G.subgraph(nodes).copy()

    # relabel nodes to 0..k-1
    mapping = {old: new for new, old in enumerate(S.nodes())}
    S = nx.relabel_nodes(S, mapping)

    return S


# ---------- Convert to JSON ----------
def graph_to_json_struct(S):
    data = {"nodes": [], "links": []}

    for u in S.nodes():
        data["nodes"].append({
            "id": int(u),
            "color": int(S.nodes[u]["color"])
        })

    for u, v in S.edges():
        data["links"].append({
            "source": int(u),
            "target": int(v)
        })

    return data


# ---------- Save ----------
def save_json(obj, filename):
    with open(filename, "w") as f:
        json.dump(obj, f, indent=2)

def add_random_edges(S, G=None, num_edges=50):
    """
    Add random edges between existing nodes in S.
    If G is provided, the same edges are also added to G.
    """
    nodes = list(S.nodes())
    added = 0
    attempts = 0
    max_attempts = 50 * num_edges

    while added < num_edges and attempts < max_attempts:
        u, v = random.sample(nodes, 2)
        attempts += 1

        if S.has_edge(u, v):
            continue

        # add to S
        S.add_edge(u, v)

        # optionally add to G
        if G is not None:
            G.add_edge(u, v)

        added += 1

    if added < num_edges:
        print(f"Only added {added}/{num_edges} edges")

    return S

# ---------- Main ----------
if __name__ == "__main__":
    print("Loading G...")
    G = load_graph_from_json(G_JSON)
    print(f"G loaded: |V|={G.number_of_nodes()}, |E|={G.number_of_edges()}")

    for i in range(START_IDX, END_IDX + 1):
        # choose size from earlier S
        ref_i = random.choice(REFERENCE_RANGE)
        k = load_subgraph_size(ref_i)

        S = sample_induced_subgraph(G, k)

        # add noise: 3 extra edges
        S = add_random_edges(S, G=G, num_edges=50)

        data_S = graph_to_json_struct(S)

        out_file = os.path.join(INPUT_DIR, f"S_{i}.json")
        save_json(data_S, out_file)

        print(f"S_{i} saved (k={k})")

    print("DONE: induced subgraphs generated.")
