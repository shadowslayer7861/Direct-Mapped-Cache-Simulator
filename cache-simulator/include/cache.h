/* Direct-mapped cache configuration, state, and access interface. */
#ifndef CACHE_SIMULATOR_CACHE_H
#define CACHE_SIMULATOR_CACHE_H

#include "address.h"
#include "stats.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    WRITE_BACK_ALLOCATE,
    WRITE_THROUGH_NO_ALLOCATE
} write_policy_t;

typedef struct {
    uint32_t cache_size;
    uint32_t block_size;
    unsigned addr_bits;
    write_policy_t policy;
    uint32_t hit_time;
    uint32_t miss_penalty;
} cache_config_t;

typedef struct {
    bool valid;
    bool dirty;
    uint32_t tag;
} cache_line_t;

typedef enum {
    ACCESS_READ,
    ACCESS_WRITE
} access_type_t;

typedef enum {
    RESULT_HIT,
    RESULT_MISS_COMPULSORY,
    RESULT_MISS_CONFLICT_CAPACITY
} access_result_t;

typedef struct cache cache_t;

void cache_config_defaults(cache_config_t *cfg);
cache_t *cache_create(const cache_config_t *cfg, char *err, size_t errlen);
void cache_destroy(cache_t *cache);
void cache_reset(cache_t *cache);
access_result_t cache_access(cache_t *cache, access_type_t type, uint32_t addr);
const cache_stats_t *cache_get_stats(const cache_t *cache);
const cache_config_t *cache_get_config(const cache_t *cache);
const addr_layout_t *cache_get_layout(const cache_t *cache);
size_t cache_get_line_count(const cache_t *cache);

#endif
