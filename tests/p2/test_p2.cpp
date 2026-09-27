// Keep assertions active in Release builds too.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "test_message.h"
#include "test_conversation.h"
#include "test_sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>

class TestFile {
public:
    std::string path;
    explicit TestFile(const std::string& text = "") {
        path = (std::filesystem::temp_directory_path() / "ece309-p2-XXXXXX").string();
        const int fd = mkstemp(path.data());
        if (fd == -1) throw std::runtime_error("Cannot create test file");
        close(fd);
        std::ofstream file(path);
        file << text;
        file.close();
        assert(file.good());
    }
    ~TestFile() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
    TestFile(const TestFile&) = delete;
    TestFile& operator=(const TestFile&) = delete;
};

class TestInput : public InputSource {
public:
    explicit TestInput(const std::string& text) : input_(text) {}
    std::string read_line() override {
        ++reads;
        std::string line;
        eof_ = !static_cast<bool>(std::getline(input_, line));
        return line;
    }
    bool is_eof() const override { return eof_; }
    int reads = 0;
private:
    std::istringstream input_;
    bool eof_ = false;
};

class TestOutput : public OutputSink {
public:
    void write(std::string_view text) override { content.append(text); }
    std::string content;
};

static void turn_limit() {
    TestFile script("role: assistant\none\n---\nrole: assistant\ntwo\n");
    for (int limit : {0, 2}) {
        HarnessConfig cfg;
        cfg.max_turns = limit;
        cfg.system_message = "system";
        Harness harness(std::make_unique<ScriptedModelClient>(script.path), cfg);
        TestInput input("first\nsecond\nunused\n");
        TestOutput output;
        assert(harness.run(input, output).kind == StopReason::Kind::TurnLimit);
        assert(input.reads == limit);
        const auto& conv = harness.conversation();
        assert(conv.size() == static_cast<std::size_t>(1 + 2 * limit));
        assert(conv.at(0).role() == Role::System);
        assert(conv.at(0).content() == "system");
        if (limit == 2) {
            assert(output.content == "you> assistant> one\nyou> assistant> two\n");
            assert(conv.at(1).role() == Role::User && conv.at(1).content() == "first");
            assert(conv.at(2).role() == Role::Assistant && conv.at(2).content() == "one");
            assert(conv.at(3).content() == "second");
            assert(conv.at(4).content() == "two");
        } else assert(output.content.empty());
    }
}

static void default_turn_limit() {
    std::string script_text, inputs, expected;
    for (int i = 0; i < 21; ++i) {
        script_text += "role: assistant\nreply\n---\n";
        inputs += "hello\n";
        if (i < 20) expected += "you> assistant> reply\n";
    }
    TestFile script(script_text);
    Harness harness(std::make_unique<ScriptedModelClient>(script.path), {});
    TestInput input(inputs);
    TestOutput output;
    assert(harness.run(input, output).kind == StopReason::Kind::TurnLimit);
    assert(input.reads == 20);
    assert(harness.conversation().size() == 40);
    assert(output.content == expected);
}

static void sentinel_halt() {
    for (std::size_t chunk = 1; chunk <= sentinel.size() + 10; ++chunk) {
        TestFile script("chunk: " + std::to_string(chunk) +
                        "\nrole: assistant\nGoodbye." + sentinel + "discard\n");
        HarnessConfig cfg;
        cfg.max_turns = 1; // The sentinel takes precedence on the last turn.
        Harness harness(std::make_unique<ScriptedModelClient>(script.path), cfg);
        TestInput input("bye\nunused\n");
        TestOutput output;
        auto reason = harness.run(input, output);
        assert(reason.kind == StopReason::Kind::Sentinel);
        assert(reason.detail == "stop sentinel after 1 turns");
        assert(input.reads == 1);
        assert(output.content == "you> assistant> Goodbye.\n");
        assert(harness.conversation().size() == 2);
        assert(harness.conversation().at(1).content() == "Goodbye." + sentinel);
    }
}

static void eof_shutdown() {
    TestFile script("chunk: 1\nrole: assistant\npartial <|end_\n");
    for (const std::string text : {"", "hello\n"}) {
        Harness harness(std::make_unique<ScriptedModelClient>(script.path), {});
        TestInput input(text);
        TestOutput output;
        assert(harness.run(input, output).kind == StopReason::Kind::UserExit);
        if (text.empty()) {
            assert(harness.conversation().size() == 0);
            assert(output.content == "you> ");
        } else {
            assert(harness.conversation().size() == 2);
            assert(harness.conversation().at(1).content() == "partial <|end_");
            assert(output.content == "you> assistant> partial <|end_\nyou> ");
        }
    }
}

