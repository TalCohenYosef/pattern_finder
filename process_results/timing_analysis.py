#!/usr/bin/env python3
"""
Timing comparison: pattern_finder vs motif_induced vs motif_non_induced.

Timing semantics (as specified):
  - pattern_finder:
        preprocessing_s = "Total pattern finding time" from PATTERN_SUMMARY.log (per folder, shared by all graphs in that folder)
        search_s        = "Total subgraph testing time" from RESULTS_<graph>.log  (per graph)
        total_s         = preprocessing_s + search_s

  - motif_induced / motif_non_induced:
        search_s        = G_compute_time   (runtime without preprocessing)
        preprocessing_s = TOTAL_TIME (or TOTAL_BATCH_TIME) - G_compute_time
        total_s         = TOTAL_TIME / TOTAL_BATCH_TIME

Graph naming is unified so all three algorithms share the same "graph" key:
  - g_den graphs  : "g_den_{i}_embedded_den_{k}_{dist}_0"
  - special graphs : "special_{dist}_k{k}"    (G_induced + G_non_induced merged per folder context)
  - NCI graphs    : "DHFR-MD", "Mutagenicity"
"""

import os, re, glob
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# ── Paths ─────────────────────────────────────────────────────────────────────
PF_BASE  = "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results"
IND_DIR  = "/home/cohent59/new-graph-measures/local_tests/induced/logs/compare_results"
NIND_DIR = "/home/cohent59/new-graph-measures/local_tests/non_induced/logs/compare_results"
OUT_DIR  = "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/process_results"
os.makedirs(OUT_DIR, exist_ok=True)

OUTPUT_CSV = os.path.join(OUT_DIR, "timing_summary.csv")
OUTPUT_PNG = os.path.join(OUT_DIR, "timing_plots.png")

# ── Helpers ───────────────────────────────────────────────────────────────────
def read_file(path):
    if os.path.exists(path):
        with open(path) as f:
            return f.read()
    print(f"  [WARN] file not found: {path}")
    return ""

def safe_float(s):
    try:    return float(s)
    except: return None

# ── 1. PATTERN FINDER ─────────────────────────────────────────────────────────
DISTS   = ["uniform", "average", "rare"]
G_DEN_3 = [(i, 3, d) for i in [5, 8, 10, 13, 15] for d in DISTS]
G_DEN_5 = [(i, 5, d) for i in [8, 10, 13, 15]    for d in DISTS]
NCI_GS  = ["DHFR-MD", "Mutagenicity"]
SPECIAL_GRAPHS = ["G_induced", "G_non_induced"]

def pf_folder(k, dist):
    if dist is None:
        return os.path.join(PF_BASE, "OUTPUT_NCI109")
    return os.path.join(PF_BASE, f"OUTPUT_input_color_{dist}_deg_{k}")

def pf_preprocessing(folder):
    """Folder-level preprocessing time from PATTERN_SUMMARY.log"""
    content = read_file(os.path.join(folder, "PATTERN_SUMMARY.log"))
    m = re.search(r"Total pattern finding time:\s*([\d.]+)\s*seconds", content)
    return safe_float(m.group(1)) if m else None

def pf_search(folder, graph_name):
    """Per-graph search time from RESULTS_<graph_name>.log"""
    content = read_file(os.path.join(folder, f"RESULTS_{graph_name}.log"))
    m = re.search(r"Total subgraph testing time:\s*([\d.]+)\s*seconds", content)
    return safe_float(m.group(1)) if m else None

pf_rows = []

# g_den graphs — unified graph name matches motif log labels exactly
for (i, k, dist) in G_DEN_3 + G_DEN_5:
    gname  = f"g_den_{i}_embedded_den_{k}_{dist}_0"
    folder = pf_folder(k, dist)
    pre    = pf_preprocessing(folder)
    srch   = pf_search(folder, gname)
    total  = (pre + srch) if (pre is not None and srch is not None) else None
    pf_rows.append(dict(graph=gname, algorithm="pattern_finder",
                        preprocessing_s=pre, search_s=srch, total_s=total))

