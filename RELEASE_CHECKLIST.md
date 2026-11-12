# GitHub publication checklist

The repository `VahedCivil/RichTerm` was checked and was not found. No repository creation, push, tag, or GitHub Release was performed. The source tree includes a GitHub Actions workflow; it becomes active after the files are pushed to a newly created public repository.

1. Create a public repository named **RichTerm** under the desired GitHub account.
2. Extract the archive and upload/push the full project root, keeping the hidden `.github` directory and workflow files.
3. Apply the suggested topics from `GITHUB_TOPICS.txt` in the repository settings.
4. Open the repository's **Actions** tab and wait for the Windows, macOS, and Linux CI matrix to finish.
5. On Windows, configure and test with the VS 2026 generator using CMake 4.2+:

   ```powershell
   cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
   cmake --build build-vs2026 --config Release --parallel
   ctest --test-dir build-vs2026 -C Release --output-on-failure
   ```

6. Review terminal behavior on Windows Terminal and at least one POSIX terminal. In particular, verify Unicode table borders and color handling with redirected output.
7. Only after CI and manual checks pass, create a version tag and publish a GitHub Release. The current `0.2.0` bundle is a development preview, not a claim of full Python Rich API parity.
