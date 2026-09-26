#pragma once

#include <string>
#include <utility>

enum class Role { System, User, Assistant };

class Message {
public:
    // Empty System messages can fill unused Conversation array slots.
    Message() : role_(Role::System), content_() {}

    // Own the text, so the caller's string does not need to stay alive.
    Message(Role role, std::string content)
        : role_(role), content_(std::move(content)) {}

    Role role() const noexcept { return role_; }

    // Read the stored text without copying it or allowing changes through it.
    const std::string& content() const noexcept { return content_; }

private:
    Role role_;
    std::string content_;
};
