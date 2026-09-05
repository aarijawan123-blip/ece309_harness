# ECE 309 Project 1: AI-assisted development log

## Specification and architecture recorded before implementation

The AI assistant read the text of all five pages of `Proj1_spec.pdf`, then
rendered and inspected all five pages before writing the implementation.
The assignment explicitly permits AI-generated code and requires Specification
Driven Development (SDD), testing, a prompt/response log, and GitHub submission.
This log records the actual user prompt and work in this session. There were
no separate user follow-up prompts during the initial design.

Architectural specification used to generate the program:

1. Use one beginner-friendly source file, `harness.c`, with small, directly
   called functions and explanatory comments. Use only standard C headers.
   Compile on Ubuntu/Linux with GCC. No API calls, networking, external
   libraries, shell execution, or dynamically generated commands.
2. State machine: initialize an empty history; print a banner; repeatedly print
   and flush a prompt, read with `fgets`, validate the line, dispatch the input,
   print the response, save a completed turn, and return to input. Exact `exit`
   or end-of-file ends the program successfully. An input stream error fails
   with a diagnostic and a nonzero exit status.
3. Accept text lines of at most 255 bytes, excluding LF or CRLF. Reject and drain
   overlong lines so leftover input cannot become an accidental command. Ignore
   blank/whitespace-only input. Commands are lowercase and case-sensitive.
4. A turn is one user input plus its model/tool response. Allocate a fixed array
   of five `Turn` structures, each with bounded character arrays. When full,
   shift the four newest pairs left before storing the new pair. Use bounded
   string writes. Automatic storage avoids heap ownership and leaks entirely.
5. `history` displays retained pairs from oldest to newest and does not add a
   turn. `help`, `exit`, blank lines, and rejected long lines also add no turn.
   Model and calculator responses, including calculator errors, do add turns.
6. The mock model receives the retained history. `recall` returns the most recent
   earlier user input or reports empty context; a sentence containing the
   lowercase word `hello` receives a fixed greeting; other input is echoed.
   Alphabetic boundaries distinguish `hello` from `shelloworld`.
7. The harness dispatches `calc NUMBER OP NUMBER` to a separate calculator
   function. Support `+`, `-`, `*`, and `/` with signed/decimal numbers.
   Use checked `strtod` conversions; reject invalid syntax, trailing junk,
   out-of-range/nonfinite numbers, division by zero, and nonfinite results.
   Return text through the same bounded response buffer as the model.
8. Generate a standalone Bash `test.sh` that builds with GCC warnings and
   compares deterministic outputs, including ordered history and eviction.
   Its `--memory` mode rebuilds using GCC AddressSanitizer and
   UndefinedBehaviorSanitizer, enables leak checking, and repeats the whole
   suite. A missing sanitizer or failed test must fail visibly, not be skipped.
9. Document commands, limits, architecture, build/test instructions and actual
   results in `README.md` and this log. Include `.gitignore`, `.gitattributes`,
   a truthful `github.txt` placeholder, and the required `github.zip` backup.
   Keep binaries and temporary inspection files out of version control.
10. Finish by rerunning the entire normal and memory suites, compiling again,
    checking all PDF requirements, and verifying the submission archive.

## Initial AI development prompt (verbatim)

