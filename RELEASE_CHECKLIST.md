# GitHub publication checklist — RichTerm 0.3.0

This checklist describes publication steps; no repository push, tag, or GitHub Release is performed by building the source archive.

1. Confirm that the intended GitHub repository exists and that you have permission to push to it.
2. Extract `RichTerm-0.3.0-source.zip` and inspect the complete project root, including the hidden `.github` directory.
3. Review the final source diff, README examples, version metadata, changelog, license and release notes.
4. Apply the repository topics listed in `GITHUB_TOPICS.txt`.
5. Configure and build locally:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --parallel
   ctest --test-dir build -C Release --output-on-failure
   ```

6. Push the source to the intended branch (for example, `main`) and wait for the GitHub Actions matrix (Linux, Windows and macOS; Debug and Release) to pass.
7. Manually verify Windows Terminal UTF-8/ANSI rendering and at least one POSIX terminal. Also check redirected output.
8. Only after CI and manual checks pass, create and push the version tag:

   ```bash
   git tag -a v0.3.0 -m "RichTerm 0.3.0"
   git push origin v0.3.0
   ```

9. Create a GitHub Release for `v0.3.0`, attach the source ZIP, and summarize the changes from `CHANGELOG.md`.

The project is a focused C++20 terminal library; this release does not claim full API parity with Python Rich. Windows visual behavior must be confirmed on a real Windows terminal before release publication.
