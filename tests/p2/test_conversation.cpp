#include "core/conversation.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

// Inspect the required growth invariant without adding a public capacity API.
struct ConversationTestAccess {
    static std::size_t capacity(const Conversation& conv) {
        return conv.capacity_;
    }
};

static void check_empty(const Conversation& conv) {
    assert(conv.size() == 0);
    assert(conv.begin() == nullptr);
    assert(conv.begin() == conv.end());
    assert(ConversationTestAccess::capacity(conv) == 0);
}

static void empty_and_bounds() {
    Conversation conv;
    check_empty(conv);
    bool threw = false;
    try { conv.at(0); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);
    conv.append(Message(Role::User, "one"));
    for (std::size_t index : {conv.size(), static_cast<std::size_t>(-1)}) {
        threw = false;
        try { conv.at(index); } catch (const std::out_of_range&) { threw = true; }
        assert(threw);
    }
    assert(conv.at(0).content() == "one");
}

static void growth_and_iteration() {
    Conversation conv;
    conv.append(Message(Role::System, "Stay concise."));
    std::size_t expected_capacity = 1;
    for (std::size_t i = 1; i <= 4096; ++i) {
        const bool must_grow = conv.size() == expected_capacity;
        const Message* previous = conv.begin();
        conv.append(Message(i % 2 ? Role::User : Role::Assistant,
                            std::to_string(i)));
        if (must_grow) expected_capacity *= 2;
        else assert(conv.begin() == previous);
        assert(ConversationTestAccess::capacity(conv) == expected_capacity);
        assert(conv.size() == i + 1);
        assert(conv.at(0).role() == Role::System);
        assert(conv.at(0).content() == "Stay concise.");
    }
    std::size_t index = 0;
    for (const Message& message : conv) {
        if (index != 0) {
            assert(message.content() == std::to_string(index));
            assert(message.role() == (index % 2 ? Role::User : Role::Assistant));
        }
        ++index;
    }
    assert(index == conv.size());
    assert(conv.end() == conv.begin() + conv.size());
}

static void system_ordering() {
    for (Role first : {Role::System, Role::User}) {
        Conversation conv;
        conv.append(Message(first, "first"));
        bool threw = false;
        try { conv.append(Message(Role::System, "late")); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
        assert(conv.size() == 1);
        assert(conv.at(0).content() == "first");
    }
}

static void copy_constructor() {
    Conversation copy;
    {
        Conversation original;
        original.append(Message(Role::User, std::string(1000, 'x')));
        Conversation constructed(original);
        assert(constructed.begin() != original.begin());
        assert(constructed.at(0).content().data() != original.at(0).content().data());
        original = Conversation();
        assert(constructed.at(0).content() == std::string(1000, 'x'));
        copy = constructed;
    }
    assert(copy.at(0).content() == std::string(1000, 'x'));
    Conversation empty;
    Conversation empty_copy(empty);
    check_empty(empty_copy);
}

static void copy_assignment() {
    Conversation source;
    source.append(Message(Role::User, "source"));
    Conversation target;
    target.append(Message(Role::Assistant, "old"));
    assert(&(target = source) == &target);
    assert(target.begin() != source.begin());
    assert(target.size() == 1);
    source.append(Message(Role::Assistant, "new"));
    assert(target.size() == 1);
    assert(target.at(0).content() == "source");
    const Message* saved = target.begin();
    target = target;
    assert(target.begin() == saved);
    assert(target.at(0).content() == "source");
    target = Conversation{};
    check_empty(target);
    Conversation empty;
    source = empty;
    check_empty(source);
}

static void move_constructor() {
    Conversation source;
    source.append(Message(Role::User, "moved"));
    const Message* saved = source.begin();
    Conversation target(std::move(source));
    assert(target.begin() == saved);
    assert(target.size() == 1);
    assert(target.at(0).content() == "moved");
    check_empty(source);
    source.append(Message(Role::User, "reused"));
    assert(source.at(0).content() == "reused");
    assert(target.at(0).content() == "moved");
    Conversation empty;
    Conversation moved_empty(std::move(empty));
    check_empty(empty);
    check_empty(moved_empty);
}

static void move_into(Conversation& target, Conversation& source) {
    target = std::move(source);
}

static void move_assignment() {
    Conversation source;
    source.append(Message(Role::User, "moved"));
    const Message* saved = source.begin();
    Conversation target;
    target.append(Message(Role::User, std::string(2000, 'z')));
    assert(&(target = std::move(source)) == &target);
    assert(target.begin() == saved);
    assert(target.at(0).content() == "moved");
    check_empty(source);
    move_into(target, target);
    assert(target.begin() == saved);
    assert(target.size() == 1);
    source = target;
    assert(source.begin() != target.begin());
    assert(source.at(0).content() == "moved");
    Conversation empty;
    target = std::move(empty);
    check_empty(target);
    check_empty(empty);
}

static void append_existing_message() {
    Conversation conv;
    conv.append(Message(Role::User, std::string(1000, 'a')));
    conv.append(conv.at(0)); // The source is inside the array that must grow.
    assert(conv.size() == 2);
    assert(conv.at(0).content() == std::string(1000, 'a'));
    assert(conv.at(1).content() == conv.at(0).content());
}

int main() {
    static_assert(std::is_nothrow_move_assignable_v<Message>);
    static_assert(std::is_nothrow_move_constructible_v<Conversation>);
    static_assert(std::is_nothrow_move_assignable_v<Conversation>);
    empty_and_bounds();
    growth_and_iteration();
    system_ordering();
    copy_constructor();
    copy_assignment();
    move_constructor();
    move_assignment();
    append_existing_message();
    std::cout << "All 8 Conversation test groups passed.\n";
}
