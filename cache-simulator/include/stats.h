/* Cache access counters and report formatting helpers. */
#ifndef CACHE_SIMULATOR_STATS_H
#define CACHE_SIMULATOR_STATS_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t accesses;
    uint64_t reads;
    uint64_t writes;
    uint64_t hits;
    uint64_t misses;
    uint64_t read_hits;
    uint64_t read_misses;
    uint64_t write_hits;
    uint64_t write_misses;
    uint64_t compulsory_misses;
    uint64_t conflict_capacity_misses;
    uint64_t evictions;
    uint64_t writebacks;
    uint64_t mem_reads;
    uint64_t mem_writes;
} cache_stats_t;

double stats_hit_rate(const cache_stats_t *stats);
double stats_miss_rate(const cache_stats_t *stats);
double stats_amat(const cache_stats_t *stats, uint32_t hit_time,
                  uint32_t miss_penalty);
void stats_print(FILE *stream, const cache_stats_t *stats,
                 uint32_t hit_time, uint32_t miss_penalty);
void stats_print_csv_header(FILE *stream);
void stats_print_csv_row(FILE *stream, const char *workload,
                         uint32_t cache_size, uint32_t block_size,
                         const char *policy, const cache_stats_t *stats,
                         uint32_t hit_time, uint32_t miss_penalty);

#endif