```text
Read the attached ECE 309 Project 1 specification completely before making any changes.

I want you to complete this entire project for me inside the current `ece309_harness` workspace. Follow the assignment specification exactly and keep the implementation simple, beginner-friendly, and easy for me to explain.

Requirements:

1. Build a minimal LLM agent harness in standard C that compiles and runs in a POSIX/Linux environment using GCC.
2. Implement a terminal-based core loop:
   - Read user input with `fgets`.
   - Continue until the user enters `exit`.
   - Pass normal input to a mock model function.
   - Print the mock model response.
3. Implement a simple mock model function. Do not use any real LLM APIs or external libraries.
4. Implement context management:
   - Maintain only the most recent 5 conversation turns.
   - Manage memory safely.
   - Make the behavior easy to demonstrate and test.
5. Implement at least one tool that represents something an LLM should delegate to a tool.
   - Use a simple mathematical calculator.
   - Keep the command format straightforward and document how to use it.
6. Keep the C code beginner-friendly.
   - Avoid unnecessary abstraction.
   - Use only standard C/POSIX functionality where appropriate.
   - Add useful comments explaining major sections.
   - Do not over-engineer the project.
7. Compile the program using GCC.
   - Use warnings such as `-Wall -Wextra` when checking it.
   - Fix all compiler errors.
   - Fix reasonable compiler warnings.
8. Create an AI-generated Bash testing script named `test.sh`.\
   It should automatically test:
   - startup/basic interaction
   - the hello/greeting behavior if implemented
   - normal mock-model input
   - calculator/tool execution
   - clean exit
   - conversation state/history behavior
   - the 5-turn history limit
9. Perform a basic memory-safety check.
   - Prefer AddressSanitizer with GCC if available.
   - Valgrind may also be used if installed.
   - Fix memory errors discovered during testing.
10. Create a detailed `README.md` explaining:

- what the project is
- project features
- file structure
- how to compile
- how to run
- available commands
- how context/history works
- how the calculator tool works
- how to run the tests
- how memory testing was performed

11. Create `vibe_coding_log.md` as required by the assignment.\
    This must demonstrate Specification Driven Development and AI-assisted development.

Include:

- the architectural specification used to design the program
- the initial AI development prompt
- testing prompt(s)
- important iterations/fixes made during development
- compiler/test problems encountered, if any
- how those problems were corrected
- a short explanation of the final architecture

Since this project is being generated from this prompt, include this prompt or a faithful record of it in the log.

Do not fabricate compiler failures or debugging events. Only document iterations/errors that actually occur while you work.

12. At the end, run the complete test suite and compile the final program again.
13. Inspect the entire repository against the Project 1 PDF and make sure every implementation and documentation requirement is satisfied.
14. Prepare the repository so it is ready to upload to GitHub.
15. Do not make up a GitHub URL. If no GitHub remote exists yet, create a placeholder `github.txt` with clear instructions that I should replace the placeholder with the actual repository URL after publishing.

When you finish, give me a concise report containing:

- every file created
- the final compile command
- test results
- memory-check results
- any assignment requirement that still requires manual action from me
- the exact commands I should run to publish/submit the project

Do not stop after generating the code. Compile it, test it, inspect the results, fix problems, and complete the repository.
```

## Testing prompts

The testing instructions came from items 7, 8, 9, 12, and 13 of the exact user
prompt above. They asked the AI to generate a separate Bash script, test startup,
greeting, echo, calculator, exit, history and the five-turn limit, use GCC
warnings and preferably AddressSanitizer, fix real problems, and rerun all checks
at the end. These were parts of the initial prompt, not invented later prompts.

## Actual AI responses and development record

The AI's initial response was:

> I’ll locate and read the full Project 1 specification first, then build, test, and check the repository against it.

After reading the specification, the AI reported:

> I’ve read all five pages. The PDF also requires a `github.zip` backup and a record of AI responses in the development log, so I’ll include both.
>
> I’ll use one commented C file, a fixed array holding five user/response pairs, `history` and `recall` commands to demonstrate context, and a `calc` command for arithmetic. Ubuntu WSL has GCC available for validation.

### Environment issues actually encountered before coding

- The Windows `python.exe` app alias failed with a logon-session error. The AI
  located and used the existing bundled Python executable instead.
- WSL initially returned `E_ACCESSDENIED` inside the execution sandbox. The AI
  requested permission to access WSL; the user approved. Ubuntu provided GCC
  15.2.0. WSL commands use the approved elevated execution path.
- WSL had no `pip`, `pdftotext`, `pdftoppm`, or Valgrind available during the
  initial tool check. Existing Windows Python/Poppler tools read and rendered
  the PDF; no project dependency was added for this inspection.
- The first PDF text extraction hit a Windows `cp1252` `UnicodeEncodeError` on
  a ligature. Re-running Python with `-X utf8` printed all five pages correctly.
- `git rev-parse --show-toplevel` reported that the initial workspace was not a
  Git repository. It initially contained only `Proj1_spec.pdf`; there was no
  existing GitHub remote or source code to preserve.

Build/test observations and complete generated source/script responses are
recorded below after execution, so this log does not predict test outcomes.

### Iteration 1: first implementation and warning correction

The AI generated `harness.c` and `test.sh` from the architectural specification.
The initial ordinary build used `-std=c11 -Wall -Wextra -Wpedantic -Werror`.
It compiled without diagnostics, and all 38 behavioral tests passed.

