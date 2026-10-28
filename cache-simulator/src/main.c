/* Command-line interface for running and sweeping cache simulations. */
#include "cache.h"
#include "stats.h"
#include "workload.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    cache_config_t cache;
    workload_params_t workload;
    bool csv;
    bool sweep;
    bool verbose;
    bool help;
    const char *out_path;
    bool workload_explicit;
} options_t;

static void print_usage(FILE *stream, const char *program)
{
    (void)fprintf(stream,
        "Usage: %s [options]\n"
        "  --cache-size N       Cache size in bytes (default 4096)\n"
        "  --block-size N       Block size in bytes (default 32)\n"
        "  --addr-bits N        Address width, 8..32 (default 32)\n"
        "  --write-policy wb|wt Write-back/allocate or write-through/no-allocate (default wb)\n"
        "  --hit-time N         Hit time in cycles (default 1)\n"
        "  --miss-penalty N     Miss penalty in cycles (default 100)\n"
        "  --workload NAME      sequential, stride, random, loop, matrix-row,\n"
        "                       matrix-col, pingpong, or trace (default sequential)\n"
        "  --count N            Number of generated accesses (default 100000)\n"
        "  --stride N           Stride workload byte stride (default 64)\n"
        "  --range N            Stride/random byte range (default 65536)\n"
        "  --loop-bytes N       Loop working-set size (default 2048)\n"
        "  --matrix-n N         Matrix dimension (default 128)\n"
        "  --write-fraction F   Generated write fraction, 0..1 (default 0)\n"
        "  --seed N             Xorshift seed (default 1; zero uses fixed seed)\n"
        "  --trace FILE         Trace path (default none; selects trace workload)\n"
        "  --csv                Print only CSV header and result row (default off)\n"
        "  --sweep              Run the fixed workload/cache/block sweep (default off)\n"
        "  --out FILE           Write CSV output to FILE (default stdout)\n"
        "  --verbose            Print the first 20 accesses (default off)\n"
        "  --help               Show this help (default off)\n",
        program);
}

static bool parse_u32(const char *text, uint32_t *value)
{
    char *end;
    unsigned long long parsed;

    if (text == NULL || text[0] == '\0' || text[0] == '-') {
        return false;
    }
    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE ||
        parsed > UINT32_MAX) {
        return false;
    }
    *value = (uint32_t)parsed;
    return true;
}

static bool parse_fraction(const char *text, double *value)
{
    char *end;
    double parsed;

    if (text == NULL || text[0] == '\0') {
        return false;
    }
    errno = 0;
    parsed = strtod(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE ||
        parsed != parsed || parsed < 0.0 || parsed > 1.0) {
        return false;
    }
    *value = parsed;
    return true;
}

static bool option_value(int argc, char **argv, int *index,
                         const char **value)
{
    if (*index + 1 >= argc) {
        return false;
    }
    ++(*index);
    *value = argv[*index];
    return true;
}

