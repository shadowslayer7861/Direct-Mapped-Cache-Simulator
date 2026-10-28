"""Plot cache simulator sweep results to PNG files in docs/images."""

import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SWEEP_PATH = ROOT / "results" / "sweep.csv"
IMAGE_DIR = ROOT / "docs" / "images"
REQUIRED_COLUMNS = {"workload", "cache_size", "block_size", "hit_rate"}


def load_rows(path):
    with path.open(newline="", encoding="utf-8") as csv_file:
        reader = csv.DictReader(csv_file)
        if reader.fieldnames is None or not REQUIRED_COLUMNS.issubset(reader.fieldnames):
            raise ValueError(f"{path} is missing required sweep CSV columns")
        rows = list(reader)
    if not rows:
        raise ValueError(f"{path} contains no sweep rows")
    for row_number, row in enumerate(rows, start=2):
        try:
            row["cache_size"] = int(row["cache_size"])
            row["block_size"] = int(row["block_size"])
            row["hit_rate"] = float(row["hit_rate"])
        except (TypeError, ValueError) as error:
            raise ValueError(f"{path}:{row_number}: invalid numeric value") from error
    return rows


def plot_sweep(rows):
    try:
        import matplotlib.pyplot as plt
    except ImportError as error:
        raise SystemExit(
            "matplotlib is required; install it with `python -m pip install matplotlib`"
        ) from error

    IMAGE_DIR.mkdir(parents=True, exist_ok=True)
    workloads = list(dict.fromkeys(row["workload"] for row in rows))

    fig, axis = plt.subplots()
    for workload in workloads:
        values = sorted(
            (row["cache_size"], row["hit_rate"])
            for row in rows
            if row["workload"] == workload and row["block_size"] == 32
        )
        if values:
            axis.plot(
                [value[0] for value in values],
                [value[1] * 100.0 for value in values],
                marker="o",
                label=workload,
            )
    axis.set(
        title="Hit rate vs. cache size (32-byte blocks)",
        xlabel="Cache size (bytes)",
        ylabel="Hit rate (%)",
    )
    axis.grid(True, alpha=0.3)
    axis.legend()
    fig.tight_layout()
    fig.savefig(IMAGE_DIR / "hit_rate_vs_cache_size.png", dpi=150)
    plt.close(fig)

    fig, axis = plt.subplots()
    for workload in workloads:
        values = sorted(
            (row["block_size"], row["hit_rate"])
            for row in rows
            if row["workload"] == workload and row["cache_size"] == 4096
        )
        if values:
            axis.plot(
                [value[0] for value in values],
                [value[1] * 100.0 for value in values],
                marker="o",
                label=workload,
            )
    axis.set(
        title="Hit rate vs. block size (4 KB cache)",
        xlabel="Block size (bytes)",
        ylabel="Hit rate (%)",
    )
    axis.grid(True, alpha=0.3)
    axis.legend()
    fig.tight_layout()
    fig.savefig(IMAGE_DIR / "hit_rate_vs_block_size.png", dpi=150)
    plt.close(fig)


def main():
    rows = load_rows(SWEEP_PATH)
    plot_sweep(rows)
    print(f"Plots written to {IMAGE_DIR}")


if __name__ == "__main__":
    main()
