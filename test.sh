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