static bool parse_options(int argc, char **argv, options_t *options,
                          char *err, size_t errlen)
{
    int i;

    memset(options, 0, sizeof(*options));
    cache_config_defaults(&options->cache);
    options->workload.kind = WL_SEQUENTIAL;
    options->workload.count = 100000U;
    options->workload.stride = 64U;
    options->workload.range = 65536U;
    options->workload.seed = 1U;
    options->workload.loop_bytes = 2048U;
    options->workload.matrix_n = 128U;
    options->workload.write_fraction = 0.0;
    options->workload.cache_size = options->cache.cache_size;

    for (i = 1; i < argc; ++i) {
        const char *value;

        if (strcmp(argv[i], "--help") == 0) {
            options->help = true;
            continue;
        }
        if (strcmp(argv[i], "--csv") == 0) {
            options->csv = true;
            continue;
        }
        if (strcmp(argv[i], "--sweep") == 0) {
            options->sweep = true;
            continue;
        }
        if (strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
            continue;
        }
        if (strcmp(argv[i], "--cache-size") == 0 ||
            strcmp(argv[i], "--block-size") == 0 ||
            strcmp(argv[i], "--addr-bits") == 0 ||
            strcmp(argv[i], "--hit-time") == 0 ||
            strcmp(argv[i], "--miss-penalty") == 0 ||
            strcmp(argv[i], "--count") == 0 ||
            strcmp(argv[i], "--stride") == 0 ||
            strcmp(argv[i], "--range") == 0 ||
            strcmp(argv[i], "--loop-bytes") == 0 ||
            strcmp(argv[i], "--matrix-n") == 0 ||
            strcmp(argv[i], "--seed") == 0) {
            const char *option = argv[i];
            uint32_t parsed;

            if (!option_value(argc, argv, &i, &value) ||
                !parse_u32(value, &parsed)) {
                (void)snprintf(err, errlen, "invalid or missing value for %s", option);
                return false;
            }
            if (strcmp(option, "--cache-size") == 0) {
                options->cache.cache_size = parsed;
            } else if (strcmp(option, "--block-size") == 0) {
                options->cache.block_size = parsed;
            } else if (strcmp(option, "--addr-bits") == 0) {
                options->cache.addr_bits = parsed;
            } else if (strcmp(option, "--hit-time") == 0) {
                options->cache.hit_time = parsed;
            } else if (strcmp(option, "--miss-penalty") == 0) {
                options->cache.miss_penalty = parsed;
            } else if (strcmp(option, "--count") == 0) {
                options->workload.count = parsed;
            } else if (strcmp(option, "--stride") == 0) {
                options->workload.stride = parsed;
            } else if (strcmp(option, "--range") == 0) {
                options->workload.range = parsed;
            } else if (strcmp(option, "--loop-bytes") == 0) {
                options->workload.loop_bytes = parsed;
            } else if (strcmp(option, "--matrix-n") == 0) {
                options->workload.matrix_n = parsed;
            } else if (strcmp(option, "--seed") == 0) {
                options->workload.seed = parsed;
            }
            continue;
        }
        if (strcmp(argv[i], "--write-policy") == 0) {
            if (!option_value(argc, argv, &i, &value)) {
                (void)snprintf(err, errlen, "missing value for --write-policy");
                return false;
            }
            if (strcmp(value, "wb") == 0) {
                options->cache.policy = WRITE_BACK_ALLOCATE;
            } else if (strcmp(value, "wt") == 0) {
                options->cache.policy = WRITE_THROUGH_NO_ALLOCATE;
            } else {
                (void)snprintf(err, errlen,
                               "invalid write policy '%s' (expected wb or wt)", value);
                return false;
            }
            continue;
        }
        if (strcmp(argv[i], "--workload") == 0) {
            if (!option_value(argc, argv, &i, &value) ||
                !workload_parse_kind(value, &options->workload.kind)) {
                (void)snprintf(err, errlen, "invalid or missing workload name");
                return false;
            }
            options->workload_explicit = true;
            continue;
        }
        if (strcmp(argv[i], "--write-fraction") == 0) {
            if (!option_value(argc, argv, &i, &value) ||
                !parse_fraction(value, &options->workload.write_fraction)) {
                (void)snprintf(err, errlen,
                               "write fraction must be a number between 0 and 1");
                return false;
            }
            continue;
        }
        if (strcmp(argv[i], "--trace") == 0) {
            if (!option_value(argc, argv, &i, &options->workload.trace_path)) {
                (void)snprintf(err, errlen, "missing value for --trace");
                return false;
            }
            continue;
        }
        if (strcmp(argv[i], "--out") == 0) {
            if (!option_value(argc, argv, &i, &options->out_path)) {
                (void)snprintf(err, errlen, "missing value for --out");
                return false;
            }
            continue;
        }
        (void)snprintf(err, errlen, "unknown option '%s'", argv[i]);
        return false;
    }

    if (options->workload.trace_path != NULL) {
        if (options->workload_explicit &&
            options->workload.kind != WL_TRACE) {
            (void)snprintf(err, errlen,
                           "--trace cannot be combined with a non-trace workload");
            return false;
        }
        options->workload.kind = WL_TRACE;
    }
    if (options->workload.kind == WL_TRACE &&
        options->workload.trace_path == NULL) {
        (void)snprintf(err, errlen, "trace workload requires --trace FILE");
        return false;
    }
    options->workload.cache_size = options->cache.cache_size;
    if (options->workload.count == 0U) {
        (void)snprintf(err, errlen, "--count must be greater than zero");
        return false;
    }
    if (options->out_path != NULL && !options->csv && !options->sweep) {
        (void)snprintf(err, errlen, "--out requires --csv or --sweep");
        return false;
    }
    if (options->sweep && options->workload.kind == WL_TRACE) {
        (void)snprintf(err, errlen, "--sweep does not include trace workloads");
        return false;
    }
    return true;
}

