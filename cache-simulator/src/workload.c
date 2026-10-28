/* Streaming workload generators and robust text trace parsing. */
#include "workload.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_LINE_CAPACITY 512U

struct workload {
    workload_params_t params;
    uint64_t position;
    uint64_t total;
    uint32_t random_state;
    FILE *trace;
    char *trace_path;
    uint64_t line_number;
    char error[256];
};

static void set_error(char *err, size_t errlen, const char *message)
{
    if (err != NULL && errlen > 0U) {
        (void)snprintf(err, errlen, "%s", message);
    }
}

static void set_trace_error(workload_t *workload, const char *detail)
{
    (void)snprintf(workload->error, sizeof(workload->error),
                   "%s:%" PRIu64 ": %s", workload->trace_path,
                   workload->line_number, detail);
}

static uint32_t next_random(workload_t *workload)
{
    uint32_t value = workload->random_state;

    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    workload->random_state = value;
    return value;
}

static uint32_t random_below(workload_t *workload, uint32_t bound)
{
    uint64_t domain = UINT32_MAX;
    uint64_t limit = domain - domain % bound;
    uint32_t value;

    do {
        value = next_random(workload) - 1U;
    } while ((uint64_t)value >= limit);
    return value % bound;
}

bool workload_parse_kind(const char *name, workload_kind_t *out)
{
    static const char *const names[] = {
        "sequential", "stride", "random", "loop", "matrix-row",
        "matrix-col", "pingpong", "trace"
    };
    size_t i;

    if (name == NULL || out == NULL) {
        return false;
    }
    for (i = 0U; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (strcmp(name, names[i]) == 0) {
            *out = (workload_kind_t)i;
            return true;
        }
    }
    return false;
}

const char *workload_kind_name(workload_kind_t kind)
{
    static const char *const names[] = {
        "sequential", "stride", "random", "loop", "matrix-row",
        "matrix-col", "pingpong", "trace"
    };

    if ((unsigned)kind >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[kind];
}

workload_t *workload_create(const workload_params_t *params,
                            char *err, size_t errlen)
{
    workload_t *workload;
    uint64_t matrix_total;

    if (params == NULL) {
        set_error(err, errlen, "workload parameters must not be null");
        return NULL;
    }
    if ((unsigned)params->kind > (unsigned)WL_TRACE) {
        set_error(err, errlen, "invalid workload kind");
        return NULL;
    }
    if (params->count == 0U) {
        set_error(err, errlen, "workload count must be greater than zero");
        return NULL;
    }
    if (params->write_fraction != params->write_fraction ||
        params->write_fraction < 0.0 || params->write_fraction > 1.0) {
        set_error(err, errlen, "write fraction must be between 0 and 1");
        return NULL;
    }
    if (params->kind == WL_STRIDE && params->range == 0U) {
        set_error(err, errlen, "stride workload range must be greater than zero");
        return NULL;
    }
    if (params->kind == WL_RANDOM && params->range < 4U) {
        set_error(err, errlen, "random workload range must be at least 4 bytes");
        return NULL;
    }
    if (params->kind == WL_LOOP &&
        (params->loop_bytes < 4U || params->loop_bytes % 4U != 0U)) {
        set_error(err, errlen,
                  "loop size must be a positive multiple of 4 bytes");
        return NULL;
    }
    if ((params->kind == WL_MATRIX_ROW || params->kind == WL_MATRIX_COL) &&
        params->matrix_n == 0U) {
        set_error(err, errlen, "matrix dimension must be greater than zero");
        return NULL;
    }
    matrix_total = (uint64_t)params->matrix_n * params->matrix_n;
    if ((params->kind == WL_MATRIX_ROW || params->kind == WL_MATRIX_COL) &&
        matrix_total > (uint64_t)UINT32_MAX / 4U + 1U) {
        set_error(err, errlen, "matrix workload addresses exceed 32 bits");
        return NULL;
    }
    if (params->kind == WL_PINGPONG && params->cache_size == 0U) {
        set_error(err, errlen, "pingpong cache size must be greater than zero");
        return NULL;
    }
    if (params->kind == WL_TRACE &&
        (params->trace_path == NULL || params->trace_path[0] == '\0')) {
        set_error(err, errlen, "trace workload requires a trace file");
        return NULL;
    }

    workload = (workload_t *)malloc(sizeof(*workload));
    if (workload == NULL) {
        set_error(err, errlen, "unable to allocate workload state");
        return NULL;
    }
    memset(workload, 0, sizeof(*workload));
    workload->params = *params;
    workload->random_state = params->seed == 0U
                                 ? UINT32_C(0x6d2b79f5)
                                 : params->seed;
    if (params->kind == WL_MATRIX_ROW || params->kind == WL_MATRIX_COL) {
        workload->total = matrix_total;
    } else {
        workload->total = params->count;
    }

    if (params->kind == WL_TRACE) {
        size_t path_len = strlen(params->trace_path);

        if (path_len == SIZE_MAX) {
            set_error(err, errlen, "trace path is too long");
            free(workload);
            return NULL;
        }
        workload->trace_path = (char *)malloc(path_len + 1U);
        if (workload->trace_path == NULL) {
            set_error(err, errlen, "unable to allocate trace path");
            free(workload);
            return NULL;
        }
        memcpy(workload->trace_path, params->trace_path, path_len + 1U);
        workload->trace = fopen(workload->trace_path, "r");
        if (workload->trace == NULL) {
            char message[256];

            (void)snprintf(message, sizeof(message),
                           "cannot open trace file '%s'", workload->trace_path);
            set_error(err, errlen, message);
            free(workload->trace_path);
            free(workload);
            return NULL;
        }
    }
    return workload;
}

static bool trace_next(workload_t *workload, mem_access_t *out)
{
    char line[TRACE_LINE_CAPACITY];

    for (;;) {
        char *cursor;
        char *end;
        unsigned long parsed;
        int type;

        if (fgets(line, (int)sizeof(line), workload->trace) == NULL) {
            if (ferror(workload->trace)) {
                ++workload->line_number;
                set_trace_error(workload, "error reading trace file");
            }
            return false;
        }
        ++workload->line_number;
        if (strchr(line, '\n') == NULL && !feof(workload->trace)) {
            int character;

            while ((character = fgetc(workload->trace)) != '\n' &&
                   character != EOF) {
            }
            set_trace_error(workload, "line is too long");
            return false;
        }
        cursor = line;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
               *cursor == '\n') {
            ++cursor;
        }
        if (*cursor == '\0' || *cursor == '#') {
            continue;
        }
        type = (unsigned char)*cursor++;
        if ((type != 'R' && type != 'W') ||
            (*cursor != ' ' && *cursor != '\t')) {
            set_trace_error(workload, "expected 'R <hex-address>' or 'W <hex-address>'");
            return false;
        }
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (*cursor == '-' || *cursor == '+' || *cursor == '\0') {
            set_trace_error(workload, "expected a hexadecimal address");
            return false;
        }
        errno = 0;
        parsed = strtoul(cursor, &end, 16);
        if (end == cursor || errno == ERANGE || parsed > UINT32_MAX) {
            set_trace_error(workload, "invalid or out-of-range hexadecimal address");
            return false;
        }
        cursor = end;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
               *cursor == '\n') {
            ++cursor;
        }
        if (*cursor != '\0') {
            set_trace_error(workload, "unexpected text after address");
            return false;
        }
        out->type = type == 'R' ? ACCESS_READ : ACCESS_WRITE;
        out->addr = (uint32_t)parsed;
        return true;
    }
}

