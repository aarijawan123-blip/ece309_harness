// Standalone Part 2 checks, runnable before the other core classes exist.
#include "core/message.h"

#include <cassert>
#include <iostream>
#include <type_traits>

int main() {
    // The default constructor must also work for array slots.
    const Message empty;
    assert(empty.role() == Role::System);
    assert(empty.content().empty());
    const Message slots[3];
    for (const Message& slot : slots) {
        assert(slot.role() == Role::System);
        assert(slot.content().empty());
    }

    // All three roles retain their supplied text, including empty content.
    const Message system(Role::System, "Be concise.");
    const Message user(Role::User, "hello");
    const Message assistant(Role::Assistant, "");
    assert(system.role() == Role::System);
    assert(system.content() == "Be concise.");
    assert(user.role() == Role::User);
    assert(user.content() == "hello");
    assert(assistant.role() == Role::Assistant);
    assert(assistant.content().empty());

    // Changing the caller's string must not change the message's own content.
    std::string original = "first line\nsecond line";
    const Message owned(Role::User, original);
    original = "changed";
    assert(owned.content() == "first line\nsecond line");

    // Check the specified accessor signatures at compile time.
    static_assert(noexcept(empty.role()));
    static_assert(noexcept(empty.content()));
    static_assert(std::is_same_v<decltype(empty.role()), Role>);
    static_assert(std::is_same_v<decltype(empty.content()), const std::string&>);

    std::cout << "Message checks passed.\n";
}
