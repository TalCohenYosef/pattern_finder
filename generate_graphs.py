import networkx as nx
import matplotlib.pyplot as plt
import random
import json
from collections import deque
import inspect
import os, sys
OUTPUT_DIR = r"/home/sapir/pattern_finder/Graph-Search/inputs"

# Create Colored Graph G(n,p)
def create_colored_graph(n,p):
    
    C=20
    pi=None
    seed=None

    if seed is not None:
        random.seed(seed)

    # Default color distribution
    if pi is None:
        pi = [
            0.20, 0.18, 0.15, 0.12, 0.10,  # 5 dominant colors (75%)
            0.06, 0.05, 0.04, 0.03, 0.02,  # medium colors
            0.01, 0.01, 0.01, 0.01, 0.01,  # rare
            0.005,0.005,0.005,0.005,0.005  # very rare
            ]


    G = nx.erdos_renyi_graph(n=n, p=p)

    # Assign colors
    for v in G.nodes():
        G.nodes[v]['color'] = random.choices(range(C), weights=pi, k=1)[0]

    components = list(nx.connected_components(G))

    for i in range(len(components) - 1):
        u = next(iter(components[i]))
        v = next(iter(components[i + 1]))
        G.add_edge(u, v)
        
    return G


# Convert NetworkX graph → required JSON format
def graph_to_json_struct(S):
    data = {"nodes": [], "links": []}

    for u in S.nodes():
        data["nodes"].append({
            "id": int(u),
            "color": int(S.nodes[u]['color'])
        })

    for u, v in S.edges():
        data["links"].append({
            "source": int(u),
            "target": int(v)
        })

    return data

# Save JSON to file
def save_json(obj, filename):
    with open(filename, "w") as f:
        json.dump(obj, f, indent=2)
    #print(f"Saved: {filename}")

#create sub-graphs:
def generate_and_save_subgraphs(count, p, sizeL, sizeH, folder, keep_first):
    os.makedirs(folder, exist_ok=True)

    s_list = []  # only the first keep_first
    
    for i in range(1, count+1):
        print(f"Generating S_{i} ...")

        # random size
        k = random.randint(int(sizeL), int(sizeH))

        # generate connected S_i
        S = create_colored_graph(k, p)
        while not nx.is_connected(S):
            print(f"S_{i} not connected — regenerating")
            S = create_colored_graph(k, 10.0/k)

        # save immediately to JSON
        data_S = graph_to_json_struct(S)
        filename = os.path.join(folder, f"S_{i}.json")
        save_json(data_S, filename)

        print(f"S_{i} saved.")

        # keep only the first 100
        if i <= keep_first:
            s_list.append(S)

        # free memory (important!)
        del S

    return s_list

def embed_subgraph(G,S,available_nodes):
    """
    Embed subgraph S into G as a non-induced subgraph.
    Only ensures edges of S exist in G.
    Does NOT remove any extra edges that G already has.
    """     
    nodes_S = list(S.nodes())
    k = len(nodes_S)
    
    if len(available_nodes) < k:
        raise ValueError("NOT ENOUGH UNUSED NODES LEFT TO EMBED S!")

    # random k nodes in G
    nodes_G = random.sample(list(available_nodes), k)

    # map S nodes → G nodes
    mapping = dict(zip(nodes_S, nodes_G))

    # copy colors
    for u_S, u_G in mapping.items():
        G.nodes[u_G]["color"] = S.nodes[u_S]["color"]
        available_nodes.remove(u_G)

    for v_G, u_G, in zip(mapping.values(), mapping.values()):
        if (v_G, u_G) in G.edges():
            G.remove_edge(v_G, u_G)
        if (u_G, v_G) in G.edges():
            G.remove_edge(u_G, v_G)

    # add edges that appear in S
    for u_S, v_S in S.edges():
        u_G = mapping[u_S]
        v_G = mapping[v_S]
        G.add_edge(u_G, v_G)

    return available_nodes


    
# Run
if __name__ == "__main__":
   
    G = create_colored_graph(100,0.1)
    print("done generating G")
    sizeL=5#int(len(G.nodes())*0.005)
    sizeH=10#int(len(G.nodes())*0.02)
    count_S=3
     #embed sub-graphs  
    how_many_to_embed = 1
    print(how_many_to_embed)
    
    s_list=generate_and_save_subgraphs(count_S,0.7,sizeL,sizeH,OUTPUT_DIR,how_many_to_embed)
    print("done generaiing s_list")
    
   
    available_nodes = set(G.nodes())  
    for i in range(how_many_to_embed):
        print(f"embed s number {i+1}")
        available = embed_subgraph(G, s_list[i],available_nodes)
        available_nodes = available
        
    data_G = graph_to_json_struct(G)
    save_json(data_G, f"{OUTPUT_DIR}/G.json")
    

    print("DONE: G and all S_i saved.")
   
   
   