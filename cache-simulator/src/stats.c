/* Cache statistic calculations and human/CSV report formatting. */
#include "stats.h"

#include <inttypes.h>

double stats_hit_rate(const cache_stats_t *stats)
{
    if (stats == NULL || stats->accesses == 0U) {
        return 0.0;
    }
    return (double)stats->hits / (double)stats->accesses;
}

double stats_miss_rate(const cache_stats_t *stats)
{
    if (stats == NULL || stats->accesses == 0U) {
        return 0.0;
    }
    return (double)stats->misses / (double)stats->accesses;
}

double stats_amat(const cache_stats_t *stats, uint32_t hit_time,
                  uint32_t miss_penalty)
{
    return (double)hit_time + stats_miss_rate(stats) * (double)miss_penalty;
}

void stats_print(FILE *stream, const cache_stats_t *stats,
                 uint32_t hit_time, uint32_t miss_penalty)
{
    double amat;
    double total_cycles;

    if (stream == NULL || stats == NULL) {
        return;
    }
    amat = stats_amat(stats, hit_time, miss_penalty);
    total_cycles = (double)stats->accesses * (double)hit_time +
                   (double)stats->misses * (double)miss_penalty;
    (void)fprintf(stream, "Accesses: %" PRIu64 "\n", stats->accesses);
    (void)fprintf(stream, "Reads: %" PRIu64 ", writes: %" PRIu64 "\n",
                  stats->reads, stats->writes);
    (void)fprintf(stream, "Hits: %" PRIu64 "\n", stats->hits);
    (void)fprintf(stream, "Misses: %" PRIu64 "\n", stats->misses);
    (void)fprintf(stream, "Hit rate: %.2f%%\n", stats_hit_rate(stats) * 100.0);
    (void)fprintf(stream, "Miss rate: %.2f%%\n", stats_miss_rate(stats) * 100.0);
    (void)fprintf(stream,
                  "Compulsory misses: %" PRIu64
                  ", conflict/capacity misses: %" PRIu64 "\n",
                  stats->compulsory_misses, stats->conflict_capacity_misses);
    (void)fprintf(stream, "Evictions: %" PRIu64 "\n", stats->evictions);
    (void)fprintf(stream, "Writebacks: %" PRIu64 "\n", stats->writebacks);
    (void)fprintf(stream, "Memory reads: %" PRIu64 "\n", stats->mem_reads);
    (void)fprintf(stream, "Memory writes: %" PRIu64 "\n", stats->mem_writes);
    (void)fprintf(stream, "AMAT: %.2f cycles\n", amat);
    (void)fprintf(stream, "Total cycles: %.0f\n", total_cycles);
}

void stats_print_csv_header(FILE *stream)
{
    if (stream != NULL) {
        (void)fprintf(stream,
                      "workload,cache_size,block_size,policy,accesses,hits,"
                      "misses,hit_rate,miss_rate,compulsory,conflict_capacity,"
                      "evictions,writebacks,mem_reads,mem_writes,amat\n");
    }
}

void stats_print_csv_row(FILE *stream, const char *workload,
                         uint32_t cache_size, uint32_t block_size,
                         const char *policy, const cache_stats_t *stats,
                         uint32_t hit_time, uint32_t miss_penalty)
{
    if (stream == NULL || workload == NULL || policy == NULL || stats == NULL) {
        return;
    }
    (void)fprintf(stream,
                  "%s,%" PRIu32 ",%" PRIu32 ",%s,%" PRIu64 ",%" PRIu64
                  ",%" PRIu64 ",%.6f,%.6f,%" PRIu64 ",%" PRIu64
                  ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                  ",%.6f\n",
                  workload, cache_size, block_size, policy, stats->accesses,
                  stats->hits, stats->misses, stats_hit_rate(stats),
                  stats_miss_rate(stats), stats->compulsory_misses,
                  stats->conflict_capacity_misses, stats->evictions,
                  stats->writebacks, stats->mem_reads, stats->mem_writes,
                  stats_amat(stats, hit_time, miss_penalty));
}
