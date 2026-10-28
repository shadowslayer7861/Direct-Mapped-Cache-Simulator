# Cache Simulator

Cache Simulator is a C11 command-line tool that streams generated or traced memory accesses through a configurable direct-mapped cache and reports hit/miss behavior, memory traffic, and average access time. It supports write-back/write-allocate and write-through/no-write-allocate policies, deterministic workloads, trace input, and CSV sweeps; it uses no external C libraries.

Status: implementation complete; further development may continue.

## Features

- Sequential, stride, deterministic random, loop, row/column matrix, ping-pong, and trace workloads.
- Configurable cache, block, address width, timing, write policy, and workload parameters.
- Compulsory and conflict/capacity miss classification with eviction, writeback, and memory operation counts.
- Streaming generators and a trace reader; accesses are not stored in a workload-sized array.
- Human-readable reports, single-run CSV, and a full CSV parameter sweep.

## Build

With GCC and PowerShell:

```powershell
.\build.ps1
.\build.ps1 test
```

With GNU Make:

```sh
make
make test
```

Both build the simulator at `build/cachesim` (`build/cachesim.exe` on Windows). The Makefile is optional; the PowerShell build does not depend on it.

## Usage

```sh
build/cachesim --workload sequential --count 1024 --block-size 32
build/cachesim --trace traces/sample_small.trace --verbose
build/cachesim --sweep --out results/sweep.csv
```

See [docs/USAGE.md](./docs/USAGE.md) for all options and defaults.

## Example Output

Output from `build/cachesim --workload sequential --count 1024 --block-size 32`:

```text
Configuration: direct-mapped cache
Cache size: 4096 bytes
Block size: 32 bytes
Lines: 128
Address width: 32 bits
Offset/index/tag bits: 5/7/20
Write policy: write-back + write-allocate
Hit time: 1 cycles; miss penalty: 100 cycles
Accesses: 1024
Reads: 1024, writes: 0
Hits: 896
Misses: 128
Hit rate: 87.50%
Miss rate: 12.50%
Compulsory misses: 128, conflict/capacity misses: 0
Evictions: 0
Writebacks: 0
Memory reads: 128
Memory writes: 0
AMAT: 13.50 cycles
Total cycles: 13824
```

## Results

Measured hit rates from the generated sweep at a 4096-byte cache and 32-byte blocks:

| Workload | Hit rate |
|---|---:|
| sequential | 87.500% |
| stride | 0.000% |
| random | 6.223% |
| loop | 99.936% |
| matrix-row | 87.500% |
| matrix-col | 0.000% |
| pingpong | 0.000% |

Recorded single runs:

| Run | Accesses | Hits | Misses | Compulsory | Conflict/capacity | Hit rate | AMAT |
|---|---:|---:|---:|---:|---:|---:|---:|
| sequential, default cache/block, count 1024 | 1024 | 896 | 128 | 128 | 0 | 87.50% | 13.50 cycles |
| matrix-row, cache 4096, block 32, N 128 | 16384 | 14336 | 2048 | 2048 | 0 | 87.50% | 13.50 cycles |
| matrix-col, cache 4096, block 32, N 128 | 16384 | 0 | 16384 | 2048 | 14336 | 0.00% | 101.00 cycles |
| pingpong, cache 1024, count 100 | 100 | 0 | 100 | 2 | 98 | 0.00% | 101.00 cycles |

The full sweep tables and observations are in [docs/RESULTS.md](./docs/RESULTS.md). The plotting script writes `docs/images/hit_rate_vs_cache_size.png` and `docs/images/hit_rate_vs_block_size.png`. Matplotlib is not installed in the current Python environment, so these images were not generated; see the plotting instructions in [docs/USAGE.md](./docs/USAGE.md).

## Design

Addresses are divided into tag, index, and offset fields using the cache layout. The simulator uses a direct-mapped line array and a dynamically growing seen-block set for miss classification. Workloads generate each access on demand. See [docs/DESIGN.md](./docs/DESIGN.md) for the address example, access flow, and timing model.

## Limitations

- Single cache level.
- No data storage; only tags and cache-line state are simulated.
- AMAT ignores write-buffer effects.
- Replacement in a direct-mapped cache is conflict eviction only.
- No associativity.
