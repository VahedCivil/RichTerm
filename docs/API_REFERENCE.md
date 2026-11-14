# API Reference (0.3.0)

## Headers

| Header | Main API |
|---|---|
| `<richterm/console.hpp>` | `Console`, `ConsoleOptions`, terminal and live-output helpers |
| `<richterm/format.hpp>` | `format`, `format_error`, `format_segments` |
| `<richterm/style.hpp>` | `Color`, `Style`, `ColorMode`, `TextStyle`, `on`, `colorize` |
| `<richterm/table.hpp>` | `Table`, `TableColumn`, `Alignment` |
| `<richterm/panel.hpp>` | `Panel` |
| `<richterm/datetime.hpp>` | `format_datetime`, `format_date`, `format_time`, `format_duration` |
| `<richterm/unicode.hpp>` | UTF-8 and terminal-cell width helpers (implementation support) |
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
- `pause(seconds)` or `pause(std::chrono::duration)` pauses execution.
- `clear()` clears an interactive terminal and does nothing for redirected streams.
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


## Markup extensions

- `[/]` closes the most recently opened markup tag.
- `Console::escape_markup(text)` escapes square brackets so input is printed literally.

## Console helpers

- `pause(double seconds)` pauses for a finite, non-negative number of seconds.
- `pause(std::chrono::duration)` accepts a standard chrono duration.
- `clear()` clears and homes an interactive terminal; redirected streams are unchanged.

## Table wrapping and line styles

- `set_max_width(width)` constrains the table width and wraps cell contents when needed.
- `set_line_style(style)` selects the markup style for row separator lines independently of the outer border style.

## Panel

`Panel(content, title)` creates a bordered content block. Use `set_width`, `set_padding`, `set_alignment`, `set_border_style`, `set_title_style`, and `set_content_style` to customize it. `render()` returns markup-capable text, and `Console::print(panel)` writes it.

## Date and time

- `format_datetime(system_clock::time_point, pattern)` formats a timestamp using `std::put_time` directives.
- `format_date(...)` and `format_time(...)` provide date-only and time-only defaults.
- `format_duration(std::chrono::duration)` formats elapsed durations as `hours:minutes:seconds` (or `minutes:seconds`).

## Progress colors

- `set_bar_color(Color)` changes the completed portion of determinate bars.
- `set_pulse_color(Color)` changes the indeterminate pulse glyph.

