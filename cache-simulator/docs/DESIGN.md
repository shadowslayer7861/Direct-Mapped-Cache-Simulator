# Design

## Address breakdown

With the configured 32-bit address width, a 1024-byte cache, and 16-byte blocks, the program reports 22 tag bits, 6 index bits, and 4 offset bits. Fields are extracted with masks and shifts:

```text
31                    10 9          4 3          0
+-----------------------+------------+------------+
|       tag (22)        | index (6)  | offset (4) |
+-----------------------+------------+------------+
```

The worked address `0x12345678` was run through the verbose trace path with that configuration. The program reported `tag=0x48d15`, `index=0x27`, and `offset=0x8`; the index is 39 in decimal. Rebuilding the address from the tag and index gives the containing block base `0x12345670`. The byte offset is applied within that block.

## Data structures and ownership

- `addr_layout_t` holds the validated bit widths and masks; `addr_parts_t` holds a split tag, index, and offset.
- `cache_t` owns the configuration, layout, direct-mapped line array, counters, and block-seen set.
- Each `cache_line_t` stores valid and dirty state plus its tag; simulated data bytes are not retained.
- `seen_set_t` is an open-addressed hash set with a separate occupancy array, so block ID zero is a valid key. It grows as entries are inserted.
- `workload_t` owns its streaming position, deterministic PRNG state, and optional trace file.

No mutable global state is used. Workload accesses are produced one at a time rather than saved in an array.

## Access flow

```text
memory access
     |
split address into tag / index / offset
     |
inspect indexed cache line
     +-- valid matching tag --> hit; update write state and counters
     |
     +-- otherwise ----------> classify miss using seen-block set
                                  |
                                  +--> write-through store miss:
                                  |    write memory; do not allocate
                                  |
                                  +--> allocating access:
                                       evict valid line if present
                                       write back dirty victim if required
                                       read block from memory and fill line
                                       record block in seen set
```

## Write policies

`wb` selects write-back with write-allocate. A write hit marks the line dirty. On an allocating miss, a valid victim is counted as an eviction; a dirty victim also causes a writeback and memory write. The missed block is read into the cache, and a write miss leaves the new line dirty.

`wt` selects write-through with no-write-allocate. A write hit writes through to memory and leaves the line clean. A write miss writes memory without allocating or evicting. A read miss still fills a line, evicting a valid line without a writeback.

## Miss classification

The simulator checks whether a missed block ID has ever been inserted into the seen set. A block not yet in the set is classified as a compulsory miss; after the block has been brought into the cache once, a later miss for it is classified as conflict/capacity. With a direct-mapped cache the counter combines conflict and capacity misses. A write-through/no-allocate store miss is not inserted into the set because the block was not brought into the cache.

The set uses linear probing and doubles its slot capacity when insertion would exceed its load threshold. Occupancy is stored separately from keys so every `uint32_t` block ID, including zero, is representable.

## Timing model

The reported average memory access time is:

```text
AMAT = hit_time + miss_rate * miss_penalty
```

Total cycles are calculated as accesses multiplied by hit time plus misses multiplied by miss penalty. This simplified model does not account for write-buffer effects.
