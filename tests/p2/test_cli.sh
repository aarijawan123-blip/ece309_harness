#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")/../.."
program=${1:-./build/miniharness}
scratch=$(mktemp -d)
trap 'rm -rf -- "$scratch"' EXIT
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1

for mode in sentinel eof limit; do
    flags=()
    reply='Goodbye.'
    stored=$reply
    prompt=''
    case "$mode" in
        sentinel)
            stored+='<|end_conversation|>'
            reply="${stored}discard"
            reason='stop sentinel after 1 turns'
            ;;
        eof)
            prompt='you> '
            reason='EOF detected'
            ;;
        limit)
            flags=(--max-turns 1)
            reason='Max turn limit reached'
            ;;
    esac
    printf 'role: system\nBe concise.\n---\nchunk: 1\nrole: assistant\n%s\n' \
        "$reply" > "$scratch/input.script"
    actual=$(printf 'hello\n' | "$program" --script "$scratch/input.script" \
        --save "$scratch/transcript.txt" "${flags[@]}")
    expected=$(printf 'you> assistant> Goodbye.\n%s[conversation ended: %s]\n[Transcript saved to %s]\n' \
        "$prompt" "$reason" "$scratch/transcript.txt")
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAIL: %s output\nExpected:\n%s\nActual:\n%s\n' "$mode" "$expected" "$actual" >&2
        exit 1
    fi
    printf 'role: system\nBe concise.\n---\nrole: user\nhello\n---\nrole: assistant\n%s\n' \
        "$stored" > "$scratch/expected.txt"
    diff -u "$scratch/expected.txt" "$scratch/transcript.txt"
    printf 'PASS: CLI %s shutdown and saved transcript\n' "$mode"
done
echo 'All 3 CLI checks passed.'
