#include "tools/tool_registry.h"

#include <cstdint>
#include <utility>

ToolRegistry::ToolRegistry() : buckets_(16) {}
ToolRegistry::~ToolRegistry() = default;

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    std::uint64_t value = 1469598103934665603ULL;
    for (unsigned char c : key) {
        value ^= c;
        value *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(value);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) return false;
    std::string name = tool->name();
    if (find(name)) return false;

    // Grow before inserting so the load factor stays at most 1.
    if (size_ == buckets_.size()) rehash(buckets_.size() * 2);
    std::size_t index = hash(name) % buckets_.size();
    buckets_[index].push_back({std::move(name), std::move(tool)});
    ++size_;
    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    const auto& bucket = buckets_[hash(name) % buckets_.size()];
    for (const auto& entry : bucket) {
        if (entry.name == name) return entry.tool.get();
    }
    return nullptr;
}

void ToolRegistry::rehash(std::size_t count) {
    std::vector<std::list<Entry>> new_buckets(count);
    for (auto& bucket : buckets_) {
        while (!bucket.empty()) {
            std::size_t index = hash(bucket.front().name) % count;
            // Splice transfers the list node without copying or moving its Tool.
            new_buckets[index].splice(new_buckets[index].end(), bucket, bucket.begin());
        }
    }
    buckets_.swap(new_buckets);
}

std::size_t ToolRegistry::size() const noexcept {
    return size_;
}

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const auto& bucket : buckets_) {
        for (const auto& entry : bucket) fn(*entry.tool);
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept {
    return buckets_.size();
}

double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(size_) / buckets_.size();
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const auto& bucket : buckets_) {
        if (bucket.size() > longest) longest = bucket.size();
    }
    return longest;
}
