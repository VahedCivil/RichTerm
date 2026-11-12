# API Reference (0.2.0)

## Headers

| Header | Main API |
|---|---|
| `<richterm/console.hpp>` | `Console`, `ConsoleOptions`, terminal and live-output helpers |
| `<richterm/format.hpp>` | `format`, `format_error`, `format_segments` |
| `<richterm/style.hpp>` | `Color`, `Style`, `ColorMode`, `TextStyle`, `on`, `colorize` |
| `<richterm/table.hpp>` | `Table`, `TableColumn`, `Alignment` |
| `<richterm/progress.hpp>` | `Progress`, `ProgressTask`, `TaskId` |
| `<richterm/status.hpp>` | `Status` |
| `<richterm/richterm.hpp>` | All public headers and version macros |

## Console

- `Console(std::ostream&, ConsoleOptions)` binds output to a stream.
- `print(text)` renders markup and appends a newline.
- `print(format_string, args...)` formats arguments and appends a newline.
- `write(...)` is the no-newline equivalent.
- `print(const Table&)` writes a table.
- `rule(title)` prints a horizontal rule.
- `is_terminal()` and `supports_live_output()` expose terminal detection.
- `render_markup(text, base_style)` returns rendered text without adding a newline.

On Windows, a real console output handle is initialized for UTF-8 and Virtual Terminal processing. CMake consumers using MSVC inherit `/utf-8`. This fixes box-drawing and spinner glyphs when the active terminal font supports them.

## Format engine

`richterm::format("{:.3f}", value)` returns a string. Replacement fields are automatic and consume arguments left-to-right. A field is `{}` or `{:spec}`. Positional and named fields are not available. See [FORMAT_SPECIFIERS.md](FORMAT_SPECIFIERS.md).

## Tables

`Table::add_column(header, alignment, min_width)` defines a column. `add_row(...)` appends a row and validates the number of cells. `render()` returns markup-capable text. `Console::print(table)` writes it to the configured stream.

## Progress

```cpp
richterm::Progress progress(console, 28, true, 12.0); // bar width, auto refresh, FPS
auto task = progress.add_task("Compile", 100.0);
progress.start();
progress.update(task, 30.0);
progress.advance(task, 5.0);
progress.stop();
```

On a recognized terminal, `start()` launches a small refresh thread and redraws the task frame in place. Task changes are synchronized; `tasks()` returns a snapshot copy. `stop()` joins the worker and leaves the final frame visible. `auto_refresh=false` disables the worker. When output is redirected, Progress prints an initial snapshot at `start()`, a final snapshot at `stop()`, and intermediate snapshots on explicit `refresh()` calls. It never emits cursor-control sequences in this mode.

The refresh rate must be finite and within `(0, 1000]` frames per second. Totals and completion values must be finite and non-negative. A zero total is displayed as complete.

## Status

```cpp
richterm::Status status(console, "Compiling", 10.0);
status.start();
status.update("Running tests");
status.stop("All checks passed");
```

Status displays an animated Unicode spinner on interactive terminals. On redirected streams it emits ordinary log lines. The refresh rate must be finite and within `(0, 1000]`.

## Threading and lifetime

Progress and Status own their worker threads and join them in `stop()` and their destructors. Destroy these objects before the referenced `Console`; the Console and its output stream must outlive active renderers. Avoid destroying a renderer from its own worker callback. Console serializes writes made through its API.

## Error behavior

Malformed format strings, incompatible types, missing/extra arguments, invalid specs, table row-size mismatches and invalid progress values throw standard exceptions (`format_error`, `invalid_argument`, or `out_of_range`).