static int validate_configuration(const options_t *options)
{
    addr_layout_t layout;
    char err[160];

    if (addr_layout_init(&layout, options->cache.cache_size,
                         options->cache.block_size, options->cache.addr_bits,
                         err, sizeof(err)) != 0) {
        (void)fprintf(stderr, "configuration error: %s\n", err);
        return 2;
    }
    if ((options->sweep || options->workload.kind == WL_STRIDE) &&
        options->workload.range == 0U) {
        (void)fprintf(stderr, "configuration error: stride range must be positive\n");
        return 2;
    }
    if ((options->sweep || options->workload.kind == WL_RANDOM) &&
        options->workload.range < 4U) {
        (void)fprintf(stderr, "configuration error: random range must be at least 4\n");
        return 2;
    }
    if ((options->sweep || options->workload.kind == WL_LOOP) &&
        (options->workload.loop_bytes < 4U ||
         options->workload.loop_bytes % 4U != 0U)) {
        (void)fprintf(stderr,
                      "configuration error: loop size must be a positive multiple of 4\n");
        return 2;
    }
    if ((options->sweep || options->workload.kind == WL_MATRIX_ROW ||
         options->workload.kind == WL_MATRIX_COL) &&
        options->workload.matrix_n == 0U) {
        (void)fprintf(stderr, "configuration error: matrix dimension must be positive\n");
        return 2;
    }
    if ((options->sweep || options->workload.kind == WL_MATRIX_ROW ||
         options->workload.kind == WL_MATRIX_COL) &&
        (uint64_t)options->workload.matrix_n *
                options->workload.matrix_n >
            (uint64_t)UINT32_MAX / 4U + 1U) {
        (void)fprintf(stderr,
                      "configuration error: matrix workload addresses exceed 32 bits\n");
        return 2;
    }
    if ((options->sweep || options->workload.kind == WL_PINGPONG) &&
        options->cache.cache_size == 0U) {
        (void)fprintf(stderr, "configuration error: pingpong cache size must be positive\n");
        return 2;
    }
    return 0;
}

static void print_configuration(const cache_t *cache)
{
    const cache_config_t *cfg = cache_get_config(cache);
    const addr_layout_t *layout = cache_get_layout(cache);

    (void)printf("Configuration: direct-mapped cache\n");
    (void)printf("Cache size: %" PRIu32 " bytes\n", cfg->cache_size);
    (void)printf("Block size: %" PRIu32 " bytes\n", cfg->block_size);
    (void)printf("Lines: %zu\n", cache_get_line_count(cache));
    (void)printf("Address width: %u bits\n", cfg->addr_bits);
    (void)printf("Offset/index/tag bits: %u/%u/%u\n",
                 layout->offset_bits, layout->index_bits, layout->tag_bits);
    (void)printf("Write policy: %s\n",
                 cfg->policy == WRITE_BACK_ALLOCATE
                     ? "write-back + write-allocate"
                     : "write-through + no-write-allocate");
    (void)printf("Hit time: %" PRIu32 " cycles; miss penalty: %" PRIu32 " cycles\n",
                 cfg->hit_time, cfg->miss_penalty);
}

static const char *policy_name(write_policy_t policy)
{
    return policy == WRITE_BACK_ALLOCATE ? "wb" : "wt";
}

static int run_single(const options_t *options, FILE *csv_stream)
{
    cache_t *cache;
    workload_t *workload;
    char err[256] = "";
    mem_access_t access;
    uint64_t verbose_count = 0U;
    const cache_stats_t *stats;

    cache = cache_create(&options->cache, err, sizeof(err));
    if (cache == NULL) {
        (void)fprintf(stderr, "runtime error: %s\n", err);
        return 1;
    }
    workload = workload_create(&options->workload, err, sizeof(err));
    if (workload == NULL) {
        bool runtime_error = options->workload.kind == WL_TRACE ||
                             strstr(err, "unable to allocate") != NULL;
        (void)fprintf(stderr, "%s: %s\n",
                      runtime_error ? "runtime error" : "configuration error",
                      err);
        cache_destroy(cache);
        return runtime_error ? 1 : 2;
    }
    if (!options->csv) {
        print_configuration(cache);
    }
    while (workload_next(workload, &access)) {
        const addr_layout_t *layout = cache_get_layout(cache);
        addr_parts_t parts;
        access_result_t result;

        parts = addr_split(layout, access.addr);
        result = cache_access(cache, access.type, access.addr);
        if (options->verbose && !options->csv && verbose_count < 20U) {
            const char *label = result == RESULT_HIT
                                    ? "HIT"
                                    : result == RESULT_MISS_COMPULSORY
                                          ? "MISS (compulsory)"
                                          : "MISS (conflict/capacity)";
            (void)printf("%s 0x%08" PRIx32 " tag=0x%" PRIx32
                         " index=0x%" PRIx32 " offset=0x%" PRIx32
                         " %s\n",
                         access.type == ACCESS_READ ? "R" : "W", access.addr,
                         parts.tag, parts.index, parts.offset, label);
        }
        ++verbose_count;
    }
    if (workload_error(workload) != NULL) {
        (void)fprintf(stderr, "runtime error: %s\n", workload_error(workload));
        workload_destroy(workload);
        cache_destroy(cache);
        return 1;
    }
    stats = cache_get_stats(cache);
    if (options->csv) {
        stats_print_csv_header(csv_stream);
        stats_print_csv_row(csv_stream,
                            workload_kind_name(options->workload.kind),
                            options->cache.cache_size, options->cache.block_size,
                            policy_name(options->cache.policy), stats,
                            options->cache.hit_time, options->cache.miss_penalty);
    } else {
        stats_print(stdout, stats, options->cache.hit_time,
                    options->cache.miss_penalty);
    }
    workload_destroy(workload);
    cache_destroy(cache);
    return 0;
}

