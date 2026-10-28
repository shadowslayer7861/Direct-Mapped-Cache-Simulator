/* Dynamically growing set of memory blocks seen by the cache. */
#ifndef CACHE_SIMULATOR_MEMORY_H
#define CACHE_SIMULATOR_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct seen_set seen_set_t;

seen_set_t *seen_create(void);
bool seen_contains(const seen_set_t *set, uint32_t block_id);
void seen_insert(seen_set_t *set, uint32_t block_id);
void seen_destroy(seen_set_t *set);
void seen_clear(seen_set_t *set);

#endif