The first memory-mode build enabled `-O1` as well as sanitizers. GCC inlined
`save_turn` and reported this real compiler diagnostic:

```text
harness.c:58:49: error: '%s' directive output may be truncated writing up to 257 bytes into a region of size 256 [-Werror=format-truncation=]
cc1: all warnings being treated as errors
```

The input buffer reserves extra bytes for line endings, whereas a history entry
holds 255 text bytes and a null. Although the input loop validates the length,
the optimizer did not prove that constraint at the `snprintf` call. The AI made
the string-copy limit explicit without disabling any warning:

```c
/* Before */
snprintf(history[*count].user, INPUT_SIZE, "%s", input);
/* After */
snprintf(history[*count].user, INPUT_SIZE, "%.*s", INPUT_SIZE - 1, input);
```

The memory suite had not executed at this point because its compile failed.

### Iteration 2: validation after the warning fix

The AI reran `bash test.sh` followed by `bash test.sh --memory` in Ubuntu WSL.
Both builds completed with warnings treated as errors, and each suite passed
all 38 cases. AddressSanitizer leak detection and UndefinedBehaviorSanitizer
were enabled. Neither reported a runtime error or leak. No behavioral test
failed during this development session, and no memory defect was discovered.

The AI then reported:

> All 38 tests now pass in both the normal and sanitizer builds, with no memory or undefined-behavior errors reported.
>
> The optimized build caught one string-copy warning. I made the copy limit explicit and reran both suites; the log records that actual issue and fix. I’m finishing the README, GitHub instructions, and submission archive now.

The AI added a detailed README with the command contract, bounded context
design, calculator behavior/limits, test commands/results, and publishing steps.
The PDF's additional ZIP requirement was included in the deliverables. No
external API, library, or extra source/header module was introduced.

## Final architecture in plain language

`main` owns all state and runs the prompt/read/dispatch/respond/store loop.
`mock_model` returns deterministic greeting, echo, or recall text.
`calculator` parses and computes a single expression. `save_turn` maintains
five complete pairs, and `print_history` displays them. Small whitespace and
greeting helpers keep parsing readable. Everything lives in one C file, uses
fixed automatic storage, and ends safely without heap cleanup.

The Bash script runs each scenario in a new process and compares deterministic
output. Its memory mode rebuilds the same source with sanitizers and runs the
same tests. The fixed specification and expected outputs were written before
the first build; the only C revision corrected an observed compiler warning.
This is the SDD cycle used here: requirements, architecture, generated code,
verification, evidence-based correction, and verification again.

## Final verification and assignment audit

After completing the README, the AI ran this final sequence in Ubuntu WSL:

```bash
gcc harness.c -o harness
bash -n test.sh
bash test.sh
bash test.sh --memory
gcc -std=c11 -Wall -Wextra -Wpedantic harness.c -o harness
```

The sequence exited with status 0. The exact test summary lines were:

```text
All 38 tests passed (./harness).
All 38 tests passed (./harness_asan).
```

Both standalone compile commands and both test builds completed without
diagnostics. No sanitizer report appeared. The final executable is the ordinary
warning-enabled build; `harness_asan` is also available locally and ignored by Git.

The AI initialized the local repository using `git init -b main`. Final
`git status --short --branch` showed `No commits yet on main` and the eight
intended source/document/configuration files. `git remote -v` returned no
remote. `git check-ignore` confirmed that both executables, `github.zip`, and
temporary PDF inspection output are excluded from version control.

The AI inspected all source, tests, documentation, and configuration against
the previously read five-page specification:

