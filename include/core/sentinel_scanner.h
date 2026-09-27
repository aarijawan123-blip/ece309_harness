#pragma once

#include <string>
#include <string_view>

class SentinelScanner {
public:
    explicit SentinelScanner(std::string sentinel);

    struct Out { std::string safe_text; bool sentinel_found; };

    Out feed(std::string_view chunk);
    Out flush();

private:
    friend struct SentinelScannerTestAccess;
    std::string sentinel_;
    std::string pending_;
    bool stopped_ = false;
};
