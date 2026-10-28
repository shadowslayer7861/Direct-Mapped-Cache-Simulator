/* Address layout validation and bitwise address decomposition. */
#include "address.h"

#include <stdio.h>

static void set_error(char *err, size_t errlen, const char *message)
{
    if (err != NULL && errlen > 0) {
        (void)snprintf(err, errlen, "%s", message);
    }
}

bool is_pow2(uint32_t x)
{
    return x != 0U && (x & (x - 1U)) == 0U;
}

unsigned log2_u32(uint32_t x)
{
    unsigned result = 0;

    while (x > 1U) {
        x >>= 1U;
        ++result;
    }
    return result;
}

int addr_layout_init(addr_layout_t *l, uint32_t cache_size,
                     uint32_t block_size, unsigned addr_bits,
                     char *err, size_t errlen)
{
    uint32_t line_count;
    unsigned used_bits;

    if (l == NULL) {
        set_error(err, errlen, "address layout pointer must not be null");
        return -1;
    }
    if (!is_pow2(cache_size)) {
        set_error(err, errlen, "cache size must be a nonzero power of two");
        return -1;
    }
    if (!is_pow2(block_size)) {
        set_error(err, errlen, "block size must be a nonzero power of two");
        return -1;
    }
    if (block_size < 4U) {
        set_error(err, errlen, "block size must be at least 4 bytes");
        return -1;
    }
    if (cache_size < block_size) {
        set_error(err, errlen, "cache size must be at least the block size");
        return -1;
    }
    if (addr_bits < 8U || addr_bits > 32U) {
        set_error(err, errlen, "address width must be between 8 and 32 bits");
        return -1;
    }

    line_count = cache_size / block_size;
    l->addr_bits = addr_bits;
    l->offset_bits = log2_u32(block_size);
    l->index_bits = log2_u32(line_count);
    used_bits = l->offset_bits + l->index_bits;
    if (used_bits >= addr_bits) {
        set_error(err, errlen, "cache configuration leaves fewer than 1 tag bit");
        return -1;
    }
    l->tag_bits = addr_bits - used_bits;
    l->offset_mask = block_size - 1U;
    l->index_mask = line_count - 1U;
    l->addr_mask = addr_bits == 32U
                       ? UINT32_MAX
                       : (UINT32_C(1) << addr_bits) - 1U;
    return 0;
}

addr_parts_t addr_split(const addr_layout_t *l, uint32_t addr)
{
    addr_parts_t parts;

    parts.offset = addr & l->offset_mask;
    parts.index = (addr >> l->offset_bits) & l->index_mask;
    parts.tag = (addr & l->addr_mask) >> (l->offset_bits + l->index_bits);
    return parts;
}

uint32_t addr_block_id(const addr_layout_t *l, uint32_t addr)
{
    return (addr & l->addr_mask) >> l->offset_bits;
}

uint32_t addr_rebuild(const addr_layout_t *l, uint32_t tag, uint32_t index)
{
    uint32_t address = (tag << l->index_bits) | (index & l->index_mask);

    return (address << l->offset_bits) & l->addr_mask;
}
