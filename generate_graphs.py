import os
import json
import random
import argparse
import multiprocessing as mp
from pathlib import Path


def make_color_probs(num_colors=20):
    """
    Almost-uniform distribution over 20 colors:
    most colors are close to equal probability,
    with a few slightly more common and a few slightly rarer.
    """
    weights = [
        1.10, 1.08, 1.06, 1.04, 1.02,
        1.00, 1.00, 1.00, 1.00, 1.00,
        1.00, 1.00, 0.98, 0.96, 0.94,
        0.92, 0.90, 0.88, 0.86, 0.84
    ]

    s = sum(weights)
    return [w / s for w in weights]

def sample_colors(n, probs):
    colors = list(range(len(probs)))
    return random.choices(colors, weights=probs, k=n)


def generate_connected_graph(n, avg_degree_min=5, avg_degree_max=6):
    """
    Creates a connected undirected graph.

    Step 1: random spanning tree => guarantees connectedness.
    Step 2: add random edges until average degree is in [4,5].
    """
    edges = set()

    # Connected backbone: random tree
    for v in range(1, n):
        u = random.randint(0, v - 1)
        edges.add((min(u, v), max(u, v)))

    min_edges = int((avg_degree_min * n) / 2)
    max_edges = int((avg_degree_max * n) / 2)

    target_edges = random.randint(min_edges, max_edges)

    while len(edges) < target_edges:
        u = random.randint(0, n - 1)
        v = random.randint(0, n - 1)

        if u == v:
            continue

        a, b = min(u, v), max(u, v)
        edges.add((a, b))

    return list(edges)


def save_graph(graph_id, output_dir, n_min, n_max, probs, seed):
    random.seed(seed + graph_id)

    n = random.randint(n_min, n_max)
    colors = sample_colors(n, probs)
    edges = generate_connected_graph(n)

    avg_degree = 2 * len(edges) / n

    nodes = [
    {"id": i, "color": colors[i]}
    for i in range(n)
    ]

    edges_formatted = [
        {"source": u, "target": v}
        for u, v in edges
    ]

    graph_data = {
        "nodes": nodes,
        "links": edges_formatted
    }

    out_path = output_dir / "G.json"

    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(graph_data, f)

    return graph_id, n, len(edges), avg_degree


def worker(args):
    return save_graph(*args)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--num-graphs", type=int, default=100_000)
    parser.add_argument("--workers", type=int, default=100)
    parser.add_argument("--n-min", type=int, default=1500)
    parser.add_argument("--n-max", type=int, default=3000)
    parser.add_argument("--output-dir", type=str, required=True)
    parser.add_argument("--seed", type=int, default=42)

    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    probs = make_color_probs(num_colors=20)

    print("Starting graph generation")
    print(f"Graphs     : {args.num_graphs}")
    print(f"Workers    : {args.workers}")
    print(f"N range    : [{args.n_min}, {args.n_max}]")
    print(f"Colors     : 20 non-uniform colors")
    print(f"Avg degree : 4-5")
    print(f"Connected  : yes")
    print(f"Output dir : {output_dir}")
    print()

    tasks = [
        (
            graph_id,
            output_dir,
            args.n_min,
            args.n_max,
            probs,
            args.seed
        )
        for graph_id in range(args.num_graphs)
    ]

    with mp.Pool(processes=args.workers) as pool:
        for i, result in enumerate(pool.imap_unordered(worker, tasks), start=1):
            if i % 1000 == 0:
                graph_id, n, m, avg_degree = result
                print(
                    f"{i}/{args.num_graphs} done | "
                    f"last graph: id={graph_id}, n={n}, edges={m}, avg_degree={avg_degree:.3f}",
                    flush=True
                )

    print("\nFinished successfully.")


if __name__ == "__main__":
    main()