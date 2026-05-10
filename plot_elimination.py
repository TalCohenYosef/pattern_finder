import re
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

files = [
    ("3", "/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/subgraph_matches_summary_minus25.txt"),
    ("4", "/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/subgraph_matches_minus30.txt"),
    ("5", "/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/subgraph_matches_minus40_from30.txt"),
    ("6", "/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/subgraph_matches_minus60_from40.txt"),
    ("7", "/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/subgraph_matches_minus80_from60.txt"),
    ]


def load_matches(path):
    data = {}
    pattern = re.compile(r"^(pattern_S_\d+\.json)\s*-\s*matches\s+(\d+)")

    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            m = pattern.match(line)
            if m:
                fname = m.group(1)
                matches = int(m.group(2))
                data[fname] = matches

    return data


print("===== SUMMARY =====")

x_values = [0]
y_remaining = [10000]

alive = None

for label, path in files:
    data = load_matches(path)

    if alive is None:
        checked = data
    else:
        checked = {k: data[k] for k in alive if k in data}

    eliminated_now = {k for k, v in checked.items() if v == 0}
    alive = {k for k, v in checked.items() if v > 0}

    print()
    print(f"Step {label}")
    print(f"Checked: {len(checked)}")
    print(f"Eliminated in this step: {len(eliminated_now)}")
    print(f"Still alive after this step: {len(alive)}")

    x_values.append(int(label))
    y_remaining.append(len(alive))

plt.figure(figsize=(10, 6))
plt.plot(x_values, y_remaining, marker="o", linewidth=2)

# מספר מדויק ליד כל נקודה
for x, y in zip(x_values, y_remaining):
    plt.annotate(
        f"{y}",
        (x, y),
        textcoords="offset points",
        xytext=(0, 8),
        ha="center"
    )

plt.xlim(left=0)
plt.xticks([0, 3, 4, 5, 6, 7, 8])

# ציר Y לוגריתמי-למחצה, תומך גם ב-0
plt.yscale("symlog", linthresh=1)
plt.ylim(0, 10000)

plt.xlabel("Step")
plt.ylabel("Number of S graphs still alive")
plt.title("Elimination Process of Candidate Patterns", pad=20)
plt.grid(True, which="both")
plt.tight_layout()

plt.savefig("/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/elimination_plot_log.png", dpi=200)

print()
print("Plot saved to:")
print("/home/cohent59/Tal/PROJECT_RUN_PATTERN/pattern_finder/elimination_plot_log.png")