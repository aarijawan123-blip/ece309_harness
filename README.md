# ECE 309 MiniHarness — Project 3

Project 3 extends the C++17 conversation harness with a tool registry and a
provided tool execution loop. Development is being completed in separate parts.

## Current progress

Parts 1–6 are complete: P3 starter migration, the tool registry, workspace
path resolution, calculator evaluation, and full integration testing. All four
CTest suites pass. Final documentation and the submission audit remain.

## Build on Linux / WSL

```bash
cmake -S . -B build-p3 -DUSE_P2_REFERENCE=OFF -DMH_WERROR=ON -DMH_SANITIZE=ON
cmake --build build-p3 -j 4
```

This uses our P2 code, C++17, -Wall -Wextra -Wpedantic -Werror, AddressSanitizer,
and UndefinedBehaviorSanitizer. Use a fresh build-p3 directory rather than an
old Project 2 build cache.

Run the checks for the completed parts:

```bash
(cd build-p3 && ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./test_p2)
(cd build-p3 && ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./test_p3 reg_)
(cd build-p3 && ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./test_p3 ws_)
(cd build-p3 && ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./test_p3 calc_)
(cd build-p3 && ./test_loop =loop_escape_attempt_is_an_error_not_a_crash)
./build-p3/miniharness --tools
./build-p3/miniharness --no-tools --script scripts/greeting.script
```

The registry tests print the 10,000-tool measurements and weak-hash comparison.
The complete run passes 14 P2 tests, 31 P3 tests (13 registry, 11 workspace,
7 calculator), 9 tool-loop tests, and 7 golden CLI sessions. No compiler
warnings or ASan/UBSan findings were reported.

To run all suites together:

```bash
(cd build-p3 && ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --output-on-failure)
```

See [the part 6 verification record](docs/integration-p3.md) for the fresh-build
command, coverage, and results.

## Layout

- `include/core/`, `src/core/`: our P2 container/scanner; provided P3 Message.
- `include/tools/`, `src/tools/`: registry, workspace, calculator, and built-in tools.
- `include/harness/`, `src/harness/`: provided harness, dispatcher, transcripts.
- `include/model/`, `src/model/`, `include/json/`, `src/json/`: provided clients and JSON.
- `tests/p2/`: provided P3 regression suite for P2 behavior.
- `tests/p3/`: student unit tests and provided tool-loop tests.
- `tests/e2e/`: provided CLI inputs, expected output, and isolated test workspace.
- `reference/p2/`: instructor fallback, not used in our build.
- `scripts/`, `workspace/`: provided demonstrations and sample files.
- `P3_spec.md`: attached assignment specification.
- `docs/design-log-p3.md`: design notes and measured results, in progress.

## Registry design

The registry owns tools with unique_ptr. A vector of linked lists provides
separate chaining: hash the full name, choose a bucket, then compare names in
that bucket. It starts with 16 buckets and doubles before an insertion would
exceed a load factor of 1. Resizing transfers list nodes; Tool objects keep their
addresses. Null and duplicate registrations return false, and a duplicate's new
tool is destroyed while the original remains registered.

## Workspace path checks

File paths are relative to the workspace. The resolver rejects empty paths,
NUL bytes, absolute/rooted paths, and normalized paths beginning with `..`.
It accepts `a/../b` as `b`, `a/..` as the workspace root, and ordinary names such
as `my..notes.txt`. New paths can resolve even when their directories do not
exist: normalization only examines text. The provided file tools then perform
the separate symlink check before accessing the filesystem.

## Calculator

The provided tokenizer reads numbers and operators. Our evaluator uses one
stack for values and another for operators. Multiplication and division have
higher precedence; equal-precedence operations run left to right. Parentheses
are handled with loops, so deeply nested expressions do not require recursion.
Negative numbers are handled by the tokenizer. Division by zero returns an
error to the model. Calculator tests include the required examples, malformed
token lists, 20,000 terms, and 5,000 nested parentheses.

## Earlier projects and remaining work

Project 1 files remain at the root and are not part of the CMake build. The
previous Project 2 README is archived in `docs/project2-readme.md`; its paths and
commands describe the old layout. Old P2 tests are preserved in `tests/p2_legacy/`
and are not current CMake targets. The existing P2 documentation is historical.
The P3 starter README is preserved in `docs/p3-starter-README.md`.

Next: finish the 500–800-word design log, finalize the documentation, and
audit the submission.
`github.txt` retains the real repository URL. Any existing `github.zip` is the
older P2 backup; it must be regenerated for the final P3 submission.
