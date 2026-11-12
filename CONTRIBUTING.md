# Contributing to RichTerm

Contributions are welcome. Before opening a pull request:

1. Create a focused branch and describe the problem or feature.
2. Keep public APIs documented and preserve source compatibility where practical.
3. Add regression tests for bug fixes and tests for new behavior.
4. Configure with CMake, build all targets, and run `ctest --output-on-failure`.
5. Use clear commit messages and avoid unrelated formatting changes.
6. For Progress/Status changes, test both a real TTY (live animation) and redirected output (plain logs).
7. Keep the Console and its stream alive until all Progress/Status objects using them have stopped or been destroyed.

Please open an issue first for large API changes. By contributing, you agree that your contributions may be distributed under the MIT License included in this repository.