| Specification item | Evidence / status |
| --- | --- |
| Page 1: standard C and POSIX compilation | One standard C file; both PDF command and warning-enabled GCC builds pass in Ubuntu WSL. |
| Page 1: core terminal loop | `main` uses `fgets`, dispatches normal input to `mock_model`, prints responses, and handles `exit`/EOF. |
| Page 1: safe minimal history | Fixed five-entry `Turn` array, bounded copies, paired eviction; 5-, 6-, and 1,000-turn tests pass. |
| Page 1: tool execution | Separate calculator function with four arithmetic operations and checked errors; tool tests pass. |
| Page 1: vibe coding log and architectural rules | This file records the architecture before implementation and the exact initial user prompt. |
| Page 2: AI-generated testing for state and leaks | Separate `test.sh` includes deterministic state tests and `--memory` with leak checking enabled. |
| Page 3: isolated workspace and compiler environment | Existing `ece309_harness` workspace; Ubuntu WSL and GCC 15.2.0 used. VS Code/extension setup remains a user environment check. |
| Pages 3-4: SDD, simple commented source, hello/echo, build/run | Architecture precedes source; `harness.c` contains explanatory comments; greeting/echo/exit and GCC compilation verified. |
| Page 4: Bash automation | `bash test.sh` runs predefined piped inputs; all 38 cases pass. |
| Page 5: source, tests, descriptive README in GitHub repository | All files prepared and local Git initialized. User must publish the GitHub repository. No separate project header is necessary. |
| Page 5: prompts, iterations, and AI responses | Exact prompt, actual progress quotations, actual warning/fix, and complete generated C/Bash responses are in this log. |
| Page 5: `github.txt` | Clearly labeled placeholder; user must replace it with the actual direct repository URL after publishing. |
| Page 5: `github.zip` | Backup generated from all eight deliverable files, including the PDF and hidden configuration files. Excludes binaries, temporary files, and `.git` internals. Regenerate after setting the URL. |
| Page 5: submit both files by September 5, 2026 | Manual course upload remains; the PDF provides no upload command or portal URL. |

Packaging uses Python's standard `zipfile` module as an environment utility,
not a program or test dependency. The archive is reopened, checked for CRC
errors, and each of its eight entries is compared byte-for-byte with the
corresponding workspace file. The publishing instructions use `git archive`
so the user does not need Python to regenerate the final submission.


## Complete generated AI code responses

The following are exact snapshots of the final AI-generated source and
testing script written during this session. The initial C response differs
only by the one string-copy correction recorded above. These snapshots
preserve the substantive AI code responses in the log as required by the PDF.

### harness.c