static void blank_input() {
    TestFile script("role: assistant\nbye" + sentinel + "\n");
    Harness harness(std::make_unique<ScriptedModelClient>(script.path), {});
    TestInput input("\n\nhello\n");
    TestOutput output;
    assert(harness.run(input, output).kind == StopReason::Kind::Sentinel);
    assert(input.reads == 3);
    assert(harness.conversation().size() == 2);
    assert(harness.conversation().at(0).content() == "hello");
    assert(output.content == "you> you> you> assistant> bye\n");
}

static void script_exhaustion() {
    TestFile script("role: assistant\none\n");
    Harness harness(std::make_unique<ScriptedModelClient>(script.path), {});
    TestInput input("hello\nmore\n");
    TestOutput output;
    const auto reason = harness.run(input, output);
    assert(reason.kind == StopReason::Kind::ClientError);
    assert(reason.detail.find("script exhausted") != std::string::npos);
    assert(harness.conversation().size() == 3);
    assert(harness.conversation().at(2).role() == Role::User);
    assert(output.content == "you> assistant> one\nyou> assistant> \n");
}

static void replay_exhaustion() {
    TestFile transcript("role: user\nhello\n---\nrole: assistant\none\n");
    Harness harness(std::make_unique<ReplayModelClient>(transcript.path), {});
    TestInput input("hello\nmore\n");
    TestOutput output;
    const auto reason = harness.run(input, output);
    assert(reason.kind == StopReason::Kind::ClientError);
    assert(reason.detail.find("transcript exhausted") != std::string::npos);
    assert(harness.conversation().size() == 3);
    assert(output.content == "you> assistant> one\nyou> assistant> \n");
}

static void save_conversation(const Conversation& conv, const std::string& path) {
    std::ofstream file(path);
    assert(file.is_open());
    for (std::size_t i = 0; i < conv.size(); ++i) {
        if (i != 0) file << "---\n";
        const auto& message = conv.at(i);
        const char* role = message.role() == Role::System ? "system" :
                           message.role() == Role::User ? "user" : "assistant";
        file << "role: " << role << '\n' << message.content() << '\n';
    }
    file.close();
    assert(file.good());
}

static void transcript_round_trip() {
    for (bool use_sentinel : {false, true}) {
        const std::string ending = use_sentinel ? sentinel : "";
        TestFile script("role: system\nBe concise.\n---\nchunk: 2\n"
                        "role: assistant\nFirst line\nSecond line\n---\n"
                        "chunk: 1\nrole: assistant\nGoodbye." + ending + "\n");
        auto client = std::make_unique<ScriptedModelClient>(script.path);
        HarnessConfig cfg;
        cfg.system_message = client->system_message();
        Harness original(std::move(client), cfg);
        TestInput input("hello\nbye\n");
        TestOutput output;
        const auto reason = original.run(input, output);
        assert(reason.kind == (use_sentinel ? StopReason::Kind::Sentinel :
                                             StopReason::Kind::UserExit));
        assert(original.conversation().size() == 5);
        assert(original.conversation().at(0).content() == "Be concise.");
        TestFile transcript;
        save_conversation(original.conversation(), transcript.path);
        auto replay_client = std::make_unique<ReplayModelClient>(transcript.path);
        cfg.system_message = replay_client->system_message();
        Harness replay(std::move(replay_client), cfg);
        TestInput replay_input("hello\nbye\n");
        TestOutput replay_output;
        const auto replay_reason = replay.run(replay_input, replay_output);
        assert(replay_reason.kind == reason.kind);
        assert(replay_reason.detail == reason.detail);
        assert(replay_output.content == output.content);
        const auto& a = original.conversation();
        const auto& b = replay.conversation();
        assert(a.size() == b.size());
        for (std::size_t i = 0; i < a.size(); ++i) {
            assert(a.at(i).role() == b.at(i).role());
            assert(a.at(i).content() == b.at(i).content());
        }
    }
}

int main() {
    run_message_tests();
    run_conversation_tests();
    run_sentinel_scanner_tests();
    turn_limit();
    default_turn_limit();
    sentinel_halt();
    eof_shutdown();
    blank_input();
    script_exhaustion();
    replay_exhaustion();
    transcript_round_trip();
    std::cout << "All 8 harness integration test groups passed.\n"
              << "All 26 Project 2 test groups passed.\n";
}
