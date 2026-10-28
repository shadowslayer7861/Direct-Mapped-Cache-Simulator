/* Streaming memory-access workload generators and trace reader. */
#ifndef CACHE_SIMULATOR_WORKLOAD_H
#define CACHE_SIMULATOR_WORKLOAD_H

#include "cache.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    access_type_t type;
    uint32_t addr;
} mem_access_t;

typedef enum {
    WL_SEQUENTIAL,
    WL_STRIDE,
    WL_RANDOM,
    WL_LOOP,
    WL_MATRIX_ROW,
    WL_MATRIX_COL,
    WL_PINGPONG,
    WL_TRACE
} workload_kind_t;

typedef struct {
    workload_kind_t kind;
    uint32_t count;
    uint32_t stride;
    uint32_t range;
    uint32_t seed;
    uint32_t loop_bytes;
    uint32_t matrix_n;
    double write_fraction;
    const char *trace_path;
    uint32_t cache_size;
} workload_params_t;

typedef struct workload workload_t;

workload_t *workload_create(const workload_params_t *params,
                            char *err, size_t errlen);
bool workload_next(workload_t *workload, mem_access_t *out);
void workload_destroy(workload_t *workload);
bool workload_parse_kind(const char *name, workload_kind_t *out);
const char *workload_kind_name(workload_kind_t kind);
const char *workload_error(const workload_t *workload);

#endif