# Special graphs (G_induced + G_non_induced) — one unified context key per (dist, k)
# PF preprocessing is the same for both G_induced and G_non_induced in the same folder.
# We average (or sum) their search times under one context row.
for dist in DISTS:
    for k in [3, 5, 8, 15]:
        context = f"special_{dist}_k{k}"
        folder  = pf_folder(k, dist)
        pre     = pf_preprocessing(folder)
        # Collect search times for both G_induced and G_non_induced
        search_times = []
        for sg in SPECIAL_GRAPHS:
            s = pf_search(folder, sg)
            if s is not None:
                search_times.append(s)
        # Use average search time (each is a separate G test)
        if search_times:
            srch  = sum(search_times) / len(search_times)
            total = (pre + srch) if pre is not None else None
        else:
            srch, total = None, None
        pf_rows.append(dict(graph=context, algorithm="pattern_finder",
                            preprocessing_s=pre, search_s=srch, total_s=total))

# NCI graphs
nci_folder = pf_folder(None, None)
nci_pre    = pf_preprocessing(nci_folder)
for gname in NCI_GS:
    srch  = pf_search(nci_folder, gname)
    total = (nci_pre + srch) if (nci_pre is not None and srch is not None) else None
    pf_rows.append(dict(graph=gname, algorithm="pattern_finder",
                        preprocessing_s=nci_pre, search_s=srch, total_s=total))

# ── 2. MOTIF ALGORITHMS ───────────────────────────────────────────────────────
# Unified regex handles TOTAL_TIME (equal_degs) and TOTAL_BATCH_TIME (times_3/5)
MOTIF_RE = re.compile(
    r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d+ - (.+?) \| '
    r'(?:(G_compute_time)|(TOTAL_TIME|TOTAL_BATCH_TIME))=([\d.]+)s'
)

def parse_motif_log(path):
    records = {}
    for m in MOTIF_RE.finditer(read_file(path)):
        label = m.group(1).strip()
        records.setdefault(label, {})
        if m.group(2): records[label]["G_compute"] = float(m.group(4))
        if m.group(3): records[label]["total"]     = float(m.group(4))
    return records

def motif_rows_from_records(records, algo, label_to_graph):
    rows = []
    for label, times in records.items():
        gc    = times.get("G_compute")
        total = times.get("total")
        pre   = round(total - gc, 6) if (gc is not None and total is not None) else None
        gname = label_to_graph(label)
        rows.append(dict(graph=gname, algorithm=algo,
                         preprocessing_s=pre, search_s=gc, total_s=total))
    return rows

def eq_label_to_graph(label):
    # "color_rare_deg_5" -> "special_rare_k5"  (matches PF special context key)
    m = re.match(r"color_(\w+)_deg_(\d+)", label)
    return f"special_{m.group(1)}_k{m.group(2)}" if m else label

motif_rows = []

for algo, base_dir, suffix in [
    ("motif_induced",     IND_DIR,  "induced_motifs"),
    ("motif_non_induced", NIND_DIR, "non_induced_motifs"),
]:
    for fname, gfn in [
        (f"equal_degs_times_{suffix}.log", eq_label_to_graph),  # special graphs
        (f"times_3_{suffix}.log",           lambda l: l),        # g_den k=3
        (f"times_5_{suffix}.log",           lambda l: l),        # g_den k=5
    ]:
        fpath = os.path.join(base_dir, fname)
        if os.path.exists(fpath):
            motif_rows += motif_rows_from_records(parse_motif_log(fpath), algo, gfn)
        else:
            print(f"  [WARN] not found: {fpath}")

# ── 3. Build & save CSV ───────────────────────────────────────────────────────
df = pd.DataFrame(pf_rows + motif_rows,
                  columns=["graph","algorithm","preprocessing_s","search_s","total_s"])
for c in ["preprocessing_s","search_s","total_s"]:
    df[c] = pd.to_numeric(df[c], errors="coerce").round(3)

df.to_csv(OUTPUT_CSV, index=False)
print(f"\nSaved CSV -> {OUTPUT_CSV}")
# Show only rows with at least one non-null time
print(df[df[["preprocessing_s","search_s","total_s"]].notna().any(axis=1)].to_string(index=False))

# ── 4. PLOTS ──────────────────────────────────────────────────────────────────
BG  = "#0F172A"
PAN = "#1E293B"
GRD = "#334155"
TIC = "#94A3B8"
WHT = "#F1F5F9"

