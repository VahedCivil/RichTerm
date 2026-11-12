# RichTerm format specifiers

Grammar:

```text
[[fill]align][sign][#][0][width][grouping][.precision][type]
```

| Token | Meaning | Example |
|---|---|---|
| `<` | Left alignment | `{:<10}` |
| `>` | Right alignment | `{:>10}` |
| `^` | Center alignment | `{:^10}` |
| `=` | Pad after sign/base prefix | `{:=+08d}` |
| `fill` | Single fill character before alignment | `{:*>8}` |
| `+` | Always show numeric sign | `{:+d}` |
| space | Leading space for non-negative numbers | `{: d}` |
| `#` | Alternate representation/prefix | `{:#x}` |
| `0` | Sign-aware zero padding | `{:08d}` |
| `,` / `_` | Decimal digit grouping | `{:,}` / `{:_}` |
| `.N` | Precision | `{:.4f}` |

Integer types: `d`, `x`, `X`, `o`, `b`, `B`, `c`.

Floating-point types: `f`, `F`, `e`, `E`, `g`, `G`, `a`, `A`, `%`.

String type: `s`.

Examples:

```cpp
richterm::format("{:d} {:#x} {:#b}", 31, 31, 31);      // 31 0x1f 0b11111
richterm::format("{:+08d}", 42);                       // +0000042
richterm::format("{:,}", 123456789);                   // 123,456,789
richterm::format("{:_}", 123456789);                   // 123_456_789
richterm::format("{:.6f}", 3.141592653589793);          // 3.141593
richterm::format("{:.2%}", 0.875);                      // 87.50%
richterm::format("|{:^8s}|", "C++");                   // |  C++   |
```

`{{` and `}}` produce literal braces. String precision counts bytes. Width calculations for formatting count bytes as well; terminal display width for multibyte glyphs is not fully implemented.
