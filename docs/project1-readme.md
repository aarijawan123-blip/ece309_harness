# ECE 309 Project 1: Mini LLM Harness in C

A small terminal program that demonstrates what an LLM agent harness does:
read input, manage limited conversation context, call a model or a tool, and
display a response. The model is deliberately mocked, so no API key, network
connection, external library, or actual LLM is needed.

The implementation follows `Proj1_spec.pdf`. The assignment allows AI-assisted
development; the original prompt, architecture, actual iterations, and generated
AI responses are recorded in `vibe_coding_log.md`.

## Features

- A terminal loop using `fgets`, with `exit` and end-of-file shutdown.
- A deterministic greeting/echo mock model.
- Five complete user/response pairs of context, stored safely in fixed arrays.
- `history` to inspect context and `recall` to demonstrate the model using it.
- A calculator tool supporting addition, subtraction, multiplication, and division.
- Checks for invalid arithmetic, oversized input, and input stream errors.
- A separate AI-generated Bash test suite with a sanitizer mode.
- A single commented C source file using only standard C library functionality.

## File structure

| File | Purpose |
| --- | --- |
| `harness.c` | Complete program: terminal loop, history, mock model, calculator. |
| `test.sh` | 38 automated behavioral tests; optional memory checking. |
| `README.md` | Build, usage, architecture, validation, and submission instructions. |
| `vibe_coding_log.md` | Specification, exact user prompt, AI responses, and real development record. |
| `github.txt` | Placeholder to replace with the actual published repository URL. |
| `Proj1_spec.pdf` | Original five-page assignment specification, preserved unchanged. |
| `.gitignore` | Excludes generated binaries, temporary files, and the submission ZIP. |
| `.gitattributes` | Keeps source, scripts, and documentation in LF format across platforms. |
| `github.zip` | Generated backup of the project files for submission; regenerate after publishing. |

Builds produce `harness` and, in memory mode, `harness_asan`. These executables
are intentionally excluded from Git and the ZIP. There are no project-specific
headers because all program functions fit in one source file. The archive
contains the source repository files, including the assignment PDF and hidden
configuration files, without `.git` internals or generated artifacts.

## Environment and compilation

Use Linux/Ubuntu WSL with GCC and Bash. The C program itself uses standard C11;
the test script uses Bash. No package is needed by the program beyond the normal
C compiler/runtime. On Windows, run the commands inside **Ubuntu WSL**, not
PowerShell. This workspace is available there at:

```bash
cd /mnt/c/Users/aarij/OneDrive/Documents/ece309_harness
```

The assignment's suggested editor is VS Code with Microsoft's C/C++ extension.
Open this folder in VS Code and use a WSL terminal for compilation. If setting
up a new Ubuntu environment, install GCC with `sudo apt install gcc` after
updating the package index. The existing environment already had GCC installed.

Compile with warnings:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic harness.c -o harness
```

The exact simpler command from the PDF also works:

```bash
gcc harness.c -o harness
```

No `-lm`, other library flags, or separate build system is required. GCC's
standard `isfinite` macro is used to validate calculator values.

## Running and commands

```bash
./harness
```

| Input | Behavior | Adds a turn? |
| --- | --- | --- |
| `hello` or `please say hello!` | Fixed greeting from the mock model. | Yes |
| Any other ordinary text | Mock model echoes it as `You said: ...`. | Yes |
| `calc 12 * 3` | Execute the calculator and return `Calculator result: 36`. | Yes |
| `history` | Print retained user/response pairs, oldest first. | No |
| `recall` | Mock model reports the previous user input, or empty context. | Yes |
| `help` | Print command instructions. | No |
| `exit` | Print `Bye!` and return success. | No |
| End-of-file (Ctrl+D on an empty Linux terminal line) | End successfully. | No |
| Empty or whitespace-only line | Ignore the line. | No |

Commands are lowercase and case-sensitive. `exit`, `history`, `recall`, and
`help` must match the whole line, without surrounding spaces. For example,
`exit now` is ordinary text. The greeting recognizes lowercase `hello` with
nonalphabetic boundaries: `hello!` matches, but `shelloworld` and `HELLO` do not.
This is a simple deterministic rule, not natural-language understanding.

Input is terminal text, limited to **255 bytes** per line, excluding its line
ending. LF and CRLF are supported, as is a final line without a newline.
Longer lines are rejected and drained completely; they cannot be split into
multiple commands or partially stored. The program expects text, not binary
input containing embedded null bytes. An input stream error prints a diagnostic
and returns failure.

Example session:

```text
ECE 309 Mini Harness
Type help for commands. Type exit to quit.
You> hello
Assistant: Hello! I am a mock model.
You> calc 12 * 3
Assistant: Calculator result: 36
You> recall
Assistant: Previous input: calc 12 * 3
You> history
History (3/5 turns):
1. User: hello
   Assistant: Hello! I am a mock model.
