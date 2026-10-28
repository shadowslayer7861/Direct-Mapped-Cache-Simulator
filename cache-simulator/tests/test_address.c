/* Tests address layout creation, decomposition, and invalid configurations. */
#include "address.h"
#include "test_util.h"

#include <string.h>

int main(void)
{
    int test_failures = 0;
    addr_layout_t layout;
    addr_parts_t parts;
    const uint32_t address = UINT32_C(0x12345678);
    char err[128];

    CHECK(is_pow2(1U));
    CHECK(is_pow2(4096U));
    CHECK(!is_pow2(0U));
    CHECK(!is_pow2(24U));
    CHECK_EQ_U(log2_u32(128U), 7U);

    CHECK(addr_layout_init(&layout, 1024U, 16U, 32U,
                           err, sizeof(err)) == 0);
    CHECK_EQ_U(layout.offset_bits, 4U);
    CHECK_EQ_U(layout.index_bits, 6U);
    CHECK_EQ_U(layout.tag_bits, 22U);

    parts = addr_split(&layout, address);
    CHECK_EQ_U(parts.tag, UINT32_C(0x48D15));
    CHECK_EQ_U(parts.index, 39U);
    CHECK_EQ_U(parts.offset, 8U);
    CHECK_EQ_U(addr_block_id(&layout, address), UINT32_C(0x1234567));
    CHECK_EQ_U(addr_rebuild(&layout, parts.tag, parts.index),
               UINT32_C(0x12345670));
    CHECK_EQ_U(addr_split(&layout, addr_rebuild(&layout, parts.tag,
                                                parts.index)).tag,
               parts.tag);
    CHECK_EQ_U(addr_split(&layout, addr_rebuild(&layout, parts.tag,
                                                parts.index)).index,
               parts.index);
    CHECK_EQ_U(addr_block_id(&layout, addr_rebuild(&layout, parts.tag,
                                                   parts.index)),
               addr_block_id(&layout, address));

    CHECK(addr_layout_init(&layout, 1024U, 24U, 32U,
                           err, sizeof(err)) == -1);
    CHECK(strlen(err) > 0U);
    CHECK(addr_layout_init(&layout, 16U, 32U, 32U,
                           err, sizeof(err)) == -1);
    CHECK(strlen(err) > 0U);
    CHECK(addr_layout_init(&layout, 1024U, 16U, 40U,
                           err, sizeof(err)) == -1);
    CHECK(strlen(err) > 0U);
    CHECK(addr_layout_init(&layout, 256U, 16U, 8U,
                           err, sizeof(err)) == -1);
    CHECK(strlen(err) > 0U);

    TEST_SUMMARY();
}
