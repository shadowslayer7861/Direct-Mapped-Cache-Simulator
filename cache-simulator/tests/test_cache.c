/* Tests workload statistics, miss classification, and both write policies. */
#include "cache.h"
#include "memory.h"
#include "test_util.h"

#include <string.h>

static cache_config_t make_config(write_policy_t policy)
{
    cache_config_t config;

    cache_config_defaults(&config);
    config.cache_size = 1024U;
    config.block_size = 32U;
    config.policy = policy;
    return config;
}

int main(void)
{
    int test_failures = 0;
    cache_config_t config = make_config(WRITE_BACK_ALLOCATE);
    cache_t *cache;
    const cache_stats_t *stats;
    seen_set_t *seen;
    uint32_t i;
    char err[128];

    seen = seen_create();
    CHECK(seen != NULL);
    if (seen != NULL) {
        CHECK(!seen_contains(seen, 0U));
        seen_insert(seen, 0U);
        CHECK(seen_contains(seen, 0U));
        for (i = 1U; i < 100U; ++i) {
            seen_insert(seen, i);
        }
        CHECK(seen_contains(seen, 99U));
        seen_clear(seen);
        CHECK(!seen_contains(seen, 0U));
        seen_destroy(seen);
    }

    cache = cache_create(&config, err, sizeof(err));
    CHECK(cache != NULL);
    if (cache != NULL) {
        for (i = 0U; i < 1024U; ++i) {
            (void)cache_access(cache, ACCESS_READ, i * 4U);
        }
        stats = cache_get_stats(cache);
        CHECK_EQ_U(stats->misses, 128U);
        CHECK_EQ_U(stats->hits, 896U);
        CHECK_NEAR(stats_hit_rate(stats), 0.875, 0.000001);
        CHECK_EQ_U(stats->compulsory_misses, 128U);
        CHECK_EQ_U(stats->conflict_capacity_misses, 0U);

        cache_reset(cache);
        for (i = 0U; i < 100U; ++i) {
            uint32_t address = (i & 1U) == 0U ? 0U : 1024U;
            (void)cache_access(cache, ACCESS_READ, address);
        }
        stats = cache_get_stats(cache);
        CHECK_EQ_U(stats->misses, 100U);
        CHECK_EQ_U(stats->hits, 0U);
        CHECK_EQ_U(stats->compulsory_misses, 2U);
        CHECK_EQ_U(stats->conflict_capacity_misses, 98U);
        CHECK_EQ_U(stats->evictions, 99U);

        cache_reset(cache);
        for (i = 0U; i < 4096U; ++i) {
            (void)cache_access(cache, ACCESS_READ, (i * 4U) % 512U);
        }
        stats = cache_get_stats(cache);
        CHECK_EQ_U(stats->misses, 16U);
        CHECK_EQ_U(stats->compulsory_misses, 16U);
        CHECK_EQ_U(stats->conflict_capacity_misses, 0U);
        CHECK_EQ_U(stats->hits, 4080U);

        cache_reset(cache);
        for (i = 0U; i < 1024U; ++i) {
            (void)cache_access(cache, ACCESS_READ, (i * 4U) % 2048U);
        }
        stats = cache_get_stats(cache);
        CHECK_EQ_U(stats->misses, 128U);
        CHECK_EQ_U(stats->compulsory_misses, 64U);
        CHECK_EQ_U(stats->conflict_capacity_misses, 64U);

        cache_reset(cache);
        CHECK(cache_access(cache, ACCESS_WRITE, 0U) ==
              RESULT_MISS_COMPULSORY);
        CHECK(cache_access(cache, ACCESS_READ, 1024U) ==
              RESULT_MISS_COMPULSORY);
        stats = cache_get_stats(cache);
        CHECK_EQ_U(stats->writebacks, 1U);
        CHECK_EQ_U(stats->mem_writes, 1U);
        CHECK_EQ_U(stats->evictions, 1U);
        cache_destroy(cache);
    }

    config = make_config(WRITE_THROUGH_NO_ALLOCATE);
    cache = cache_create(&config, err, sizeof(err));
    CHECK(cache != NULL);
    if (cache != NULL) {
        CHECK(cache_access(cache, ACCESS_WRITE, 0U) ==
              RESULT_MISS_COMPULSORY);
        CHECK_EQ_U(cache_get_stats(cache)->mem_writes, 1U);
        CHECK_EQ_U(cache_get_stats(cache)->mem_reads, 0U);
        CHECK(cache_access(cache, ACCESS_READ, 0U) ==
              RESULT_MISS_COMPULSORY);
        CHECK(cache_access(cache, ACCESS_READ, 0U) == RESULT_HIT);
        CHECK(cache_access(cache, ACCESS_WRITE, 0U) == RESULT_HIT);
        CHECK_EQ_U(cache_get_stats(cache)->mem_writes, 2U);
        CHECK(cache_access(cache, ACCESS_READ, 1024U) ==
              RESULT_MISS_COMPULSORY);
        stats = cache_get_stats(cache);
        CHECK_EQ_U(stats->evictions, 1U);
        CHECK_EQ_U(stats->writebacks, 0U);
        CHECK_EQ_U(stats->mem_reads, 2U);
        cache_destroy(cache);
    }

    config = make_config(WRITE_BACK_ALLOCATE);
    config.block_size = 24U;
    err[0] = '\0';
    cache = cache_create(&config, err, sizeof(err));
    CHECK(cache == NULL);
    CHECK(strlen(err) > 0U);
    config = make_config(WRITE_BACK_ALLOCATE);
    config.cache_size = 16U;
    config.block_size = 32U;
    err[0] = '\0';
    cache = cache_create(&config, err, sizeof(err));
    CHECK(cache == NULL);
    CHECK(strlen(err) > 0U);
    config = make_config(WRITE_BACK_ALLOCATE);
    config.addr_bits = 40U;
    err[0] = '\0';
    cache = cache_create(&config, err, sizeof(err));
    CHECK(cache == NULL);
    CHECK(strlen(err) > 0U);
    config = make_config(WRITE_BACK_ALLOCATE);
    config.cache_size = 256U;
    config.block_size = 16U;
    config.addr_bits = 8U;
    err[0] = '\0';
    cache = cache_create(&config, err, sizeof(err));
    CHECK(cache == NULL);
    CHECK(strlen(err) > 0U);

    TEST_SUMMARY();
}