2. User: calc 12 * 3
   Assistant: Calculator result: 36
3. User: recall
   Assistant: Previous input: calc 12 * 3
You> exit
Bye!
```

## Architecture and context management

```text
initialize empty history -> print banner -> prompt and fgets
                                              |
                      +-----------------------+--------------------+
                      |                       |                    |
                exit / EOF             local commands      model or calc tool
                      |               help / history               |
                 return success           |                 print response
                                          |                 save complete pair
                                          +---- next prompt -------+
```

`main` owns the input buffer, response buffer, and `Turn history[5]`. Each
`Turn` has a 256-byte user buffer and a 512-byte response buffer. `count`
tracks how many entries are valid, from zero through five. This is a fixed
3,840-byte allocation for the history on the tested platform. It does not grow
with the conversation.

`save_turn` appends a pair. If all five slots are occupied, it shifts entries
2-5 into slots 1-4, clears the reused last slot, and saves the new pair there.
The oldest user input **and its corresponding response** are dropped together.
Numbers displayed by `history` are positions in the retained window, not
lifetime turn IDs. History lasts for one process and starts empty on every run.

The mock model receives the existing history **before** its current response
is stored. Thus `recall` can read the previous input. Calculator results and
calculator error responses are also saved as complete turns; local control
commands and invalid/blank input do not consume history slots.

Demonstrate eviction:

```bash
printf 'one\ntwo\nthree\nfour\nfive\nsix\nhistory\nexit\n' | ./harness
```

The final history contains exactly `two`, `three`, `four`, `five`, and `six`,
with their five matching responses. `one` is gone.

All application storage has automatic lifetime, so no `malloc`/`free` is needed.
Bounded `fgets`/`snprintf` calls protect the buffers; the response capacity is
large enough for every input plus its fixed response prefix. Array shifts copy
entire structures. These choices keep ownership and cleanup easy to explain.

## Calculator tool

The harness recognizes `calc` followed by whitespace or end-of-line and calls
`calculator` instead of asking the mock model to invent a numerical answer.
This is a local C function call, demonstrating delegated tool execution.

Use one binary expression at a time:

```text
calc 2 + 3
calc 2 - 5
calc -2 * 3.5
calc 7 / 2
```

The outputs are `5`, `-3`, `-7`, and `3.5`. Spaces around the operator are
recommended for readability; `calc 2+3` also works. Signed numbers, decimals,
and scientific notation are accepted through the standard `strtod` function.
When typing into the running program, `*` needs no shell escaping.

The parser reads the first number, a supported operator, and the second number,
then verifies that only whitespace remains. It checks conversion errors and
rejects nonfinite operands, unsupported operators, extra tokens, division by
positive or negative zero, and nonfinite arithmetic results. Error text is
returned normally, so the session can continue.

Calculations use `double` and print up to 10 significant digits. Floating-point
rounding applies; this is not an exact symbolic or arbitrary-precision calculator.
Very small arithmetic results can round to zero. Parentheses, chained
expressions, and operator precedence are deliberately outside this tool's scope.

## Automated tests

```bash
bash test.sh
bash test.sh --memory
```

Both modes compile the source themselves and fail with a nonzero status on
compiler errors, warnings, output mismatches, or runtime failures. Run with
`bash`, so executable file permissions are not required. Each test pipes
predefined input into a fresh program and compares the complete transcript
after removing interactive prompt markers. It also checks the process status.

The 38 cases cover startup, greeting rules, normal echo, format-string-like
text, all calculator operators, calculator errors, exact command routing,
exit/EOF, CRLF, blank input, help, empty history, context recall, stored
user/response pairs, read-only history, exactly five turns, sixth-turn eviction,
1,000-turn eviction stress, 255-byte input, 256-byte rejection, and draining
10,000-byte lines without corrupting subsequent input or history.

`--memory` uses the **same entire suite** with a sanitizer build. Sanitizer
support is required for that mode: failures are not silently treated as passes.

## Memory testing and verified results

Validation was performed in Ubuntu WSL on this Windows workspace with
**GCC 15.2.0**. The normal test build treats warnings as errors:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror harness.c -o harness
```

