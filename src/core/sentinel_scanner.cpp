#include "core/sentinel_scanner.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {
    if (sentinel_.empty()) {
        throw std::invalid_argument("Sentinel must not be empty");
    }
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    if (stopped_) return {"", true};

    std::string text = pending_;
    text.append(chunk);
    const std::size_t match = text.find(sentinel_);
    if (match != std::string::npos) {
        std::string safe = text.substr(0, match);
        pending_.clear();
        stopped_ = true;
        return {std::move(safe), true};
    }

    // A sentinel crossing the next boundary can use at most this many old bytes.
    const std::size_t keep = std::min(text.size(), sentinel_.size() - 1);
    const std::size_t safe_count = text.size() - keep;
    pending_ = text.substr(safe_count);
    text.resize(safe_count);
    return {std::move(text), false};
}

SentinelScanner::Out SentinelScanner::flush() {
    std::string safe;
    safe.swap(pending_);
    return {std::move(safe), stopped_};
}