ALGO_COLORS = {
    "pattern_finder":    "#60A5FA",
    "motif_induced":     "#34D399",
    "motif_non_induced": "#FBBF24",
}
ALGOS = list(ALGO_COLORS.keys())

def style_ax(ax, title, ylabel="Seconds"):
    ax.set_facecolor(PAN)
    ax.tick_params(colors=TIC)
    for sp in ax.spines.values(): sp.set_edgecolor(GRD)
    ax.yaxis.set_tick_params(labelcolor=TIC)
    ax.set_axisbelow(True)
    ax.yaxis.grid(True, color=GRD, linewidth=0.5, linestyle="--")
    ax.set_ylabel(ylabel, color=TIC, fontsize=9)
    ax.set_title(title, color=WHT, fontsize=11, fontweight="bold", pad=9)

def shorten(s):
    s = str(s)
    for a, b in [("g_den_","g"), ("_embedded_den_","e"), ("_0",""),
                 ("uniform","uni"), ("average","avg"), ("special_","")]:
        s = s.replace(a, b)
    return s[:22]

def grouped_bar_panel(ax, labels, val_dict, title):
    """Draw grouped bars. val_dict = {algo: [val_or_None, ...]}"""
    n  = len(labels)
    na = len(ALGOS)
    x  = np.arange(n)
    w  = 0.68 / na

    style_ax(ax, title)
    for ki, algo in enumerate(ALGOS):
        vals   = [v if (v is not None and not np.isnan(v)) else 0 for v in val_dict[algo]]
        offset = (ki - (na - 1) / 2) * (w + 0.025)
        ax.bar(x + offset, vals, width=w,
               color=ALGO_COLORS[algo], alpha=0.88, zorder=3,
               edgecolor="white", linewidth=0.2, label=algo)

    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=40, ha="right", fontsize=7.5, color=TIC)

def build_pivot(metric):
    """
    Returns (ordered_graphs, {algo: [val, ...]}) for graphs where
    at least 2 algorithms have a non-null value for `metric`.
    """
    pivot = (df.dropna(subset=[metric])
               .groupby(["graph","algorithm"])[metric]
               .first()
               .unstack("algorithm"))
    # Keep only rows with >= 2 algorithms present
    valid = pivot[pivot.notna().sum(axis=1) >= 2]
    graphs = list(valid.index)
    vals = {a: [valid.loc[g, a] if a in valid.columns and not pd.isna(valid.loc[g, a]) else None
                for g in graphs]
            for a in ALGOS}
    return graphs, vals

# Build pivot tables for each metric
pre_graphs,    pre_vals    = build_pivot("preprocessing_s")
search_graphs, search_vals = build_pivot("search_s")
total_graphs,  total_vals  = build_pivot("total_s")

pre_labels    = [shorten(g) for g in pre_graphs]
search_labels = [shorten(g) for g in search_graphs]
total_labels  = [shorten(g) for g in total_graphs]

# ── Draw: 3 separate panels ───────────────────────────────────────────────────
fig = plt.figure(figsize=(26, 20), facecolor=BG)
gs  = fig.add_gridspec(3, 1, hspace=0.60)
axA = fig.add_subplot(gs[0])
axB = fig.add_subplot(gs[1])
axC = fig.add_subplot(gs[2])

grouped_bar_panel(axA, pre_labels,    pre_vals,
                  "A — Preprocessing Time  (pattern building / motif index per graph-set)")
grouped_bar_panel(axB, search_labels, search_vals,
                  "B — Search / Runtime  (G computation only, without preprocessing)  per Graph")
grouped_bar_panel(axC, total_labels,  total_vals,
                  "C — Total Time  (preprocessing + search)  per Graph")

# Legend
handles = [plt.Rectangle((0,0),1,1, color=ALGO_COLORS[a], alpha=0.88) for a in ALGOS]
axA.legend(handles, ALGOS, frameon=True, framealpha=0.35,
           facecolor=PAN, edgecolor=GRD, labelcolor="white",
           fontsize=9, loc="upper right")

fig.suptitle(
    "Algorithm Timing Comparison — Pattern Finder  vs  Motif Induced  vs  Motif Non-Induced",
    color=WHT, fontsize=14, fontweight="bold", y=0.998)

plt.savefig(OUTPUT_PNG, dpi=150, bbox_inches="tight", facecolor=BG)
print(f"\nSaved plot -> {OUTPUT_PNG}")
