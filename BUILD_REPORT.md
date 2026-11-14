# RichTerm 0.3.0 — Build and regression report

## Implemented changes

- Added short BBCode closing tags (`[/]`) and literal markup escaping through `Console::escape_markup()`.
- Added UTF-8 decoding and terminal-cell width estimation for common combining marks, East Asian wide characters, regional-indicator flags and emoji ZWJ sequences.
- Added optional table maximum width, cell wrapping, and independently configurable row-separator styling.
- Added the `Panel` renderable with title, width, padding, alignment and style controls.
- Added `std::chrono` date/time and duration formatting helpers.
- Added configurable determinate Progress bar and indeterminate pulse colors.
- Added `Console::pause()` overloads and TTY-only `Console::clear()`.
- Added standalone feature examples, screenshot/GIF destination directories, updated API/architecture documentation, and version metadata for 0.3.0.

## Validation results

| Validation | Result |
|---|---|
| GCC 14.2, Release, examples and tests, `-Werror` | PASS |
| Clang 17, Release, examples and tests, `-Werror` | PASS |
| CTest regression suite | PASS (1/1) on both compilers |
| AddressSanitizer + UndefinedBehaviorSanitizer, GCC 14.2 | PASS |
| CMake install + separate `find_package(RichTerm)` consumer | PASS |
| Independent compilation of all public headers | PASS |
| Final source ZIP extracted, built with GCC `-Werror`, and tested | PASS |
| Windows/MSVC and macOS builds | Not run in this environment |
| Interactive Windows terminal visual inspection | Not run |

The local build/test results above were run against the extracted 0.3.0 working tree. The GitHub Actions workflow remains responsible for the Windows/macOS CI matrix after publication.

## Known limitations

- Unicode width support uses compact internal ranges and grapheme grouping for common cases; it is not a complete generated Unicode East Asian Width implementation and may differ from terminal-specific emoji rendering.
- Table wrapping is opt-in through `Table::set_max_width()`.
- Date/time formatting uses the process local time zone; no arbitrary time-zone database integration is included.
- `Console::clear()` intentionally does nothing for redirected streams.
- Windows UTF-8/VT code is present but still requires real Windows/MSVC and terminal verification.
