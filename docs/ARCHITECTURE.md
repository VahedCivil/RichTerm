# Architecture

RichTerm is a header-only C++20 library. `richterm.hpp` is the umbrella include.

## Modules

- **style.hpp** — strongly typed color/style tokens and ANSI SGR sequences.
- **format.hpp** — validated automatic-field format parser; formatting completes in memory before Console commits output.
- **console.hpp** — stream binding, terminal detection, ANSI policy, BBCode-like markup, UTF-8 Windows console initialization, serialized output and live line/frame rewriting.
- **table.hpp** — cell storage, Unicode terminal-cell sizing, wrapping, alignment, box drawing and markup generation.
- **unicode.hpp** — UTF-8 decoding, compact terminal-width ranges, grapheme-aware width and wrapping helpers.
- **panel.hpp** — bordered content rendering built on the table/text layout helpers.
- **datetime.hpp** — local date/time and duration presentation using `std::chrono` and `std::put_time`.
- **progress.hpp** — synchronized task state, determinate bars, indeterminate pulse animation and an optional refresh worker for TTY output.
- **status.hpp** — animated spinner for TTYs and log-friendly output for redirected streams.

## Design decisions

1. **No third-party runtime dependencies.** Only the C++ standard library and small Windows/POSIX terminal APIs are used. CMake links the platform thread target.
2. **Header-only distribution.** CMake exports one interface target and installs the public headers.
3. **TTY-aware interaction.** Live redraw and cursor control are enabled only for recognized terminal streams. Redirected output remains readable and does not contain cursor-control codes.
4. **Thread ownership.** Progress and Status own and join their worker threads. Console serializes output operations to avoid interleaving with other RichTerm writes.
5. **UTF-8 on Windows.** The Console switches real Windows console handles to UTF-8 and enables Virtual Terminal processing. MSVC consumers receive `/utf-8` through the CMake target.
6. **Explicit scope.** Full terminal layout, Markdown, syntax highlighting, tracebacks and HTML/SVG export remain outside the supported subset.

## Current limitations

- The format grammar is a focused subset and does not support positional/named fields, locale-dependent formatting, dates or custom formatter registration.
- String precision/alignment use byte counts. Table width uses compact built-in Unicode ranges and grapheme grouping for common combining/emoji sequences; it is not a complete generated Unicode width database.
- Table cell wrapping is opt-in through `set_max_width()`; width measurement uses compact built-in Unicode ranges and is not a complete implementation of the Unicode East Asian Width database.
- Live rendering requires a terminal that supports ANSI cursor controls and a font with the required Unicode glyphs. Visual Studio's output pane and some embedded consoles may not provide a true TTY; use the integrated terminal or Windows Terminal.
- The upstream Python Rich API is not fully replicated.
