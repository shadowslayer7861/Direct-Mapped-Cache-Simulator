# Results

These tables were generated from `results/sweep.csv` produced by `build/cachesim --sweep --out results/sweep.csv`. The sweep uses the seven built-in generated workloads, six cache sizes, four block sizes, the default count and workload parameters, write-back/allocate, and the sweep write fraction of `0.25`. Hit rates are displayed as percentages from the CSV fractions; AMAT values are in cycles.

## Hit rate by workload and cache size

Block size is fixed at 32 bytes.

| Workload | 512 bytes | 1024 bytes | 2048 bytes | 4096 bytes | 8192 bytes | 16384 bytes |
|---|---:|---:|---:|---:|---:|---:|
| sequential | 87.500% | 87.500% | 87.500% | 87.500% | 87.500% | 87.500% |
| stride | 0.000% | 0.000% | 0.000% | 0.000% | 0.000% | 0.000% |
| random | 0.813% | 1.604% | 3.127% | 6.223% | 12.427% | 24.792% |
| loop | 87.500% | 87.500% | 99.936% | 99.936% | 99.936% | 99.936% |
| matrix-row | 87.500% | 87.500% | 87.500% | 87.500% | 87.500% | 87.500% |
| matrix-col | 0.000% | 0.000% | 0.000% | 0.000% | 0.000% | 0.000% |
| pingpong | 0.000% | 0.000% | 0.000% | 0.000% | 0.000% | 0.000% |

Observations from these measurements:

- The random workload's hit rate increases at every cache-size step, from 0.813% to 24.792%.
- The loop workload improves from 87.500% at 512 and 1024 bytes to 99.936% at cache sizes of 2048 bytes and above.
- Sequential and row-major matrix accesses remain at 87.500%; stride, column-major matrix, and ping-pong remain at 0.000% for every listed cache size. The latter patterns repeatedly revisit no resident line under these direct-mapped access patterns.

## Hit rate by workload and block size

Cache size is fixed at 4096 bytes.

| Workload | 16 bytes | 32 bytes | 64 bytes | 128 bytes |
|---|---:|---:|---:|---:|
| sequential | 75.000% | 87.500% | 93.750% | 96.875% |
| stride | 0.000% | 0.000% | 0.000% | 50.000% |
| random | 6.185% | 6.223% | 6.259% | 6.180% |
| loop | 99.872% | 99.936% | 99.968% | 99.984% |
| matrix-row | 75.000% | 87.500% | 93.750% | 96.875% |
| matrix-col | 0.000% | 0.000% | 0.000% | 0.000% |
| pingpong | 0.000% | 0.000% | 0.000% | 0.000% |

Observations from these measurements:

- Sequential and row-major matrix hit rates rise with each larger block size, from 75.000% to 96.875%.
- The 64-byte stride workload has a 50.000% hit rate with 128-byte blocks, while its smaller-block rates are 0.000%; a 128-byte block contains each pair of successive stride addresses.
- Random hit rates remain between 6.180% and 6.259% across these block sizes. The column-major and ping-pong workloads are at 0.000% for each tested size.

## AMAT by workload

Cache size is fixed at 4096 bytes and block size at 32 bytes. The default hit time and miss penalty are used.

| Workload | Hit rate | Miss rate | AMAT (cycles) |
|---|---:|---:|---:|
| sequential | 87.500% | 12.500% | 13.500 |
| stride | 0.000% | 100.000% | 101.000 |
| random | 6.223% | 93.777% | 94.777 |
| loop | 99.936% | 0.064% | 1.064 |
| matrix-row | 87.500% | 12.500% | 13.500 |
| matrix-col | 0.000% | 100.000% | 101.000 |
| pingpong | 0.000% | 100.000% | 101.000 |

Observations from these measurements:

- The loop's 99.936% hit rate corresponds to the lowest measured AMAT, 1.064 cycles.
- Sequential and row-major matrix produce equal measured hit rates and equal AMAT values in this configuration.
- Stride, column-major matrix, and ping-pong have 100.000% miss rates and 101.000-cycle AMAT under this configuration.

## Recorded single runs

The README's sequential output is the run with `--workload sequential --count 1024 --block-size 32`. The matrix comparison used `--cache-size 4096 --block-size 32 --matrix-n 128`: row-major reported 87.50% hits over 16384 accesses, while column-major reported 0.00%. Ping-pong used `--workload pingpong --count 100 --cache-size 1024` and reported 0 hits, 100 misses, 2 compulsory misses, and 98 conflict/capacity misses.

The plotting script is implemented, but the PNGs were not generated because matplotlib is not installed in the available Python environment.