bool workload_next(workload_t *workload, mem_access_t *out)
{
    uint64_t position;
    uint32_t address;
    uint32_t random_value;

    if (workload == NULL || out == NULL || workload->error[0] != '\0') {
        return false;
    }
    if (workload->params.kind == WL_TRACE) {
        return trace_next(workload, out);
    }
    if (workload->position >= workload->total) {
        return false;
    }
    position = workload->position++;
    switch (workload->params.kind) {
    case WL_SEQUENTIAL:
        address = (uint32_t)(position * 4U);
        break;
    case WL_STRIDE:
        address = (uint32_t)((position * workload->params.stride) %
                             workload->params.range);
        break;
    case WL_RANDOM: {
        uint32_t slots = (uint32_t)(((uint64_t)workload->params.range + 3U) / 4U);

        address = random_below(workload, slots) * 4U;
        break;
    }
    case WL_LOOP:
        address = (uint32_t)((position * 4U) % workload->params.loop_bytes);
        break;
    case WL_MATRIX_ROW: {
        uint64_t n = workload->params.matrix_n;
        uint64_t row = position / n;
        uint64_t column = position % n;

        address = (uint32_t)((row * n + column) * 4U);
        break;
    }
    case WL_MATRIX_COL: {
        uint64_t n = workload->params.matrix_n;
        uint64_t row = position % n;
        uint64_t column = position / n;

        address = (uint32_t)((row * n + column) * 4U);
        break;
    }
    case WL_PINGPONG:
        address = (position & 1U) == 0U ? 0U : workload->params.cache_size;
        break;
    case WL_TRACE:
        return false;
    default:
        return false;
    }

    random_value = next_random(workload);
    out->type = (double)random_value / 4294967296.0 <
                        workload->params.write_fraction
                    ? ACCESS_WRITE
                    : ACCESS_READ;
    out->addr = address;
    return true;
}

void workload_destroy(workload_t *workload)
{
    if (workload != NULL) {
        if (workload->trace != NULL) {
            (void)fclose(workload->trace);
        }
        free(workload->trace_path);
        free(workload);
    }
}

const char *workload_error(const workload_t *workload)
{
    if (workload == NULL || workload->error[0] == '\0') {
        return NULL;
    }
    return workload->error;
}
