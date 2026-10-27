# Project 3 integration verification — part 6

Verified on October 26, 2026, with GCC 15.2.0 in WSL Ubuntu, using our own
Project 2 Conversation and SentinelScanner. The tested implementation is
commit 71df8fd (part 5); part 6 only records verification and updates docs.

## Reproduce

From the repository root in Linux:

```bash
cmake -S . -B build-p3-integration \
  -DUSE_P2_REFERENCE=OFF -DMH_WERROR=ON -DMH_SANITIZE=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p3-integration -j 4
(cd build-p3-integration && \
  ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --output-on-failure -V)
```

The directory was new for this run. C++17 compilation used -Wall -Wextra
-Wpedantic -Werror, AddressSanitizer, and UndefinedBehaviorSanitizer.
The build and all tests exited successfully with no compiler warnings or
sanitizer diagnostics. The local detailed log is generated at
build-p3-integration/Testing/Temporary/LastTest.log (ignored by Git).

## Results

| CTest suite | Cases | Result |
|---|---:|---|
| p2_regression | 14 | Passed |
| p3 | 31 | Passed |
| tool_loop | 9 | Passed |
| e2e | 7 CLI sessions | Passed |

CTest reported 4/4 suites passing, with zero failures.

The P3 suite contains 13 registry, 11 workspace, and 7 calculator cases.
Registry measurements remain 16,384 buckets, load 0.610352, and longest chain
6 for 10,000 tools. The weak-hash comparison has a longest chain of 9,000.
The calculator tests include 20,000 terms and 5,000 nested parentheses.

The provided tool-loop tests cover model/tool/model execution without another
user turn, recovery from workspace rejection, chained tool rounds, the step
limit, no-tools behavior, sentinel priority, model-client errors, tool names
in the system message, and transcript save/load/replay with tool results.

All seven provided CLI sessions matched their golden files: escape_attempt,
greeting, malformed_calls, runaway, split_test, tools_demo, and tools_list.
The tools demo exercises all four built-in tools. The provided CLI runner
uses a fresh temporary workspace and normalizes error wording before comparing
outputs, including exit status. No expected outputs were regenerated.

## Scope and remaining work

No integration fixes were necessary. The provided implementation and test
fixtures were unchanged. The build artifacts and temporary transcripts are
ignored, and the tracked demo workspace was not modified by the tests.

This completes part 6. Part 7 still needs the final documentation/design-log
revision; part 8 needs the final submission audit and regenerated github.zip.
The current design log is deliberately a development draft, not yet the final
500–800-word submission document.
