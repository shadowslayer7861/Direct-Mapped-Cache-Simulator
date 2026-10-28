/* Direct-mapped cache access behavior and statistics accounting. */
#include "cache.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cache {
    cache_config_t config;
    addr_layout_t layout;
    cache_line_t *lines;
    size_t line_count;
    seen_set_t *seen;
    cache_stats_t stats;
};

static void set_error(char *err, size_t errlen, const char *message)
{
    if (err != NULL && errlen > 0U) {
        (void)snprintf(err, errlen, "%s", message);
    }
}

void cache_config_defaults(cache_config_t *cfg)
{
    if (cfg != NULL) {
        cfg->cache_size = 4096U;
        cfg->block_size = 32U;
        cfg->addr_bits = 32U;
        cfg->policy = WRITE_BACK_ALLOCATE;
        cfg->hit_time = 1U;
        cfg->miss_penalty = 100U;
    }
}

cache_t *cache_create(const cache_config_t *cfg, char *err, size_t errlen)
{
    cache_t *cache;

    if (cfg == NULL) {
        set_error(err, errlen, "cache configuration must not be null");
        return NULL;
    }
    if (cfg->policy != WRITE_BACK_ALLOCATE &&
        cfg->policy != WRITE_THROUGH_NO_ALLOCATE) {
        set_error(err, errlen, "invalid write policy");
        return NULL;
    }
    cache = (cache_t *)malloc(sizeof(*cache));
    if (cache == NULL) {
        set_error(err, errlen, "unable to allocate cache state");
        return NULL;
    }
    cache->config = *cfg;
    cache->lines = NULL;
    cache->line_count = 0U;
    cache->seen = NULL;
    memset(&cache->stats, 0, sizeof(cache->stats));

    if (addr_layout_init(&cache->layout, cfg->cache_size, cfg->block_size,
                         cfg->addr_bits, err, errlen) != 0) {
        free(cache);
        return NULL;
    }
    cache->line_count = (size_t)(cfg->cache_size / cfg->block_size);
    if (cache->line_count > SIZE_MAX / sizeof(*cache->lines)) {
        set_error(err, errlen, "cache line array size is too large");
        free(cache);
        return NULL;
    }
    cache->lines = (cache_line_t *)calloc(cache->line_count,
                                          sizeof(*cache->lines));
    if (cache->lines == NULL) {
        set_error(err, errlen, "unable to allocate cache lines");
        free(cache);
        return NULL;
    }
    cache->seen = seen_create();
    if (cache->seen == NULL) {
        set_error(err, errlen, "unable to allocate block-seen set");
        free(cache->lines);
        free(cache);
        return NULL;
    }
    return cache;
}

void cache_destroy(cache_t *cache)
{
    if (cache != NULL) {
        seen_destroy(cache->seen);
        free(cache->lines);
        free(cache);
    }
}

void cache_reset(cache_t *cache)
{
    if (cache != NULL) {
        memset(cache->lines, 0, cache->line_count * sizeof(*cache->lines));
        memset(&cache->stats, 0, sizeof(cache->stats));
        seen_clear(cache->seen);
    }
}

access_result_t cache_access(cache_t *cache, access_type_t type, uint32_t addr)
{
    addr_parts_t parts = addr_split(&cache->layout, addr);
    uint32_t block_id = addr_block_id(&cache->layout, addr);
    cache_line_t *line = &cache->lines[parts.index];
    bool is_write = type == ACCESS_WRITE;
    access_result_t result;

    ++cache->stats.accesses;
    if (is_write) {
        ++cache->stats.writes;
    } else {
        ++cache->stats.reads;
    }

    if (line->valid && line->tag == parts.tag) {
        result = RESULT_HIT;
        ++cache->stats.hits;
        if (is_write) {
            ++cache->stats.write_hits;
            if (cache->config.policy == WRITE_BACK_ALLOCATE) {
                line->dirty = true;
            } else {
                line->dirty = false;
                ++cache->stats.mem_writes;
            }
        } else {
            ++cache->stats.read_hits;
        }
        return result;
    }

    ++cache->stats.misses;
    if (is_write) {
        ++cache->stats.write_misses;
    } else {
        ++cache->stats.read_misses;
    }
    if (seen_contains(cache->seen, block_id)) {
        result = RESULT_MISS_CONFLICT_CAPACITY;
        ++cache->stats.conflict_capacity_misses;
    } else {
        result = RESULT_MISS_COMPULSORY;
        ++cache->stats.compulsory_misses;
    }

    if (cache->config.policy == WRITE_THROUGH_NO_ALLOCATE && is_write) {
        ++cache->stats.mem_writes;
        return result;
    }

    if (line->valid) {
        ++cache->stats.evictions;
        if (line->dirty) {
            ++cache->stats.writebacks;
            ++cache->stats.mem_writes;
        }
    }
    ++cache->stats.mem_reads;
    line->valid = true;
    line->dirty = is_write &&
                  cache->config.policy == WRITE_BACK_ALLOCATE;
    line->tag = parts.tag;
    seen_insert(cache->seen, block_id);
    return result;
}

const cache_stats_t *cache_get_stats(const cache_t *cache)
{
    return cache == NULL ? NULL : &cache->stats;
}

const cache_config_t *cache_get_config(const cache_t *cache)
{
    return cache == NULL ? NULL : &cache->config;
}

const addr_layout_t *cache_get_layout(const cache_t *cache)
{
    return cache == NULL ? NULL : &cache->layout;
}

size_t cache_get_line_count(const cache_t *cache)
{
    return cache == NULL ? 0U : cache->line_count;
}
