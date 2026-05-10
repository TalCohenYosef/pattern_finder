import json
import random
from pathlib import Path

G_PATH = Path("/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/Gnp_graphs/G.json")
S_PATH = Path("/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/Gnp_graphs/S_1.json")
OUT_PATH = Path("/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/Gnp_graphs/G_with_S1_connected.json")

SEED = 42
NUM_BRIDGES = 10  

random.seed(SEED)

with open(G_PATH, "r", encoding="utf-8") as f:
    G = json.load(f)

with open(S_PATH, "r", encoding="utf-8") as f:
    S = json.load(f)

# מזהים קיימים ב-G
g_node_ids = [node["id"] for node in G["nodes"]]
max_g_id = max(g_node_ids)
offset = max_g_id + 1


id_map = {
    node["id"]: node["id"] + offset
    for node in S["nodes"]
}

planted_nodes = [
    {
        "id": id_map[node["id"]],
        "color": node["color"]
    }
    for node in S["nodes"]
]


planted_links = [
    {
        "source": id_map[edge["source"]],
        "target": id_map[edge["target"]]
    }
    for edge in S["links"]
]


planted_node_ids = list(id_map.values())

bridge_links = []
existing_edges = {
    (min(edge["source"], edge["target"]), max(edge["source"], edge["target"]))
    for edge in G["links"]
}


while len(bridge_links) < NUM_BRIDGES:
    u = random.choice(g_node_ids)
    v = random.choice(planted_node_ids)

    edge_key = (min(u, v), max(u, v))

    if edge_key in existing_edges:
        continue

    existing_edges.add(edge_key)
    bridge_links.append({
        "source": u,
        "target": v
    })

G_connected = {
    "nodes": G["nodes"] + planted_nodes,
    "links": G["links"] + planted_links + bridge_links
}

with open(OUT_PATH, "w", encoding="utf-8") as f:
    json.dump(G_connected, f)

print("Created:", OUT_PATH)
print("Original G nodes:", len(G["nodes"]))
print("Original G links:", len(G["links"]))
print("S_1 nodes planted:", len(S["nodes"]))
print("S_1 internal links planted:", len(S["links"]))
print("Bridge links added:", len(bridge_links))
print("New G nodes:", len(G_connected["nodes"]))
print("New G links:", len(G_connected["links"]))
print("Offset used:", offset)

print("\nBridge edges:")
for e in bridge_links:
    print(e)