```c
/* ECE 309 Project 1: a small, deterministic LLM agent harness. */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_LIMIT 5
#define INPUT_SIZE 256       /* 255 bytes of text plus the terminating null. */
#define RESPONSE_SIZE 512

/* One conversation turn includes both sides of the interaction. */
typedef struct {
    char user[INPUT_SIZE];
    char response[RESPONSE_SIZE];
} Turn;

/* Skip whitespace safely, including when char is signed on this system. */
static const char *skip_spaces(const char *text)
{
    while (isspace((unsigned char)*text)) {
        text++;
    }
    return text;
}

static void print_help(void)
{
    puts("Commands: hello, calc NUMBER OP NUMBER, history, recall, help, exit");
    puts("Calculator operators: + - * / (example: calc 12 * 3)");
    puts("Other text is echoed. History keeps the last 5 user/response pairs.");
}

/* Displaying history does not change it. Entries are oldest first. */
static void print_history(const Turn history[], int count)
{
    printf("History (%d/%d turns):\n", count, HISTORY_LIMIT);
    for (int i = 0; i < count; i++) {
        printf("%d. User: %s\n", i + 1, history[i].user);
        printf("   Assistant: %s\n", history[i].response);
    }
}

static void save_turn(Turn history[], int *count,
                      const char *input, const char *response)
{
    /* When full, discard the oldest complete pair by shifting left. */
    if (*count == HISTORY_LIMIT) {
        for (int i = 1; i < HISTORY_LIMIT; i++) {
            history[i - 1] = history[i];
        }
        (*count)--;
    }

    /* Clear the reused slot and copy both strings with explicit bounds. */
    history[*count] = (Turn){0};
    snprintf(history[*count].user, INPUT_SIZE, "%.*s", INPUT_SIZE - 1, input);
    snprintf(history[*count].response, RESPONSE_SIZE, "%s", response);
    (*count)++;
}

/* Match lowercase hello as a word, including in "please say hello!". */
static int contains_hello(const char *input)
{
    const char *word = input;
    while ((word = strstr(word, "hello")) != NULL) {
        if ((word == input || !isalpha((unsigned char)word[-1])) &&
            !isalpha((unsigned char)word[5])) {
            return 1;
        }
        word++;
    }
    return 0;
}

/* The mock model reads prior context but never changes the history itself. */
static void mock_model(const char *input, const Turn history[], int count,
                       char response[])
{
    if (strcmp(input, "recall") == 0) {
        if (count == 0) {
            snprintf(response, RESPONSE_SIZE, "No previous conversation turn.");
        } else {
            snprintf(response, RESPONSE_SIZE, "Previous input: %s",
                     history[count - 1].user);
        }
    } else if (contains_hello(input)) {
        snprintf(response, RESPONSE_SIZE, "Hello! I am a mock model.");
    } else {
        snprintf(response, RESPONSE_SIZE, "You said: %s", input);
    }
}

/* Arithmetic belongs in a tool: the mock model does not guess the answer. */
static void calculator(const char *expression, char response[])
{
    const char *cursor = skip_spaces(expression);
    char *end;
    double left, right, result;
    char operation;

    /* strtod lets us detect failed conversions and out-of-range numbers. */
    errno = 0;
    left = strtod(cursor, &end);
    if (cursor == end || errno == ERANGE || !isfinite(left)) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: invalid first number.");
        return;
    }

    cursor = skip_spaces(end);
    operation = *cursor;
    if (operation != '+' && operation != '-' &&
        operation != '*' && operation != '/') {
        snprintf(response, RESPONSE_SIZE, "Calculator error: use +, -, *, or /.");
        return;
    }

    cursor = skip_spaces(cursor + 1);
    errno = 0;
    right = strtod(cursor, &end);
    if (cursor == end || errno == ERANGE || !isfinite(right)) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: invalid second number.");
        return;
    }
    if (*skip_spaces(end) != '\0') {
        snprintf(response, RESPONSE_SIZE, "Calculator error: unexpected trailing text.");
        return;
    }
    if (operation == '/' && right == 0.0) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: division by zero.");
        return;
    }

    switch (operation) {
        case '+': result = left + right; break;
        case '-': result = left - right; break;
        case '*': result = left * right; break;
        default:  result = left / right; break; /* Already validated as '/'. */
    }

    if (!isfinite(result)) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: result out of range.");
    } else {
        snprintf(response, RESPONSE_SIZE, "Calculator result: %.10g", result);
    }
}

int main(void)
{
    /* Fixed automatic storage: no malloc/free or heap ownership is needed. */
    Turn history[HISTORY_LIMIT] = {0};
    int count = 0;
    /* Extra room accepts 255 text bytes plus either LF or CRLF and a null. */
    char input[INPUT_SIZE + 2];
    char response[RESPONSE_SIZE];

    puts("ECE 309 Mini Harness");
    puts("Type help for commands. Type exit to quit.");

    while (1) {
        size_t length;
        printf("You> ");
        fflush(stdout); /* Show the prompt before waiting for terminal input. */

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (ferror(stdin)) {
                perror("Input error");
                return EXIT_FAILURE;
            }
            break; /* End-of-file (Ctrl+D on Linux) is also a clean exit. */
        }

        length = strlen(input);
        if (length > 0 && input[length - 1] == '\n') {
            input[--length] = '\0';
        } else if (length == sizeof(input) - 1) {
            /* Drain an overlong line so its remainder is never a command. */
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* Discard the remainder of this one line. */
            }
            if (ferror(stdin)) {
                perror("Input error");
                return EXIT_FAILURE;
            }
        }
        if (length > 0 && input[length - 1] == '\r') {
            input[--length] = '\0';
        }
        if (length >= INPUT_SIZE) {
            puts("Input too long (maximum 255 bytes). Turn ignored.");
            continue;
        }
        if (*skip_spaces(input) == '\0') {
            continue;
        }
        if (strcmp(input, "exit") == 0) {
            break;
        }
        if (strcmp(input, "help") == 0) {
            print_help();
            continue;
        }
        if (strcmp(input, "history") == 0) {
            print_history(history, count);
            continue;
        }

        /* Only a complete calc command prefix selects the calculator tool. */
        if (strncmp(input, "calc", 4) == 0 &&
            (input[4] == '\0' || isspace((unsigned char)input[4]))) {
            calculator(input + 4, response);
        } else {
            mock_model(input, history, count, response);
        }

        printf("Assistant: %s\n", response);
        save_turn(history, &count, input, response);
    }

    puts("Bye!");
    return EXIT_SUCCESS; /* Automatic arrays are released on return. */
}
```

### test.sh

