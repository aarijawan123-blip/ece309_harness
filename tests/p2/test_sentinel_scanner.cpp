#include "core/sentinel_scanner.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

struct SentinelScannerTestAccess {
    static std::size_t pending_size(const SentinelScanner& scanner) {
        return scanner.pending_.size();
    }
};

static const std::string sentinel = "<|end_conversation|>";

static void check_bound(const SentinelScanner& scanner, std::size_t length) {
    assert(SentinelScannerTestAccess::pending_size(scanner) < length);
}

static void clean_text() {
    SentinelScanner scanner(sentinel);
    std::string result;
    for (std::string_view chunk : {"", "Hello", ", world!", "\nMore text.", ""}) {
        auto out = scanner.feed(chunk);
        assert(!out.sentinel_found);
        result += out.safe_text;
        check_bound(scanner, sentinel.size());
    }
    auto tail = scanner.flush();
    assert(!tail.sentinel_found);
    assert(result + tail.safe_text == "Hello, world!\nMore text.");
    assert(SentinelScannerTestAccess::pending_size(scanner) == 0);
    assert(scanner.flush().safe_text.empty());
}

static void whole_sentinel() {
    SentinelScanner scanner(sentinel);
    auto out = scanner.feed("Goodbye." + sentinel + "discard this" + sentinel);
    assert(out.sentinel_found);
    assert(out.safe_text == "Goodbye.");
    assert(SentinelScannerTestAccess::pending_size(scanner) == 0);
    out = scanner.feed("also discarded");
    assert(out.sentinel_found && out.safe_text.empty());
    out = scanner.flush();
    assert(out.sentinel_found && out.safe_text.empty());
    SentinelScanner immediate(sentinel);
    out = immediate.feed(sentinel);
    assert(out.sentinel_found && out.safe_text.empty());
}

static void every_split() {
    const std::string text = "Goodbye." + sentinel + "ignored";
    const std::size_t stop = std::string("Goodbye.").size() + sentinel.size();
    for (std::size_t split = 0; split <= text.size(); ++split) {
        SentinelScanner scanner(sentinel);
        auto first = scanner.feed(std::string_view(text).substr(0, split));
        assert(first.sentinel_found == (split >= stop));
        check_bound(scanner, sentinel.size());
        auto second = scanner.feed(std::string_view(text).substr(split));
        assert(second.sentinel_found);
        assert(first.safe_text + second.safe_text == "Goodbye.");
        check_bound(scanner, sentinel.size());
    }
}

static void character_chunks() {
    const std::string text = "Hello!" + sentinel;
    SentinelScanner scanner(sentinel);
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        auto out = scanner.feed(std::string_view(text).substr(i, 1));
        result += out.safe_text;
        assert(out.sentinel_found == (i + 1 == text.size()));
        check_bound(scanner, sentinel.size());
    }
    assert(result == "Hello!");
}

static void false_alarms_and_partial_end() {
    const std::string text = "<|end_world|><|end_<|end_conversation|X<|end_";
    SentinelScanner scanner(sentinel);
    std::string result;
    for (char ch : text) {
        auto out = scanner.feed(std::string_view(&ch, 1));
        assert(!out.sentinel_found);
        result += out.safe_text;
        check_bound(scanner, sentinel.size());
    }
    auto tail = scanner.flush();
    assert(!tail.sentinel_found);
    assert(result + tail.safe_text == text);

    for (std::size_t length = 0; length < sentinel.size(); ++length) {
        SentinelScanner partial(sentinel);
        auto out = partial.feed(sentinel.substr(0, length));
        assert(!out.sentinel_found && out.safe_text.empty());
        auto flushed = partial.flush();
        assert(!flushed.sentinel_found);
        assert(flushed.safe_text == sentinel.substr(0, length));
    }
}

static void custom_sentinels() {
    SentinelScanner single("!");
    auto out = single.feed("hello");
    assert(!out.sentinel_found && out.safe_text == "hello");
    check_bound(single, 1);
    out = single.feed("!discard");
    assert(out.sentinel_found && out.safe_text.empty());

    const std::string text = "aaabababtail";
    for (std::size_t split = 0; split <= text.size(); ++split) {
        SentinelScanner overlap("abab");
        auto first = overlap.feed(text.substr(0, split));
        auto second = overlap.feed(text.substr(split));
        assert(second.sentinel_found);
        assert(first.safe_text + second.safe_text == "aa");
        check_bound(overlap, 4);
    }
}

static void empty_sentinel_rejected() {
    bool threw = false;
    try { SentinelScanner scanner(""); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

static void binary_text() {
    const std::string prefix("a\0b", 3);
    SentinelScanner scanner(sentinel);
    auto out = scanner.feed(prefix + sentinel);
    assert(out.sentinel_found);
    assert(out.safe_text == prefix);
}

static void bounded_stress() {
    const std::string pattern = "<|end_";
    std::string text(4 * 1024 * 1024, ' ');
    for (std::size_t i = 0; i < text.size(); ++i) text[i] = pattern[i % pattern.size()];

    SentinelScanner scanner(sentinel);
    std::size_t emitted = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        auto out = scanner.feed(std::string_view(text).substr(i, 1));
        assert(!out.sentinel_found);
        check_bound(scanner, sentinel.size());
        assert(text.compare(emitted, out.safe_text.size(), out.safe_text) == 0);
        emitted += out.safe_text.size();
    }
    auto tail = scanner.flush();
    assert(!tail.sentinel_found);
    assert(text.substr(emitted) == tail.safe_text);
    assert(emitted + tail.safe_text.size() == text.size());

    SentinelScanner large_chunk(sentinel);
    auto out = large_chunk.feed(text);
    assert(!out.sentinel_found);
    check_bound(large_chunk, sentinel.size());
    assert(out.safe_text + large_chunk.flush().safe_text == text);
}

int main() {
    clean_text();
    whole_sentinel();
    every_split();
    character_chunks();
    false_alarms_and_partial_end();
    custom_sentinels();
    empty_sentinel_rejected();
    binary_text();
    bounded_stress();
    std::cout << "All 9 SentinelScanner test groups passed.\n";
}
