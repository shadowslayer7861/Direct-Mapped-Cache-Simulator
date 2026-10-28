/* Tests deterministic generators, matrix locality, and trace diagnostics. */
#include "cache.h"
#include "test_util.h"
#include "workload.h"

#include <stdio.h>
#include <string.h>

static workload_t *create_workload(workload_params_t *params,
                                   char *err, size_t errlen)
{
    return workload_create(params, err, errlen);
}

static double matrix_hit_rate(workload_kind_t kind, int *failed)
{
    cache_config_t config;
    workload_params_t params;
    cache_t *cache;
    workload_t *workload;
    mem_access_t access;
    char err[128] = "";

    cache_config_defaults(&config);
    config.cache_size = 4096U;
    config.block_size = 32U;
    memset(&params, 0, sizeof(params));
    params.kind = kind;
    params.count = 1U;
    params.matrix_n = 128U;
    params.seed = 1U;
    cache = cache_create(&config, err, sizeof(err));
    if (cache == NULL) {
        (void)fprintf(stderr, "matrix cache_create failed: %s\n", err);
        *failed = 1;
        return 0.0;
    }
    workload = create_workload(&params, err, sizeof(err));
    if (workload == NULL) {
        (void)fprintf(stderr, "matrix workload_create failed: %s\n", err);
        cache_destroy(cache);
        *failed = 1;
        return 0.0;
    }
    while (workload_next(workload, &access)) {
        (void)cache_access(cache, access.type, access.addr);
    }
    if (workload_error(workload) != NULL) {
        (void)fprintf(stderr, "matrix workload failed: %s\n",
                      workload_error(workload));
        *failed = 1;
    }
    {
        double result = stats_hit_rate(cache_get_stats(cache));

        workload_destroy(workload);
        cache_destroy(cache);
        return result;
    }
}

int main(void)
{
    int test_failures = 0;
    workload_params_t params;
    workload_t *workload;
    workload_t *same_workload;
    workload_t *different_workload;
    mem_access_t access;
    mem_access_t same_access;
    mem_access_t different_access;
    workload_kind_t kind;
    char err[256] = "";
    uint32_t i;
    int different_seed_found = 0;
    int matrix_failed = 0;
    double row_hit_rate;
    double col_hit_rate;

    CHECK(workload_parse_kind("matrix-col", &kind));
    CHECK_EQ_U(kind, WL_MATRIX_COL);
    CHECK(strcmp(workload_kind_name(WL_PINGPONG), "pingpong") == 0);
    CHECK(!workload_parse_kind("unknown", &kind));

    memset(&params, 0, sizeof(params));
    params.kind = WL_SEQUENTIAL;
    params.count = 5U;
    params.seed = 1U;
    workload = create_workload(&params, err, sizeof(err));
    CHECK(workload != NULL);
    if (workload != NULL) {
        for (i = 0U; i < 5U; ++i) {
            CHECK(workload_next(workload, &access));
            CHECK_EQ_U(access.addr, i * 4U);
        }
        CHECK(!workload_next(workload, &access));
        workload_destroy(workload);
    }

    params.kind = WL_STRIDE;
    params.count = 5U;
    params.stride = 12U;
    params.range = 40U;
    workload = create_workload(&params, err, sizeof(err));
    CHECK(workload != NULL);
    if (workload != NULL) {
        static const uint32_t expected[] = {0U, 12U, 24U, 36U, 8U};

        for (i = 0U; i < 5U; ++i) {
            CHECK(workload_next(workload, &access));
            CHECK_EQ_U(access.addr, expected[i]);
        }
        workload_destroy(workload);
    }

    params.kind = WL_LOOP;
    params.count = 6U;
    params.loop_bytes = 16U;
    workload = create_workload(&params, err, sizeof(err));
    CHECK(workload != NULL);
    if (workload != NULL) {
        static const uint32_t expected[] = {0U, 4U, 8U, 12U, 0U, 4U};

        for (i = 0U; i < 6U; ++i) {
            CHECK(workload_next(workload, &access));
            CHECK_EQ_U(access.addr, expected[i]);
        }
        workload_destroy(workload);
    }

    params.kind = WL_RANDOM;
    params.count = 64U;
    params.range = 256U;
    params.seed = 123U;
    same_workload = create_workload(&params, err, sizeof(err));
    workload = create_workload(&params, err, sizeof(err));
    params.seed = 456U;
    different_workload = create_workload(&params, err, sizeof(err));
    CHECK(workload != NULL);
    CHECK(same_workload != NULL);
    CHECK(different_workload != NULL);
    if (workload != NULL && same_workload != NULL &&
        different_workload != NULL) {
        for (i = 0U; i < 64U; ++i) {
            CHECK(workload_next(workload, &access));
            CHECK(workload_next(same_workload, &same_access));
            CHECK(workload_next(different_workload, &different_access));
            CHECK_EQ_U(access.addr, same_access.addr);
            CHECK_EQ_U(access.type, same_access.type);
            if (access.addr != different_access.addr ||
                access.type != different_access.type) {
                different_seed_found = 1;
            }
        }
        CHECK(different_seed_found);
    }
    workload_destroy(workload);
    workload_destroy(same_workload);
    workload_destroy(different_workload);

    params.kind = WL_PINGPONG;
    params.count = 4U;
    params.cache_size = 1024U;
    workload = create_workload(&params, err, sizeof(err));
    CHECK(workload != NULL);
    if (workload != NULL) {
        static const uint32_t expected[] = {0U, 1024U, 0U, 1024U};

        for (i = 0U; i < 4U; ++i) {
            CHECK(workload_next(workload, &access));
            CHECK_EQ_U(access.addr, expected[i]);
        }
        workload_destroy(workload);
    }

    row_hit_rate = matrix_hit_rate(WL_MATRIX_ROW, &matrix_failed);
    col_hit_rate = matrix_hit_rate(WL_MATRIX_COL, &matrix_failed);
    CHECK(!matrix_failed);
    CHECK_NEAR(row_hit_rate, 0.875, 0.001);
    CHECK(col_hit_rate < 0.10);

    params.kind = WL_TRACE;
    params.trace_path = "traces/sample_small.trace";
    workload = create_workload(&params, err, sizeof(err));
    CHECK(workload != NULL);
    if (workload != NULL) {
        uint32_t trace_accesses = 0U;

        while (workload_next(workload, &access)) {
            ++trace_accesses;
        }
        CHECK(workload_error(workload) == NULL);
        CHECK_EQ_U(trace_accesses, 30U);
        workload_destroy(workload);
    }

    {
        const char *bad_path = "tests/tmp_bad_trace.trace";
        FILE *bad_trace = fopen(bad_path, "w");

        CHECK(bad_trace != NULL);
        if (bad_trace != NULL) {
            CHECK(fputs("# bad trace fixture\nR 0x10\nBAD 0x20\n",
                        bad_trace) >= 0);
            CHECK(fclose(bad_trace) == 0);
            params.trace_path = bad_path;
            workload = create_workload(&params, err, sizeof(err));
            CHECK(workload != NULL);
            if (workload != NULL) {
                CHECK(workload_next(workload, &access));
                CHECK(!workload_next(workload, &access));
                CHECK(workload_error(workload) != NULL);
                if (workload_error(workload) != NULL) {
                    CHECK(strstr(workload_error(workload), ":3:") != NULL);
                }
                workload_destroy(workload);
            }
            CHECK(remove(bad_path) == 0);
        }
    }

    TEST_SUMMARY();
}