The memory mode uses:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
  -fsanitize=address,undefined -fno-omit-frame-pointer harness.c -o harness_asan
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

Observed results after correcting the warning documented in the log:

- Ordinary tests: **38/38 passed**.
- AddressSanitizer and UndefinedBehaviorSanitizer tests: **38/38 passed**.
- Leak detection was enabled; no leaks, invalid memory accesses, or undefined
  behavior were reported in the exercised cases.
- GCC builds completed with no warnings or errors after the correction.
- Valgrind was unavailable and was not used.

Fixed arrays avoid application-owned heap leaks, while AddressSanitizer also
checks the exercised stack/buffer accesses. Passing tests are evidence for these
inputs, not a proof for every possible input. The log records the actual
optimized-build warning and correction, without inventing debugging failures.

## GitHub publishing and assignment submission

The assignment requires **both `github.txt` and `github.zip`**, due
**September 5, 2026** according to the PDF. Publishing and course upload remain
manual: no repository URL has been invented. The supplied ZIP is a backup of
the completed project but currently includes the placeholder `github.txt`.
Regenerate it after setting the real URL.

1. Review the source and log so you can explain the design. If following the
   editor setup in the guide, ensure VS Code and Microsoft's C/C++ extension
   are installed; editor installation was not verified during this session.
2. On GitHub, create an **empty** repository named `ece309_harness`. Do not add
   a GitHub-generated README, license, or `.gitignore` to that empty repository.
   Copy its actual HTTPS repository URL from your browser.
3. Run the following in Ubuntu WSL from this workspace. The local repository
   has already been initialized on `main`; no commits or remote are required
   to run the program. Git may request your GitHub credentials when pushing.

```bash
cd /mnt/c/Users/aarij/OneDrive/Documents/ece309_harness
bash test.sh
bash test.sh --memory
gcc -std=c11 -Wall -Wextra -Wpedantic harness.c -o harness

read -r -p 'Paste your actual GitHub repository HTTPS URL: ' REPO_URL
printf '%s\n' "$REPO_URL" > github.txt
git add .gitattributes .gitignore harness.c test.sh README.md vibe_coding_log.md github.txt Proj1_spec.pdf
git commit -m "Complete ECE 309 Project 1 mini harness"
git remote add origin "$REPO_URL"
git push -u origin main
git archive --format=zip --prefix=ece309_harness/ --output=github.zip HEAD
```

If Git reports an unknown author identity, set your own name/email and retry
the commit (do not use somebody else's identity):

```bash
read -r -p 'Your Git author name: ' AUTHOR_NAME
read -r -p 'Your Git author email: ' AUTHOR_EMAIL
git config user.name "$AUTHOR_NAME"
git config user.email "$AUTHOR_EMAIL"
```

If using a newly extracted ZIP rather than this workspace, run `git init -b main`
before the publishing commands. If you have already added `origin`, update it
with `git remote set-url origin "$REPO_URL"` instead of adding it again.
Only generate the final archive after a successful commit so it contains the
latest URL, source, tests, README, and log.

4. Open your GitHub URL and confirm the project files are visible and accessible
   to the grader. Submit **`github.txt` and `github.zip`** through the course's
   submission system. The PDF does not specify an upload command or portal URL.