static int run_sweep(const options_t *options, FILE *stream)
{
    static const workload_kind_t kinds[] = {
        WL_SEQUENTIAL, WL_STRIDE, WL_RANDOM, WL_LOOP,
        WL_MATRIX_ROW, WL_MATRIX_COL, WL_PINGPONG
    };
    static const uint32_t cache_sizes[] = {
        512U, 1024U, 2048U, 4096U, 8192U, 16384U
    };
    static const uint32_t block_sizes[] = {16U, 32U, 64U, 128U};
    size_t wi;

    stats_print_csv_header(stream);
    for (wi = 0U; wi < sizeof(kinds) / sizeof(kinds[0]); ++wi) {
        size_t ci;

        for (ci = 0U; ci < sizeof(cache_sizes) / sizeof(cache_sizes[0]); ++ci) {
            size_t bi;

            for (bi = 0U; bi < sizeof(block_sizes) / sizeof(block_sizes[0]); ++bi) {
                options_t current = *options;
                cache_t *cache;
                workload_t *workload;
                mem_access_t access;
                char err[256] = "";
                int cache_result;

                current.cache.cache_size = cache_sizes[ci];
                current.cache.block_size = block_sizes[bi];
                current.cache.policy = WRITE_BACK_ALLOCATE;
                current.workload.kind = kinds[wi];
                current.workload.cache_size = cache_sizes[ci];
                current.workload.write_fraction = 0.25;
                current.workload.trace_path = NULL;
                cache = cache_create(&current.cache, err, sizeof(err));
                if (cache == NULL) {
                    (void)fprintf(stderr, "runtime error: %s\n", err);
                    return 1;
                }
                workload = workload_create(&current.workload, err, sizeof(err));
                if (workload == NULL) {
                    (void)fprintf(stderr, "runtime error: %s\n", err);
                    cache_destroy(cache);
                    return 1;
                }
                while (workload_next(workload, &access)) {
                    (void)cache_access(cache, access.type, access.addr);
                }
                if (workload_error(workload) != NULL) {
                    (void)fprintf(stderr, "runtime error: %s\n",
                                  workload_error(workload));
                    workload_destroy(workload);
                    cache_destroy(cache);
                    return 1;
                }
                stats_print_csv_row(stream, workload_kind_name(kinds[wi]),
                                    cache_sizes[ci], block_sizes[bi], "wb",
                                    cache_get_stats(cache),
                                    current.cache.hit_time,
                                    current.cache.miss_penalty);
                workload_destroy(workload);
                cache_destroy(cache);
                cache_result = ferror(stream);
                if (cache_result) {
                    (void)fprintf(stderr, "runtime error: failed writing CSV output\n");
                    return 1;
                }
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    options_t options;
    char err[256] = "";
    FILE *csv_stream = stdout;
    bool close_stream = false;
    int result;

    if (!parse_options(argc, argv, &options, err, sizeof(err))) {
        (void)fprintf(stderr, "usage error: %s\n", err);
        print_usage(stderr, argv[0]);
        return 2;
    }
    if (options.help) {
        print_usage(stdout, argv[0]);
        return 0;
    }
    result = validate_configuration(&options);
    if (result != 0) {
        return result;
    }
    if ((options.csv || options.sweep) && options.out_path != NULL) {
        csv_stream = fopen(options.out_path, "w");
        if (csv_stream == NULL) {
            (void)fprintf(stderr, "runtime error: cannot open output file '%s'\n",
                          options.out_path);
            return 1;
        }
        close_stream = true;
    }
    if (options.sweep) {
        result = run_sweep(&options, csv_stream);
    } else {
        result = run_single(&options, csv_stream);
    }
    if (close_stream && fclose(csv_stream) != 0 && result == 0) {
        (void)fprintf(stderr, "runtime error: failed closing CSV output file\n");
        result = 1;
    }
    return result;
}
