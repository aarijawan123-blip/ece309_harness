# ECE 309 Harness Projects

Project 2 is a C++17 conversation harness with a custom growable message array
and streaming stop-sentinel detection. It uses deterministic scripted/replay
clients, with no network connection or model API required.

**Status:** Parts 1-6 are complete: setup, Message, Conversation, SentinelScanner,
integration tests, and documentation. Part 7 is the final audit and submission
packaging. Each part has its own commit.

Project 1 remains at the repository root. Its C source and Bash tests are not
part of the Project 2 CMake targets. Its original documentation is preserved in
[the Project 1 README](docs/project1-readme.md), including historical setup and
submission instructions that should not be used for Project 2.

## Build

Use Linux or Ubuntu WSL with GCC's C++ compiler, CMake 3.16 or newer, Make, and
Bash. On a new Ubuntu installation:

```bash
sudo apt-get update
sudo apt-get install g++ cmake make
```

Run all following commands from the repository root. In this Windows workspace,
the WSL path is `/mnt/c/Users/aarij/OneDrive/Documents/ece309_harness`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=-Werror
cmake --build build -j2
```

The provided CMake configuration enables C++17, `-Wall -Wextra -Wpedantic`,
AddressSanitizer, and UndefinedBehaviorSanitizer. The command above additionally
treats warnings as errors. It produces `build/miniharness` and `build/test_p2`.

## Run

```bash
./build/miniharness --script scripts/greeting.script --save transcript.txt
```

Enter any three nonempty lines to consume the example's three replies. The last
reply is `Goodbye!`, followed by a stop message reporting three turns. Replies
come from the script in order; they do not depend on the words you type.

| Option | Behavior |
| --- | --- |
| `--script PATH` | Read scripted model replies from a file. Always supply this: the default `default.script` is not included. |
| `--max-turns N` | Stop after N completed user/assistant turns; default 20. Use a nonnegative integer. |
| `--save PATH` | Save the conversation when the loop finishes. Use an existing, writable parent directory. |

Ctrl+D on an empty terminal line ends input gracefully and still saves the
transcript when requested. Empty lines are skipped. Project 1 commands such as
`exit`, `history`, and `calc` are ordinary input in Project 2.

The supplied CLI has basic argument handling: use the documented options with
valid values. Its input adapter treats EOF as shutdown, so finish redirected
input lines with newlines. Its transcript writer does not report a failed file
open; verify the output file exists. These provided implementations are unchanged.

## Scripts, streaming, and transcripts

A script contains message blocks separated by a line containing exactly `---`.
A leading System block supplies the initial system message. Assistant blocks
are consumed in order. Put any `chunk: N` directive before the block's role:

```text
role: system
Be concise.
---
chunk: 2
role: assistant
Goodbye.<|end_conversation|>
```

The stop sentinel may span chunks. Text before it is displayed; the sentinel
and subsequent text are hidden. The saved Assistant message includes the
sentinel so replay stops at the same point. Text after the sentinel is discarded
from both terminal output and stored history. A bare `---` line cannot appear
inside message content. Running out of scripted replies yields ClientError.

ReplayModelClient reads saved Assistant messages and the initial System message.
The supplied CLI selects ScriptedModelClient only; replay is exercised through
the C++ tests, not through a `--replay` option. The optional `match:` directives
are ignored by the supplied scripted client.

## Implementation and layout

| Files | Ownership / purpose |
| --- | --- |
| `include/core/message.h` | Our Message class: role, owned text, constructors, const accessors. |
| `include/core/conversation.h`, `src/conversation.cpp` | Our growable array, bounds checking, iteration, and Rule of Five. |
| `include/core/sentinel_scanner.h`, `src/sentinel_scanner.cpp` | Our bounded streaming scanner. |
| `include/model/`, `src/model_client.cpp`, `src/scripted_client.cpp`, `src/replay_client.cpp` | Provided model interfaces and implementations, unchanged. |
| `include/harness/harness.h`, `src/harness.cpp`, `src/main.cpp` | Provided execution loop and terminal CLI, unchanged. |
| `CMakeLists.txt`, `scripts/greeting.script` | Provided build configuration and example, unchanged. |
| `tests/p2/test_p2.cpp` | Main component/integration test runner. |
| `tests/p2/test_message.h`, `test_conversation.h`, `test_sentinel_scanner.h` | Shared component test functions in `tests/p2/`. |
| `tests/p2/test_message.cpp`, `test_conversation.cpp`, `test_sentinel_scanner.cpp` | Standalone component runners in `tests/p2/`. |
| `tests/p2/test_cli.sh` | Actual CLI output and transcript checks. |
| `docs/design-log-p2.md` | Design decisions, growth and buffer proofs, validation, and hindsight. |
| `docs/p2-starter-README.md` | Original starter instructions, preserved for reference. |
| `github.txt` | Existing repository URL. |

Conversation starts without allocating, then grows through capacities 1, 2, 4,
8, and so on. It keeps every message; the Project 1 five-turn limit does not
apply. A System message is allowed only first; later ones throw
`std::invalid_argument`. Invalid `at()` indices throw `std::out_of_range`.
Copies own independent storage; moves leave the source empty and reusable.
Growth invalidates pointers into the old array. Raw array allocation/deallocation
is confined to Conversation, which does not use `std::vector`.

SentinelScanner retains at most `sentinel.size() - 1` trailing bytes. Its first
match stops output permanently. Without a match, `flush()` releases the withheld
suffix, including partial sentinel text. Repeated flushing emits nothing more.
Empty sentinels throw `std::invalid_argument`. Temporary processing/output memory
depends on the current chunk size, while retained state depends only on sentinel
length. Private friend helpers let tests inspect capacity and pending length
without adding methods to the required public interfaces.

See the [design log](docs/design-log-p2.md) for the proofs and ownership reasoning.

## Tests and memory checks

After building:

```bash
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_p2
bash tests/p2/test_cli.sh
```

The C++ runner executes **26 assert-based groups**: one Message, eight
Conversation, nine SentinelScanner, and eight harness integration groups.
Assertions remain active in Release builds. The **three CLI checks** compare
complete output and saved transcripts on sentinel, EOF, and turn-limit shutdown.
The provided CMake file does not register CTest tests; run these commands directly.

Coverage includes empty bounds, pinned System messages, deep copies, pointer-
stealing moves, self-assignment, doubled capacities, every sentinel split point,
false matches, and 4 MiB fed one byte at a time with the pending bound checked
after every byte. Integration checks cover zero/two/default-20 turn limits,
blank input, both clients' exhaustion, and saved-conversation replay ending at
EOF or a sentinel. Round trips compare all roles, content, output, and stop
reasons. Temporary fixtures are cleaned up after successful runs.

To check Release separately:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror
cmake --build build-release -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build-release/test_p2
bash tests/p2/test_cli.sh ./build-release/miniharness
```

**Verified results from Part 5:** Ubuntu WSL, GCC 15.2.0, Debug and Release builds;
26/26 C++ groups and 3/3 CLI checks passed in each build. No compiler warnings,
AddressSanitizer errors, UndefinedBehaviorSanitizer errors, or leaks were reported.
Allocation-failure cleanup was reviewed but not tested by injecting allocation
failures. These results cover the exercised inputs, not every possible input.

The component runners can also be built independently, for example:

```bash
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -g \
  -fsanitize=address,undefined -Iinclude src/conversation.cpp \
  tests/p2/test_conversation.cpp -o build/test_conversation
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_conversation
```

## Submission preparation

The repository URL is already recorded in `github.txt`. Part 7 will perform the
final audit and generate `github.zip`. Any existing ZIP from Project 1 is not a
current Project 2 submission. Build directories, binaries, the example transcript,
and the ZIP are excluded from Git.

After the final changes have been committed and pushed, generate the backup
from the final commit:

```bash
git archive --format=zip --prefix=ece309_harness/ --output=github.zip HEAD
```

Submit `github.txt` and the regenerated `github.zip` through the course submission
system. The archive includes all tracked project files, including Project 1,
without generated files or `.git` internals. It contains committed changes only.
