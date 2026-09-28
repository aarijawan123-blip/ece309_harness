# Project 2 final verification

The final audit used the complete supplied `P2_spec_v3.md` and
`ece309-project2-starter.zip`. No implementation changes were needed in Part 7.

## Build and test results

Fresh out-of-source Debug and Release builds ran in Ubuntu WSL with GCC 15.2.0.
Both used the supplied CMake configuration plus `-Werror`; compiler diagnostics
were clean. AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection
were enabled. Both builds passed all 26 C++ test groups and all three CLI checks,
with no sanitizer or leak reports.

Commands used, repeated with `build-final-release` and `Release`:

```bash
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1
cmake -S . -B build-final-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=-Werror
cmake --build build-final-debug -j2
./build-final-debug/test_p2
bash tests/p2/test_cli.sh ./build-final-debug/miniharness
```

## Required test categories

The main `tests/p2/test_p2.cpp` runner includes the shared component test headers
and runs the integration tests. The following covers all 12 required categories.

| Requirement | Evidence |
| --- | --- |
| Empty Conversation bounds | `empty_and_bounds` checks null/empty state and out-of-range exceptions. |
| System message ordering | `system_ordering`, growth, and harness tests retain System first and reject late System messages. |
| Deep copying | `copy_constructor` and `copy_assignment` verify independent array/string storage and source independence. |
| Pointer-stealing moves | `move_constructor` and `move_assignment` check identical transferred pointers, zeroed sources, reuse, and self-assignment. |
| Growth behavior | `growth_and_iteration` checks doubling through 4,097 messages and preserves order/content. |
| Clean scanner text | `clean_text` checks chunked ordinary output and flushing. |
| Every sentinel split | `every_split` loops over all split points; `character_chunks` uses one byte per feed. |
| False matches | `false_alarms_and_partial_end` checks false prefixes and every incomplete sentinel prefix. |
| Bounded scanner memory | `bounded_stress` checks pending size after every byte in a 4 MiB adversarial stream; a large single chunk is also tested. |
| Harness turn limits | `turn_limit` and `default_turn_limit` check zero, two, and default 20 turns. |
| Sentinel halt | `sentinel_halt` checks chunk sizes, terminal suppression, stored sentinel, discarded trailing text, and stopping on the final allowed turn. |
| Transcript round trip | `transcript_round_trip` saves/replays conversations with EOF and sentinel endings, comparing all messages, roles, output, and stop reasons. |

Additional coverage includes EOF, blank input, script/replay exhaustion,
custom sentinels, and three real CLI transcript checks. Assertions remain enabled
in Release test builds. Allocation-failure handling was reviewed structurally;
the suite does not inject allocation failures.

## Source and documentation audit

- All required source/header paths and fixed public interfaces are present.
- Message provides both constructors and the specified const/noexcept accessors.
- Conversation owns a raw growable array and implements the complete Rule of Five.
- Raw allocation/deallocation appears only in Conversation. Deleted test-helper
  copy operations (`= delete`) are declarations, not memory deallocation.
- Student code does not use `std::vector`. Its two occurrences belong to the
  unchanged provided model-client headers.
- All 11 supplied production/build/example files match the starter ZIP
  byte-for-byte: four headers, five source files, CMakeLists.txt, and the script.
- The design log is 752 whitespace-delimited words including headings, within
  the 500-800-word limit. It covers the growth proof, lifetime/exception safety,
  pending-buffer proof, and hindsight.
- The README documents usage, build/test commands, actual behavior, and known
  limits of the supplied CLI. Project 1 is preserved and excluded from P2 targets.

## Repository and submission

`github.txt` contains the direct repository URL and matches `origin`:
https://github.com/aarijawan123-blip/ece309_harness

GitHub's unauthenticated repository API confirmed that this repository exists,
is public, and uses `main` as its default branch.

Part 7 packaging uses `git archive` on the final commit to create `github.zip`.
The archive includes all tracked files under `ece309_harness/`, including the
retained Project 1 files. Build outputs, the ZIP itself, and `.git` internals
are excluded. The packaging check reads every ZIP entry, checks CRCs, and
compares its Git blob hash with the final commit's tree.

The remaining manual step is uploading `github.txt` and `github.zip` to Moodle.
Course submission itself is not performed by the build or test commands.
