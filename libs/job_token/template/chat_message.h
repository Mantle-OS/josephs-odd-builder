#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "jobtoken_export.h"

namespace job::token {

enum class ChatRole : uint8_t {
    System = 0,
    User,
    Assistant,
    Tool,
    Custom
};

[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view roleToString(ChatRole role) noexcept
{
    switch (role) {
    case ChatRole::System:
        return "system";
    case ChatRole::User:
        return "user";
    case ChatRole::Assistant:
        return "assistant";
    case ChatRole::Tool:
        return "tool";
    case ChatRole::Custom:
        return "custom";
    default:
        return "user";
    }
}

[[nodiscard]] JOBTOKEN_EXPORT constexpr ChatRole stringToRole(std::string_view str) noexcept
{
    if (str == "system")
        return ChatRole::System;
    if (str == "user")
        return ChatRole::User;
    if (str == "assistant")
        return ChatRole::Assistant;
    if (str == "tool")
        return ChatRole::Tool;
    return ChatRole::Custom;
}

struct JOBTOKEN_EXPORT ChatMessage {
    ChatRole    role{ChatRole::User};
    std::string content;
    std::string customRole; // Used when role == ChatRole::Custom
    std::string name;       // Optional tool or participant identifier

    [[nodiscard]] std::string_view roleName() const noexcept
    {
        if (role == ChatRole::Custom && !customRole.empty())
            return customRole;
        return roleToString(role);
    }
};

} // namespace job::token