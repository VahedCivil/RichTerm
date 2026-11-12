# Changelog

## 0.2.0 — Live terminal rendering and Windows UTF-8 fix

- Fixed Windows console output encoding by switching real console handles to UTF-8 and enabling Virtual Terminal processing.
- Serialized Console writes so background renderers do not interleave RichTerm output.
- Added animated in-place Progress frames for interactive terminals, including indeterminate pulse bars and elapsed time.
- Added animated Status spinner for interactive terminals.
- Kept plain, stable snapshots/log lines for redirected streams and CI.
- Added synchronized progress task snapshots, refresh-rate validation and CMake thread dependency propagation.
- Updated showcase and regression tests for the new behavior.

## 0.1.0 — Initial preview

- Added modular C++20 headers for Console, formatting, styles, tables, progress and status.
- Added CMake install/export support, examples, unit tests and CI configuration.
- Added Rich-style BBCode subset and Colorma-inspired color/style format tokens.
