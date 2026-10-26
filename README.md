# ECE 309 MiniHarness — Project 3

Project 3 extends the C++17 conversation harness with a tool registry and a
provided tool execution loop. Development is being completed in separate parts.

## Current progress

Parts 1–4 are complete: starter setup, migration of our own Project 2
Conversation and SentinelScanner, and the hash-table ToolRegistry with resizing
and measurements, plus Workspace::resolve and path-safety tests. The provided
file tools now work with the resolver. Calculator evaluation is still a starter
stub (part 5).
The full CTest suite is therefore not expected to pass at this checkpoint.

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
(cd build-p3 && ./test_loop =loop_escape_attempt_is_an_error_not_a_crash)
./build-p3/miniharness --tools
./build-p3/miniharness --no-tools --script scripts/greeting.script
```

The registry tests print the 10,000-tool measurements and weak-hash comparison.
The current results are 14/14 P2 regression tests, 13/13 registry tests,
11/11 workspace tests, and the provided escape-attempt tool-loop test.
No compiler warnings or sanitizer errors were reported. Tool listing and the
no-tools greeting were also compared with the supplied golden output. The
escape-attempt CLI session passes its golden comparison too.

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

## Earlier projects and remaining work

Project 1 files remain at the root and are not part of the CMake build. The
previous Project 2 README is archived in `docs/project2-readme.md`; its paths and
commands describe the old layout. Old P2 tests are preserved in `tests/p2_legacy/`
and are not current CMake targets. The existing P2 documentation is historical.
The P3 starter README is preserved in `docs/p3-starter-README.md`.

Next: implement calculator evaluation, run the full
integration suite, finish the 500–800-word design log, and audit the submission.
`github.txt` retains the real repository URL. Any existing `github.zip` is the
older P2 backup; it must be regenerated for the final P3 submission.
