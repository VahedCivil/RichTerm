# RichTerm

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Header-only](https://img.shields.io/badge/library-header--only-informational.svg)](#features)

**RichTerm is a lightweight, modular C++20 terminal presentation library for colorful console output, format strings, BBCode-style markup, tables, animated progress bars, and status spinners.**

It is an independently implemented C++ library inspired by the user experience of [Rich for Python](https://github.com/Textualize/rich) and the color/formatting workflow of [ColormaCpp](https://github.com/VahedCivil/ColormaCpp). It is not an official port of Rich and is not affiliated with Textualize.

> Make terminal output useful, readable, and expressive — without adding third-party runtime dependencies.

## Features

- **Console** — formatted `print`, `println`, and `write`; terminal-aware ANSI color selection; terminal-width detection; markup rendering; section rules.
- **Format strings** — automatic `{}` fields, escaped braces, numeric format specifications, alignment, fill, signs, zero padding, grouping, precision, integer bases, floating-point notation, and percentage output.
- **BBCode-style markup** — nested tags such as `[bold cyan]...[/bold cyan]`, `[u]...[/u]`, `[i]...[/i]`, color names, and `on` background colors.
- **Colorma-style tokens** — 16 standard/bright foreground colors, 16 backgrounds, bold, dim, italic, underline, reverse, strike, reset, and automatic/always/never color modes.
- **Tables** — Unicode box drawing, terminal-cell-aware width measurement for common Unicode ranges, aligned columns, optional wrapping with a maximum width, header/caption/title, and independently styled borders and row separators.
- **Panels** — titled bordered content blocks with configurable width, padding, alignment, and styles.
- **Date and time** — `std::chrono`-based date/time formatting and human-readable duration formatting.
- **Progress** — multiple tasks, determinate bars, indeterminate pulse animation, elapsed time, synchronized updates and live terminal refresh.
- **Status** — animated Unicode spinner on interactive terminals, with readable log output when redirected.
- **CMake package** — exported `RichTerm::RichTerm` target and installable headers.
- **No third-party dependencies** — header-only implementation using the C++ standard library.

## Scope and compatibility

RichTerm intentionally implements a focused subset of Rich's terminal-facing features. The first version focuses on the Console, formatting and styling, tables, progress, and status workflows requested for this project.

The following Rich features are **not included** in this release: Markdown rendering, Pygments syntax highlighting, tracebacks, full-screen live layouts, Jupyter integration, SVG/HTML export, emoji substitution, automatic layout/renderable protocols, and Rich's full theme/highlighter ecosystem. Progress and Status implement focused live terminal animations, not the complete Rich Live subsystem. These features would need additional subsystems or dependencies and are outside the current tested scope.

The format engine follows a practical subset of Python's format mini-language and familiar C++ formatting conventions. It is not a full implementation of Python `str.format`, `std::format`, or the `{fmt}` library. Positional/named fields, locale-aware `n`, and custom formatter specializations are not implemented. Date/time presentation is provided separately by the `datetime.hpp` helpers.

## Requirements

- C++20 compiler.
- CMake 3.20 or newer for the CMake workflow.
- No third-party libraries.

The revised source bundle has been built and tested locally with GCC 14.2 and CMake on Linux. Windows-specific UTF-8 setup and MSVC `/utf-8` options are included, but this environment cannot run a real MSVC/Windows terminal test; run the included GitHub Actions matrix before claiming Windows visual compatibility.

## Quick start

```cpp
#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;

    console.print("Hello, [bold cyan]RichTerm[/bold cyan]!");
    console.print("decimal={:,.2f}, hex={:#08x}, percent={:.1%}",
                   1234567.891, 48879, 0.875);

    console.print("{}Success{}: {} checks passed.",
                   richterm::Color::bright_green,
                   richterm::Style::reset,
                   12);
}
```

### Example output

```text
Hello, RichTerm!
decimal=1,234,567.89, hex=0x00beef, percent=87.5%
Success: 12 checks passed.
```

## Build, test, and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run the showcase:

```bash
# Linux / macOS
./build/examples/richterm_showcase

# Windows (Visual Studio multi-configuration generator)
.\build\examples\Release\richterm_showcase.exe
```

### Visual Studio 2026

CMake 4.2 or newer provides the `Visual Studio 18 2026` generator. With that version installed, configure from Developer PowerShell:

```powershell
cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
cmake --build build-vs2026 --config Release --parallel
ctest --test-dir build-vs2026 -C Release --output-on-failure
```

The project itself requires CMake 3.20; the higher version is specifically for selecting the VS 2026 generator by name. See the [official CMake VS 2026 generator documentation](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2018%202026.html).

To build without examples or tests:

```bash
cmake -S . -B build -DRICHTERM_BUILD_EXAMPLES=OFF -DRICHTERM_BUILD_TESTS=OFF
cmake --build build --config Release
```

## Install and use from another CMake project

Install to a custom prefix:

```bash
cmake --install build --prefix ./install
```

Then use the exported target:

```cmake
find_package(RichTerm CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE RichTerm::RichTerm)
```

RichTerm is header-only; no separate runtime library needs to be deployed.

## Console and markup

```cpp
#include <richterm/richterm.hpp>

richterm::Console console;
console.rule("Build report");
console.print("Where there is a [bold cyan]Will[/bold cyan] there [u]is[/u] a [i]way[/i].");
console.print("[red]Error[/red], [yellow]warning[/yellow], [green]success[/green]");
console.write("This stays on the current line... ");
console.println("done");

// Disable markup when literal square-bracket text should be preserved.
richterm::ConsoleOptions options;
options.color = richterm::ColorMode::never;
options.markup = false;
richterm::Console plain_console(std::cout, options);
plain_console.print("[not markup]");
```

Supported markup tags include `bold`, `dim`, `italic`/`i`, `underline`/`u`, `reverse`, `strike`, the 16 basic/bright color names, and background syntax such as `[black on bright_green]...[/black on bright_green]`. Nested tags are supported when they are closed in last-in, first-out order. Use `[/]` to close the most recently opened tag. `[[` and `]]` escape literal square brackets, and `Console::escape_markup()` escapes both bracket characters in an input string.

Automatic color mode emits ANSI only for recognized terminal streams and respects `NO_COLOR` and `TERM=dumb`. Use `ColorMode::always` to force ANSI or `ColorMode::never` to suppress it. On Windows, real console handles are switched to UTF-8 and Virtual Terminal mode is enabled. The exported CMake target passes `/utf-8` to MSVC consumers so Unicode box-drawing and spinner literals are compiled correctly.

## Format strings and numeric values

RichTerm uses automatic replacement fields and left-to-right argument consumption:

```cpp
using richterm::format;

std::string label = "OpenSees";
std::string message = format("Project: {:>12s} | id={:05d}", label, 42);

console.print("int={:d}, hex={:#x}, oct={:#o}, bin={:#b}", 31, 31, 31, 31);
console.print("fixed={:.8f}, sci={:.3e}, general={:.5g}",
               3.141592653589793, 3.141592653589793, 3.141592653589793);
console.print("signed={:+d}, grouped={:,}, ratio={:.2%}", 17, 123456789, 0.875);
console.print("left=|{:<8}| center=|{:^8}| right=|{:>8}|", "C++", "C++", "C++");
console.print("zero={:08d}, custom={:*>8s}", 42, "OK");
```

Format specification grammar:

```text
[[fill]align][sign][#][0][width][grouping][.precision][type]
```

| Component | Supported values |
|---|---|
| Alignment | `<` left, `>` right, `^` center, `=` sign-aware numeric padding |
| Sign | `-`, `+`, space |
| Alternate form | `#` for integer prefixes and alternate floating output |
| Zero padding | `0` |
| Grouping | `,` or `_` |
| Precision | `.N` for strings/floating point |
| Integer types | `d`, `x`, `X`, `o`, `b`, `B`, `c` |
| Floating types | `f`, `F`, `e`, `E`, `g`, `G`, `a`, `A`, `%` |
| String type | `s` |

Escaped braces are written as `{{` and `}}`. Invalid fields, unsupported types, missing arguments, extra arguments, null C strings, and malformed format specifications throw `richterm::format_error` before Console writes the formatted result. String precision counts bytes, not Unicode grapheme clusters.

## Colors and styles

```cpp
using namespace richterm;

console.print("{}PASS{}: {} tests", Color::bright_green, Style::reset, 24);
console.print("{}warning{}", Style::bold + Color::bright_yellow, Style::reset);
console.print("{}dark text on green{}",
              Color::black + on(Color::bright_green) + Style::bold,
              Style::reset);

// Explicit color output helper.
const auto emphasized = colorize("Important", Color::bright_white,
                                 Color::red, {Style::bold, Style::underline},
                                 ColorMode::always);
```

`ColorMode` values are `automatic`, `always`, and `never`. ANSI output is reset at each rendered segment so styles do not leak into later console text.

## Tables

```cpp
richterm::Table table("Analysis results");
table.add_column("Model")
     .add_column("Status", richterm::Alignment::center)
     .add_column("Period (s)", richterm::Alignment::right)
     .add_row("Elastic", "[green]OK[/green]", 0.125)
     .add_row("Nonlinear", "[yellow]CHECK[/yellow]", 1.875)
     .set_caption("Two analysis cases");

console.print(table);          // or console.print(table.render())
```

Tables calculate widths using terminal-cell widths for common combining marks, East Asian wide/full-width characters, and emoji sequences. Set `Table::set_max_width()` to constrain the table and wrap cells; `set_line_style()` controls the color of row separator lines independently from the outer border. Unicode width handling is based on compact built-in ranges rather than a complete generated Unicode database, so unusual scripts and terminal-specific emoji presentation may still differ between terminals.

## Panels

```cpp
richterm::Panel panel("This content wraps to fit the panel width.", "Build summary");
panel.set_width(42).set_padding(1).set_border_style("bright_blue")
     .set_title_style("bold yellow").set_alignment(richterm::Alignment::left);
console.print(panel);
```

`Panel` supports a title, configurable total width, inner padding, content alignment, and separate border/title/content styles. If no width is set, the panel sizes itself to its content.

## Date, time, and durations

```cpp
#include <chrono>

const auto now = std::chrono::system_clock::now();
console.print("Date: {}", richterm::format_date(now));
console.print("Time: {}", richterm::format_time(now));
console.print("Timestamp: {}", richterm::format_datetime(now, "%Y-%m-%d %H:%M:%S"));
console.print("Elapsed: {}", richterm::format_duration(std::chrono::seconds{3661}));
```

The date/time helpers use the process local time zone and `std::put_time` format directives. `format_duration()` accepts standard chrono durations.

## Console pause and clear

```cpp
console.print("Preparing...");
console.pause(0.5); // seconds
console.clear();     // clears an interactive terminal; no-op for redirected streams
```

`pause()` also accepts a `std::chrono::duration`. Negative or non-finite floating-point delays are rejected. `clear()` emits terminal control sequences only when the configured stream is an interactive terminal.

## Progress color

```cpp
richterm::Progress progress(console);
progress.set_bar_color(richterm::Color::bright_green);
progress.set_pulse_color(richterm::Color::bright_cyan);
```

## Visual examples and media

Each example is a separate source file under `examples/` and can be built/run independently. Store screenshots in `docs/images/` and animated terminal recordings in `docs/gifs/`; each directory contains a short naming guide. Suggested captures include the console, table, and panel examples, plus progress/status animations.

## Progress and status

On an interactive terminal, Progress redraws its frame in place and Status runs an animated spinner. Both use a small owned worker thread; `stop()` joins it. When output is redirected to a file, pipe, IDE output pane, or CI log, cursor animation is disabled. Progress writes an initial and final snapshot (and additional snapshots when `refresh()` is called), while Status writes ordinary log lines.

```cpp
richterm::Progress progress(console, 28, true, 12.0); // width, auto-refresh, refresh FPS
const auto download = progress.add_task("Download", 100.0);
progress.start();
for (int value = 0; value <= 100; value += 5) {
    progress.update(download, static_cast<double>(value));
    // Do useful work here; the renderer refreshes independently.
}
progress.stop();

richterm::Status status(console, "Compiling project", 10.0);
status.start();
status.update("Running tests");
status.stop("All checks passed");
```

Tasks can be added or updated while Progress is active. Unknown totals display a moving pulse segment. The refresh rate must be in `(0, 1000]`. The `Console` and output stream must outlive all Progress and Status objects that refer to them. Use the integrated terminal or Windows Terminal for live animation; some IDE output panes do not expose a true interactive TTY.

## Project layout

```text
RichTerm/
├── include/
│   ├── RichTerm.hpp
│   └── richterm/
│       ├── richterm.hpp     # umbrella include and version macros
│       ├── console.hpp      # Console, markup parser, terminal detection
│       ├── datetime.hpp     # std::chrono date/time and duration formatting
│       ├── unicode.hpp      # UTF-8 decoding and terminal-cell width helpers
│       ├── panel.hpp        # bordered content panels
│       ├── format.hpp       # format engine and numeric presentation
│       ├── style.hpp        # colors, ANSI styles, colorize helpers
│       ├── table.hpp        # Unicode tables and cell alignment
│       ├── progress.hpp     # live progress tasks and animated bars
│       └── status.hpp       # animated status spinner and log fallback
├── examples/                # standalone examples for individual features
├── docs/images/             # screenshot destination
├── docs/gifs/               # animated demo destination
├── tests/                   # CTest regression tests
├── cmake/                   # package config template
├── docs/                    # API and format reference
└── .github/workflows/        # CI matrix
```

## Relationship to Rich and ColormaCpp

- **Rich for Python:** [Textualize/Rich](https://github.com/Textualize/rich) is the upstream Python project that inspired the terminal-oriented feature set and the BBCode-like markup example. RichTerm does not bundle its Python source or claim API parity.
- **ColormaCpp:** [VahedCivil/ColormaCpp](https://github.com/VahedCivil/ColormaCpp) is the related C++ color and formatting project. RichTerm independently implements compatible concepts: color modes, foreground/background colors, common text attributes, inline style tokens, and brace-based numeric formatting.

## Contributing

Issues, bug reports, documentation fixes, tests, and focused feature contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md), open an issue for large changes, and include regression tests with bug fixes. The project welcomes contributors who want to extend supported formatting types, terminal compatibility, table layout, and progress reporting.

## License

RichTerm 0.3.0 is released under the MIT License. See [LICENSE](LICENSE).

## Search keywords

`C++ terminal library`, `C++ console styling`, `C++ rich console`, `Rich for C++`, `C++ BBCode markup`, `C++ ANSI colors`, `C++ terminal colors`, `C++ format strings`, `C++ numeric formatting`, `C++ console tables`, `C++ progress bar`, `C++ status messages`, `CMake C++ library`, `header-only C++20`, `ColormaCpp`, `Textualize Rich`.
