# Trace files

Trace files are UTF-8 text with one access per line. Each record is an access type (`R` for read or `W` for write), whitespace, then a hexadecimal byte address. The hexadecimal address may have a `0x` prefix. Blank lines and lines whose first non-whitespace character is `#` are ignored.

For example:

```text
# two accesses
R 0x10
W 20
```

Malformed records are reported with the trace filename and one-based line number. [sample_small.trace](./sample_small.trace) contains 30 mixed accesses, including addresses that conflict in a 1 KB direct-mapped cache when using the default 32-byte block size.
