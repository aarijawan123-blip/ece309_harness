// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/file_tools.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <memory>
#include <set>
#include <map>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
}

// ------------------------------------------------------------- your tests
//
// Still to write (spec §6 has the full list). Suggested names:
//   reg_null_rejected, reg_for_each_visits_each_once
//   reg_10000_tools               REQUIRED: print bucket_count / load_factor /
//                                 max_probe_length, check the bound
//   reg_alternative_hash          REQUIRED: longest chain of a weak hash
//   ws_<case>                     REQUIRED: every row of the table in spec §4.4
//   calc_<case>                   left-to-right, parentheses, negatives

namespace {
class CountedTool : public NamedTool {
public:
    CountedTool(std::string name, int& destroyed)
        : NamedTool(std::move(name)), destroyed_(destroyed) {}
    ~CountedTool() override { ++destroyed_; }
private:
    int& destroyed_;
};
}

TEST(reg_empty) {
    const ToolRegistry r;
    CHECK(r.size() == 0);
    CHECK(r.bucket_count() >= 1);
    CHECK(r.bucket_count() <= 64);
    CHECK(r.load_factor() == 0.0);
    CHECK(r.max_probe_length() == 0);
    CHECK(r.find("missing") == nullptr);
    int visits = 0;
    r.for_each([&](const Tool&) { ++visits; });
    CHECK(visits == 0);
}

TEST(reg_null_rejected) {
    ToolRegistry r;
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);
    REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") != nullptr);
}

TEST(reg_duplicate_ownership) {
    int old_destroyed = 0;
    int new_destroyed = 0;
    {
        ToolRegistry r;
        REQUIRE(r.add(std::make_unique<CountedTool>("alpha", old_destroyed)));
        Tool* original = r.find("alpha");
        CHECK(!r.add(std::make_unique<CountedTool>("alpha", new_destroyed)));
        CHECK(new_destroyed == 1);
        CHECK(old_destroyed == 0);
        CHECK(r.find("alpha") == original);
        CHECK(r.size() == 1);
    }
    CHECK(old_destroyed == 1);
    CHECK(new_destroyed == 1);
}

TEST(reg_destroys_all_after_resizing) {
    int destroyed = 0;
    {
        ToolRegistry r;
        for (int i = 0; i < 200; ++i) {
            REQUIRE(r.add(std::make_unique<CountedTool>(std::to_string(i), destroyed)));
        }
        CHECK(destroyed == 0);
    }
    CHECK(destroyed == 200);
}

TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    for (int i = 0; i < 100; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>(std::to_string(i))));
    }
    std::map<std::string, int> visits;
    const ToolRegistry& view = r;
    view.for_each([&](const Tool& tool) { ++visits[tool.name()]; });
    CHECK(visits.size() == 100);
    for (int i = 0; i < 100; ++i) CHECK(visits[std::to_string(i)] == 1);
}

TEST(reg_collision_chain) {
    ToolRegistry r;
    std::vector<std::string> names;
    for (int i = 0; names.size() < 8; ++i) {
        std::string name = "collision_" + std::to_string(i);
        if (ToolRegistry::hash(name) % r.bucket_count() == 0) names.push_back(name);
    }
    for (const auto& name : names) REQUIRE(r.add(std::make_unique<NamedTool>(name)));
    CHECK(r.max_probe_length() == names.size());
    for (const auto& name : names) {
        REQUIRE(r.find(name) != nullptr);
        CHECK(r.find(name)->name() == name);
    }
    CHECK(r.find("absent") == nullptr);
}

TEST(reg_resize_threshold) {
    ToolRegistry r;
    const std::size_t initial = r.bucket_count();
    for (std::size_t i = 0; i < initial; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>(std::to_string(i))));
        CHECK(r.bucket_count() == initial);
    }
    CHECK(r.load_factor() == 1.0);
    CHECK(!r.add(std::make_unique<NamedTool>("0")));
    CHECK(!r.add(nullptr));
    CHECK(r.bucket_count() == initial);
    REQUIRE(r.add(std::make_unique<NamedTool>("next")));
    CHECK(r.bucket_count() == initial * 2);
    CHECK(r.size() == initial + 1);
    CHECK(r.load_factor() == static_cast<double>(r.size()) / r.bucket_count());
}

TEST(reg_exact_names) {
    ToolRegistry r;
    const std::vector<std::string> names = {"", "a", "A", "ab", "ba", std::string("a\0b", 3)};
    for (const auto& name : names) REQUIRE(r.add(std::make_unique<NamedTool>(name)));
    CHECK(r.size() == names.size());
    for (const auto& name : names) {
        REQUIRE(r.find(name) != nullptr);
        CHECK(r.find(name)->name() == name);
    }
    CHECK(r.find("abc") == nullptr);
}

TEST(reg_hash_uses_whole_key) {
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba"));
    std::string name(1000, 'x');
    auto original = ToolRegistry::hash(name);
    for (std::size_t i = 0; i < name.size(); ++i) {
        name[i] = 'y';
        CHECK(ToolRegistry::hash(name) != original);
        name[i] = 'x';
    }
    CHECK(ToolRegistry::hash(std::string_view("a\0b", 3)) != ToolRegistry::hash("a"));
}

TEST(reg_hash_distinct_values) {
    std::set<std::size_t> hashes;
    for (int i = 0; i < 10000; ++i) hashes.insert(ToolRegistry::hash("tool_" + std::to_string(i)));
    CHECK(hashes.size() >= 9990);
}

