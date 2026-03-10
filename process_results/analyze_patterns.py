#!/usr/bin/env python3
"""
Summarise all CSVs in the pattern_finder process_results/CSV folder.
Two filename formats are handled:
  - g_den_{i}_embedded_den_{j}_{dist}_0.csv   → 4 cols: S_index, pattern_finder, induced_motifs, non_induced_motifs
  - {induced|non_induced}_{dist}_deg_{k}_.csv → 3 cols: S_index, pattern_finder, induced_motifs OR non_induced_motifs

Output:
  - pattern_summary.csv   (one row per input CSV)
  - pattern_summary.png   (grouped bar chart)
"""

import os, glob, re
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# ── Paths ────────────────────────────────────────────────────────────────────
INPUT_DIR  = "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/process_results/CSV"
OUTPUT_CSV = os.path.join(INPUT_DIR, "pattern_summary.csv")
OUTPUT_PNG = os.path.join(INPUT_DIR, "pattern_summary.png")

FLAG_COLS  = ["pattern_finder", "induced_motifs", "non_induced_motifs"]

# ── Collect ──────────────────────────────────────────────────────────────────
csv_files = sorted(glob.glob(os.path.join(INPUT_DIR, "*.csv")))
csv_files = [f for f in csv_files if os.path.basename(f) != "pattern_summary.csv"]

if not csv_files:
    raise FileNotFoundError(f"No CSV files found in: {INPUT_DIR}")

records = []
for fpath in csv_files:
    fname = os.path.basename(fpath)
    df    = pd.read_csv(fpath)

    row     = {"file": fname}
    present = []

    for col in FLAG_COLS:
        if col in df.columns:
            row[col] = int((df[col] == 1).sum())
            present.append(col)
        else:
            row[col] = 0

    # sum_failed: rows where ANY present flag column == 1
    if present:
        row["sum_failed"] = int((df[present] == 1).any(axis=1).sum())
    else:
        row["sum_failed"] = 0

    records.append(row)

summary = pd.DataFrame(records, columns=["file"] + FLAG_COLS + ["sum_failed"])
summary.to_csv(OUTPUT_CSV, index=False)
print(f"Saved summary CSV  ->  {OUTPUT_CSV}")
print(summary.to_string(index=False))

# ── Plot ─────────────────────────────────────────────────────────────────────
n        = len(summary)
x        = np.arange(n)
n_bars   = 4
width    = 0.18
gap      = 0.04
all_cols = FLAG_COLS + ["sum_failed"]

PALETTE = {
    "pattern_finder":     "#60A5FA",
    "induced_motifs":     "#34D399",
    "non_induced_motifs": "#FBBF24",
    "sum_failed":         "#F87171",
}

fig_w = max(16, n * 0.95)
fig, ax = plt.subplots(figsize=(fig_w, 7))
fig.patch.set_facecolor("#0F172A")
ax.set_facecolor("#1E293B")

for k, col in enumerate(all_cols):
    offset = (k - (n_bars - 1) / 2) * (width + gap)
    vals   = summary[col].values
    bars   = ax.bar(
        x + offset, vals,
        width=width, color=PALETTE[col], alpha=0.90, zorder=3, label=col,
        edgecolor="white", linewidth=0.3,
    )
    for bar, v in zip(bars, vals):
        if v > 0:
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                v + 0.4,
                str(v),
                ha="center", va="bottom",
                fontsize=6.5, color="white", fontweight="bold",
            )

# ── Short x-tick labels ──────────────────────────────────────────────────────
def shorten(fname):
    name = fname.replace(".csv", "").replace("_o", "").rstrip("_")
    m = re.match(r"g_den_(\d+)_embedded_den_(\d+)_(uniform|average|rare)_0?", name)
    if m:
        dist_abbr = {"uniform": "uni", "average": "avg", "rare": "rare"}[m.group(3)]
        return f"g{m.group(1)}.e{m.group(2)}.{dist_abbr}"
    m2 = re.match(r"(induced|non_induced)_(uniform|average|rare)_deg_(\d+)", name)
    if m2:
        ind_abbr  = "ind" if m2.group(1) == "induced" else "non"
        dist_abbr = {"uniform": "uni", "average": "avg", "rare": "rare"}[m2.group(2)]
        return f"{ind_abbr}.{dist_abbr}.k{m2.group(3)}"
    return name[:22]

labels = [shorten(f) for f in summary["file"]]
ax.set_xticks(x)
ax.set_xticklabels(labels, rotation=45, ha="right", fontsize=7.5, color="#CBD5E1")

ax.set_ylabel("Count  (failures / not-found)", color="#94A3B8", fontsize=11)
ax.set_title(
    "Pattern Finder - Motif & Failure Summary per Graph",
    color="white", fontsize=14, fontweight="bold", pad=16,
)
ax.tick_params(colors="#94A3B8")
for spine in ax.spines.values():
    spine.set_edgecolor("#334155")
ax.yaxis.set_tick_params(labelcolor="#94A3B8")
ax.set_axisbelow(True)
ax.yaxis.grid(True, color="#334155", linewidth=0.55, linestyle="--")

ax.legend(
    frameon=True, framealpha=0.3,
    facecolor="#1E293B", edgecolor="#475569",
    labelcolor="white", fontsize=9,
    loc="upper right",
)

plt.tight_layout()
plt.savefig(OUTPUT_PNG, dpi=150, bbox_inches="tight")
print(f"Saved plot         ->  {OUTPUT_PNG}")
