# RichTerm 0.2.0 — Build and regression report

## Repairs and enhancements

- Fixed Windows Unicode table/output handling: real standard console handles switch to UTF-8, Virtual Terminal Processing is enabled, and MSVC consumers inherit `/utf-8` from the CMake interface target.
- Restricted Windows code-page/VT changes to `std::cout`, `std::cerr` and `std::clog`; custom string/file streams are not used to mutate console settings.
- Serialized Console writes to prevent concurrent worker output from interleaving with ordinary RichTerm writes.
- Implemented live animated Progress rendering on TTYs, with determinate bars, indeterminate pulse, elapsed time and a refresh worker.
- Implemented live animated Status spinner on TTYs.
- Retained non-interactive behavior: no cursor-control sequences for redirected streams; Progress writes initial/final snapshots and optional intermediate snapshots through `refresh()`, while Status emits log lines.
- Added task synchronization, validated refresh rates, joined worker threads in `stop()`/destructors, and linked `Threads::Threads` transitively through the installed CMake package.
- Updated the showcase, API documentation, Persian/English READMEs and release notes.

## Validation results

| Validation | Result |
|---|---|
| GCC 14.2, Debug, `-Werror` | PASS |
| Clang 17, Release, `-Werror` | PASS |
| CTest regression suite | PASS |
| AddressSanitizer + UndefinedBehaviorSanitizer | PASS |
| Interactive TTY smoke test | PASS — emitted 323 clear-line frames, 101 cursor-up operations, spinner and progress glyphs |
| CMake install + separate `find_package(RichTerm)` consumer | PASS |
| Build from extracted release ZIP | To be re-run on final archive before packaging |

## Platform caveat

The code includes Windows UTF-8 and MSVC `/utf-8` fixes, but this environment cannot run the user's Visual Studio 2026/MSVC toolchain or visually inspect Windows Terminal. The included GitHub Actions workflow covers Windows, Linux and macOS once the project is pushed. Run the Windows CI job and inspect `richterm_showcase` in Windows Terminal or the VS Code integrated terminal before declaring Windows rendering fully verified.

No software project can honestly be guaranteed to contain zero bugs. The known issues visible in the supplied screenshot have been addressed, and the validations above describe exactly what was tested.