TEST(reg_10000_tools) {
    ToolRegistry r;
    std::vector<Tool*> pointers;
    for (int i = 0; i < 10000; ++i) {
        std::string name = "tool_" + std::to_string(i);
        auto tool = std::make_unique<NamedTool>(name);
        pointers.push_back(tool.get());
        REQUIRE(r.add(std::move(tool)));
        CHECK(r.size() == static_cast<std::size_t>(i + 1));
        CHECK(r.load_factor() <= 1.0);
    }
    for (int i = 0; i < 10000; ++i) {
        CHECK(r.find("tool_" + std::to_string(i)) == pointers[i]);
    }
    CHECK(r.max_probe_length() <= 64);

    std::vector<int> weak_counts(r.bucket_count(), 0);
    int weak_longest = 0;
    for (int i = 0; i < 10000; ++i) {
        std::string name = "tool_" + std::to_string(i);
        std::size_t index = (static_cast<unsigned char>(name[0]) + name.size()) % r.bucket_count();
        int count = ++weak_counts[index];
        if (count > weak_longest) weak_longest = count;
    }
    CHECK(weak_longest == 9000);
    std::cout << "10000 tools: buckets=" << r.bucket_count()
              << ", load=" << r.load_factor()
              << ", longest chain=" << r.max_probe_length()
              << ", weak hash longest chain=" << weak_longest << '\n';
}

TEST(ws_accepts_required_paths) {
    TempDir d;
    Workspace ws(d.path());
    const std::vector<std::pair<std::string, fs::path>> cases = {
        {"notes/todo.txt", ws.root() / "notes/todo.txt"},
        {"./notes//todo.txt", ws.root() / "notes/todo.txt"},
        {".", ws.root()},
        {"a/../b", ws.root() / "b"},
        {"a/..", ws.root()},
        {"new/dir/file.txt", ws.root() / "new/dir/file.txt"},
        {"my..notes.txt", ws.root() / "my..notes.txt"}
    };
    for (const auto& item : cases) {
        fs::path out;
        std::string reason;
        REQUIRE(ws.resolve(item.first, &out, &reason));
        CHECK(out == item.second);
    }
    CHECK(fs::is_empty(d.path()));
}

TEST(ws_rejects_empty_path) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("", &out, &reason));
    CHECK(reason == "empty path");
}

TEST(ws_rejects_nul_bytes) {
    TempDir d;
    Workspace ws(d.path());
    const std::vector<std::string> paths = {
        std::string("notes") + '\0' + "/x",
        std::string(1, '\0') + "notes",
        std::string("notes") + '\0'
    };
    for (const auto& path : paths) {
        fs::path out;
        std::string reason;
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
    }
}

TEST(ws_rejects_absolute_paths) {
    TempDir d;
    Workspace ws(d.path());
    const std::vector<std::string> paths = {
        "/etc/passwd", "/", "//etc/passwd", (ws.root() / "inside.txt").string()
    };
    for (const auto& path : paths) {
        fs::path out;
        std::string reason;
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
    }
}

TEST(ws_rejects_normalized_parent_paths) {
    TempDir d;
    Workspace ws(d.path() / "ws");
    const std::vector<std::string> paths = {
        "..", "../etc/passwd", "a/../../b",
        "notes/../../ws/notes/todo.txt", "./../x", "a/b/../../../x"
    };
    for (const auto& path : paths) {
        fs::path out;
        std::string reason;
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
    }
}

TEST(ws_resolves_without_existing_root) {
    TempDir d;
    Workspace ws(d.path() / "missing");
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("new/dir/file.txt", &out, &reason));
    CHECK(out == ws.root() / "new/dir/file.txt");
    CHECK(!fs::exists(ws.root()));
}

TEST(ws_uses_string_view_length) {
    TempDir d;
    Workspace ws(d.path());
    std::string text = "notes.txt/../../outside";
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve(std::string_view(text.data(), 9), &out, &reason));
    CHECK(out == ws.root() / "notes.txt");
}

TEST(ws_leaves_symlinks_to_provided_check) {
    TempDir d;
    TempDir outside;
    Workspace ws(d.path());
    fs::create_directory_symlink(outside.path(), d.path() / "link");
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("link/new.txt", &out, &reason));
    CHECK(out == ws.root() / "link/new.txt");
    CHECK(!ws.check_symlinks(out, &reason));
    CHECK(!reason.empty());
    CHECK(fs::is_empty(outside.path()));
}

TEST(ws_file_tools_round_trip) {
    TempDir d;
    ToolContext ctx{Workspace(d.path())};
    WriteFileTool writer;
    ReadFileTool reader;
    ListDirTool listing;
    auto args = args1("path", JsonValue::string("./notes//todo.txt"));
    args.set("content", JsonValue::string("hello\n"));
    REQUIRE(writer.invoke(args, ctx).ok);
    auto read = reader.invoke(args1("path", JsonValue::string("notes/todo.txt")), ctx);
    REQUIRE(read.ok);
    CHECK(read.content == "hello\n");
    auto listed = listing.invoke(args1("path", JsonValue::string("notes")), ctx);
    REQUIRE(listed.ok);
    CHECK(listed.content == "todo.txt\t6");
}

TEST(ws_file_tools_reject_escape) {
    TempDir d;
    fs::create_directory(d.path() / "ws");
    write_text(d.path() / "outside.txt", "keep");
    ToolContext ctx{Workspace(d.path() / "ws")};
    WriteFileTool writer;
    ReadFileTool reader;
    ListDirTool listing;
    auto args = args1("path", JsonValue::string("../outside.txt"));
    args.set("content", JsonValue::string("changed"));
    CHECK(!writer.invoke(args, ctx).ok);
    CHECK(!reader.invoke(args, ctx).ok);
    CHECK(!listing.invoke(args1("path", JsonValue::string("..")), ctx).ok);
    CHECK(read_text(d.path() / "outside.txt") == "keep");
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }
