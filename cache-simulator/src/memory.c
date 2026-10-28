/* Open-addressed block-seen set used for miss classification. */
#include "memory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct seen_set {
    uint32_t *keys;
    unsigned char *occupied;
    size_t capacity;
    size_t count;
};

static size_t hash_block(uint32_t value)
{
    value ^= value >> 16U;
    value *= UINT32_C(0x7feb352d);
    value ^= value >> 15U;
    value *= UINT32_C(0x846ca68b);
    value ^= value >> 16U;
    return (size_t)value;
}

static bool allocate_slots(size_t capacity, uint32_t **keys,
                           unsigned char **occupied)
{
    if (capacity > SIZE_MAX / sizeof(**keys)) {
        return false;
    }
    *keys = (uint32_t *)malloc(capacity * sizeof(**keys));
    if (*keys == NULL) {
        return false;
    }
    *occupied = (unsigned char *)calloc(capacity, sizeof(**occupied));
    if (*occupied == NULL) {
        free(*keys);
        *keys = NULL;
        return false;
    }
    return true;
}

seen_set_t *seen_create(void)
{
    seen_set_t *set = (seen_set_t *)malloc(sizeof(*set));

    if (set == NULL) {
        return NULL;
    }
    set->keys = NULL;
    set->occupied = NULL;
    set->capacity = 16U;
    set->count = 0U;
    if (!allocate_slots(set->capacity, &set->keys, &set->occupied)) {
        free(set);
        return NULL;
    }
    return set;
}

bool seen_contains(const seen_set_t *set, uint32_t block_id)
{
    size_t slot;

    if (set == NULL || set->capacity == 0U) {
        return false;
    }
    slot = hash_block(block_id) & (set->capacity - 1U);
    while (set->occupied[slot] != 0U) {
        if (set->keys[slot] == block_id) {
            return true;
        }
        slot = (slot + 1U) & (set->capacity - 1U);
    }
    return false;
}

static void insert_without_growth(seen_set_t *set, uint32_t block_id)
{
    size_t slot = hash_block(block_id) & (set->capacity - 1U);

    while (set->occupied[slot] != 0U) {
        if (set->keys[slot] == block_id) {
            return;
        }
        slot = (slot + 1U) & (set->capacity - 1U);
    }
    set->occupied[slot] = 1U;
    set->keys[slot] = block_id;
    ++set->count;
}

static bool grow_set(seen_set_t *set)
{
    uint32_t *new_keys;
    unsigned char *new_occupied;
    size_t old_capacity = set->capacity;
    size_t i;

    if (old_capacity > SIZE_MAX / 2U) {
        return false;
    }
    if (!allocate_slots(old_capacity * 2U, &new_keys, &new_occupied)) {
        return false;
    }
    {
        uint32_t *old_keys = set->keys;
        unsigned char *old_occupied = set->occupied;

        set->keys = new_keys;
        set->occupied = new_occupied;
        set->capacity = old_capacity * 2U;
        set->count = 0U;
        for (i = 0U; i < old_capacity; ++i) {
            if (old_occupied[i] != 0U) {
                insert_without_growth(set, old_keys[i]);
            }
        }
        free(old_keys);
        free(old_occupied);
    }
    return true;
}

void seen_insert(seen_set_t *set, uint32_t block_id)
{
    size_t load_threshold;

    if (set == NULL || seen_contains(set, block_id)) {
        return;
    }
    load_threshold = (set->capacity / 10U) * 7U +
                     ((set->capacity % 10U) * 7U) / 10U;
    if (set->count + 1U > load_threshold &&
        !grow_set(set)) {
        (void)fprintf(stderr, "fatal: unable to grow block-seen set\n");
        exit(EXIT_FAILURE);
    }
    insert_without_growth(set, block_id);
}

void seen_destroy(seen_set_t *set)
{
    if (set != NULL) {
        free(set->keys);
        free(set->occupied);
        free(set);
    }
}

void seen_clear(seen_set_t *set)
{
    if (set != NULL) {
        memset(set->occupied, 0, set->capacity * sizeof(*set->occupied));
        set->count = 0U;
    }
}
