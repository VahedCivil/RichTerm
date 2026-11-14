# Changelog

## 0.3.0 — Panels, Unicode tables and richer console helpers

- Added short BBCode closing tag `[/]` and a helper for escaping literal markup text.
- Added UTF-8 decoding and terminal-cell width handling for common combining marks, East Asian wide characters and emoji sequences.
- Added optional table width constraints, cell wrapping, and independently configurable row separator styling.
- Added the `Panel` renderable with title, width, padding, alignment and style options.
- Added `std::chrono` date/time and duration formatting helpers.
- Added configurable Progress bar and pulse colors.
- Added `Console::pause()` and interactive-terminal `Console::clear()`.
- Added standalone examples for individual features and screenshot/GIF destination directories.
- Updated version metadata, documentation and regression tests.

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