```bash
#!/usr/bin/env bash
# AI-generated black-box tests. Run with Bash from Linux/WSL or a POSIX host.
set -euo pipefail
cd -- "$(dirname -- "$0")"

flags=(-std=c11 -Wall -Wextra -Wpedantic -Werror)
program=./harness
case "${1:-}" in
    "") ;;
    --memory)
        program=./harness_asan
        flags+=(-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
        export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
        export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
        ;;
    *) echo "Usage: bash test.sh [--memory]" >&2; exit 2 ;;
esac
if (( $# > 1 )); then
    echo "Usage: bash test.sh [--memory]" >&2
    exit 2
fi

# A compiler or sanitizer failure stops the script; nothing is silently skipped.
gcc "${flags[@]}" harness.c -o "$program"
echo "Build passed: $program"
passed=0
banner=$'ECE 309 Mini Harness\nType help for commands. Type exit to quit.'

# Compare the entire transcript, ignoring only interactive prompt markers.
run_case() {
    local name=$1 input=$2 expected=$3 actual
    if ! actual=$(printf '%s' "$input" | "$program" 2>&1); then
        printf 'FAIL: %s (program or sanitizer failed)\n%s\n' "$name" "$actual" >&2
        exit 1
    fi
    actual=${actual//You> /}
    expected="$banner"$'\n'"$expected"
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAIL: %s\nEXPECTED:\n%s\nACTUAL:\n%s\n' \
            "$name" "$expected" "$actual" >&2
        exit 1
    fi
    printf 'PASS: %s\n' "$name"
    passed=$((passed + 1))
}

run_case 'startup and immediate exit' $'exit\n' 'Bye!'
run_case 'greeting' $'hello\nexit\n' $'Assistant: Hello! I am a mock model.\nBye!'
run_case 'greeting inside a sentence' $'please say hello!\nexit\n' \
    $'Assistant: Hello! I am a mock model.\nBye!'
run_case 'greeting word boundaries and case' $'shelloworld\nHELLO\nexit\n' \
    $'Assistant: You said: shelloworld\nAssistant: You said: HELLO\nBye!'
run_case 'normal mock input' $'testing the mock\nexit\n' \
    $'Assistant: You said: testing the mock\nBye!'
run_case 'input is text, not a printf format' $'%s %n\nexit\n' \
    $'Assistant: You said: %s %n\nBye!'
run_case 'all four calculator operators' $'calc 2 + 3\ncalc 2 - 5\ncalc -2 * 3.5\ncalc 7 / 2\nexit\n' \
    $'Assistant: Calculator result: 5\nAssistant: Calculator result: -3\nAssistant: Calculator result: -7\nAssistant: Calculator result: 3.5\nBye!'
run_case 'compact expression and whitespace' $'calc 2+3\ncalc\t 1.5 + -2.5  \nexit\n' \
    $'Assistant: Calculator result: 5\nAssistant: Calculator result: -1\nBye!'
run_case 'division by positive and negative zero' $'calc 1 / 0\ncalc 1 / -0\nexit\n' \
    $'Assistant: Calculator error: division by zero.\nAssistant: Calculator error: division by zero.\nBye!'
run_case 'missing calculator expression' $'calc\nexit\n' \
    $'Assistant: Calculator error: invalid first number.\nBye!'
run_case 'invalid first operand' $'calc apple + 2\nexit\n' \
    $'Assistant: Calculator error: invalid first number.\nBye!'
run_case 'invalid or missing second operand' $'calc 2 + apple\ncalc 2 +\nexit\n' \
    $'Assistant: Calculator error: invalid second number.\nAssistant: Calculator error: invalid second number.\nBye!'
run_case 'unsupported or missing operator' $'calc 2 ^ 3\ncalc 2\nexit\n' \
    $'Assistant: Calculator error: use +, -, *, or /.\nAssistant: Calculator error: use +, -, *, or /.\nBye!'
run_case 'trailing calculator text' $'calc 1 + 2 junk\nexit\n' \
    $'Assistant: Calculator error: unexpected trailing text.\nBye!'
run_case 'nonfinite operands' $'calc nan + 2\ncalc 1 + inf\nexit\n' \
    $'Assistant: Calculator error: invalid first number.\nAssistant: Calculator error: invalid second number.\nBye!'
run_case 'operand conversion range' $'calc 1e999 + 2\ncalc 1 + 1e-999\nexit\n' \
    $'Assistant: Calculator error: invalid first number.\nAssistant: Calculator error: invalid second number.\nBye!'
run_case 'result overflow' $'calc 1e308 * 1e308\nexit\n' \
    $'Assistant: Calculator error: result out of range.\nBye!'
run_case 'calculator prefix is a whole command' $'calculator\nexit\n' \
    $'Assistant: You said: calculator\nBye!'
run_case 'exit stops processing' $'exit\nthis must not run\n' 'Bye!'
run_case 'exit must match exactly' $'exit now\nexit\n' \
    $'Assistant: You said: exit now\nBye!'
run_case 'empty end of file' '' 'Bye!'
run_case 'end of file after unterminated input' 'last line' \
    $'Assistant: You said: last line\nBye!'
run_case 'CRLF input' $'hello\r\nexit\r\n' \
    $'Assistant: Hello! I am a mock model.\nBye!'
run_case 'empty history and ignored blank lines' $'\n \t\nhistory\nexit\n' \
    $'History (0/5 turns):\nBye!'
run_case 'empty model context' $'recall\nexit\n' \
    $'Assistant: No previous conversation turn.\nBye!'
run_case 'model uses previous context' $'remember this\nrecall\nexit\n' \
    $'Assistant: You said: remember this\nAssistant: Previous input: remember this\nBye!'
run_case 'complete pairs and read-only history' $'hello\ncalc 4 * 5\nhistory\nhistory\nexit\n' \
    $'Assistant: Hello! I am a mock model.\nAssistant: Calculator result: 20\nHistory (2/5 turns):\n1. User: hello\n   Assistant: Hello! I am a mock model.\n2. User: calc 4 * 5\n   Assistant: Calculator result: 20\nHistory (2/5 turns):\n1. User: hello\n   Assistant: Hello! I am a mock model.\n2. User: calc 4 * 5\n   Assistant: Calculator result: 20\nBye!'
run_case 'calculator errors are stored' $'calc 1 / 0\nhistory\nexit\n' \
    $'Assistant: Calculator error: division by zero.\nHistory (1/5 turns):\n1. User: calc 1 / 0\n   Assistant: Calculator error: division by zero.\nBye!'
run_case 'help does not add a turn' $'help\nhistory\nexit\n' \
    $'Commands: hello, calc NUMBER OP NUMBER, history, recall, help, exit\nCalculator operators: + - * / (example: calc 12 * 3)\nOther text is echoed. History keeps the last 5 user/response pairs.\nHistory (0/5 turns):\nBye!'

# Check the exact boundary and repeated eviction, including order and responses.
for turns in 5 6 1000; do
    input=''
    expected=''
    for ((i = 1; i <= turns; i++)); do
        input+="turn$i"$'\n'
        expected+="Assistant: You said: turn$i"$'\n'
    done
    input+=$'history\nrecall\nexit\n'
    expected+=$'History (5/5 turns):\n'
    for ((i = turns - 4, slot = 1; i <= turns; i++, slot++)); do
        expected+="$slot. User: turn$i"$'\n'
        expected+="   Assistant: You said: turn$i"$'\n'
    done
    expected+="Assistant: Previous input: turn$turns"$'\nBye!'
    run_case "$turns turns: limit, eviction order, and recall" "$input" "$expected"
done

# Exercise the input boundary and prove long-line leftovers are discarded.
printf -v max_input '%0255d' 0
run_case '255-byte input and full history copy' "$max_input"$'\nhistory\nexit\n' \
    "Assistant: You said: $max_input"$'\nHistory (1/5 turns):\n1. User: '"$max_input"$'\n   Assistant: You said: '"$max_input"$'\nBye!'
run_case '255-byte input with CRLF' "$max_input"$'\r\nexit\r\n' \
    "Assistant: You said: $max_input"$'\nBye!'
run_case '255-byte input ending at EOF' "$max_input" \
    "Assistant: You said: $max_input"$'\nBye!'
run_case '256-byte line rejected' "${max_input}x"$'\nhistory\nexit\n' \
    $'Input too long (maximum 255 bytes). Turn ignored.\nHistory (0/5 turns):\nBye!'
printf -v long_input '%010000d' 0
run_case 'long line drained before next command' "${long_input}exit"$'\nhello\nhistory\nexit\n' \
    $'Input too long (maximum 255 bytes). Turn ignored.\nAssistant: Hello! I am a mock model.\nHistory (1/5 turns):\n1. User: hello\n   Assistant: Hello! I am a mock model.\nBye!'
run_case 'long line at EOF' "$long_input" \
    $'Input too long (maximum 255 bytes). Turn ignored.\nBye!'

printf 'All %d tests passed (%s).\n' "$passed" "$program"
```
