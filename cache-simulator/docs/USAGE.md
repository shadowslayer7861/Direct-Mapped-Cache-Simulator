# Usage

Build from the project root with PowerShell:

```powershell
.\build.ps1
.\build.ps1 test
.\build.ps1 run
```

The resulting executable is `build/cachesim.exe` on Windows and `build/cachesim` on other platforms. GNU Make is also supported where available with `make`, `make test`, `make run`, and `make clean`.

## Command-line options

Defaults below are from the program's `--help` output. Numeric options accept unsigned decimal values except `--write-fraction`, which accepts a finite fraction in the inclusive range shown by help.

| Option | Meaning | Default |
|---|---|---|
| `--cache-size N` | Cache size in bytes | `4096` |
| `--block-size N` | Block size in bytes | `32` |
| `--addr-bits N` | Address width, validated from `8` through `32` bits | `32` |
| `--write-policy wb\|wt` | Write-back/allocate (`wb`) or write-through/no-allocate (`wt`) | `wb` |
| `--hit-time N` | Hit time in cycles | `1` |
| `--miss-penalty N` | Miss penalty in cycles | `100` |
| `--workload NAME` | `sequential`, `stride`, `random`, `loop`, `matrix-row`, `matrix-col`, `pingpong`, or `trace` | `sequential` |
| `--count N` | Number of generated accesses; trace workloads read to end-of-file | `100000` |
| `--stride N` | Stride workload byte stride | `64` |
| `--range N` | Stride/random workload byte range | `65536` |
| `--loop-bytes N` | Loop working-set size in bytes | `2048` |
| `--matrix-n N` | Matrix dimension; matrix workloads produce its square number of accesses | `128` |
| `--write-fraction F` | Fraction of generated accesses selected as writes | `0` |
| `--seed N` | Deterministic random seed; zero is replaced by a fixed nonzero seed | `1` |
| `--trace FILE` | Trace path; selecting it also selects the trace workload | none |
| `--csv` | Print just the CSV header and one result row | off |
| `--sweep` | Run the built-in workload/cache/block sweep | off |
| `--out FILE` | CSV destination when using CSV or sweep mode | standard output |
| `--verbose` | Print initial accesses with split address fields | off |
| `--help` | Print command help | off |

Cache and block sizes must be powers of two, the block size must be at least the element size, the cache must contain at least one block, and the address layout must leave a tag field. Generated workloads require a positive count. Trace workloads stop at end-of-file; their count is not used to truncate the trace.

## Examples

Run a sequential workload:

```sh
build/cachesim --workload sequential --count 1024 --block-size 32
```

Read and display a trace:

```sh
build/cachesim --trace traces/sample_small.trace --verbose
```

Run the CSV sweep:

```sh
build/cachesim --sweep --out results/sweep.csv
```

The sweep uses workloads `sequential`, `stride`, `random`, `loop`, `matrix-row`, `matrix-col`, and `pingpong`; cache sizes `512`, `1024`, `2048`, `4096`, `8192`, and `16384`; and block sizes `16`, `32`, `64`, and `128`. It uses write-back/allocate and a write fraction of `0.25`.

## Plots

The plotting script reads `results/sweep.csv` and writes the cache-size and block-size hit-rate figures to `docs/images/`. Run it with `python scripts/plot_results.py` after installing matplotlib with `python -m pip install matplotlib`. In the environment used for these results Python was available, but matplotlib was not installed, so plot generation was skipped.
