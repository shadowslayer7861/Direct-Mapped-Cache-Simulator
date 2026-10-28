/* Address layout and bitwise address decomposition. */
#ifndef CACHE_SIMULATOR_ADDRESS_H
#define CACHE_SIMULATOR_ADDRESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    unsigned addr_bits;
    unsigned offset_bits;
    unsigned index_bits;
    unsigned tag_bits;
    uint32_t offset_mask;
    uint32_t index_mask;
    uint32_t addr_mask;
} addr_layout_t;

typedef struct {
    uint32_t tag;
    uint32_t index;
    uint32_t offset;
} addr_parts_t;

bool is_pow2(uint32_t x);
unsigned log2_u32(uint32_t x);
int addr_layout_init(addr_layout_t *l, uint32_t cache_size,
                    uint32_t block_size, unsigned addr_bits,
                    char *err, size_t errlen);
addr_parts_t addr_split(const addr_layout_t *l, uint32_t addr);
uint32_t addr_block_id(const addr_layout_t *l, uint32_t addr);
uint32_t addr_rebuild(const addr_layout_t *l, uint32_t tag, uint32_t index);

#endif
