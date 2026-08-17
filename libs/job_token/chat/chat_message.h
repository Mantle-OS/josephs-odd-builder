#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "job_token_enums.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ChatMessage
{
public:
    using Ptr  = std::shared_ptr<ChatMessage>;
    using WPtr = std::weak_ptr<ChatMessage>;
    using UPtr = std::unique_ptr<ChatMessage>;

    ChatMessage() = default;
    ~ChatMessage() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<ChatMessage>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<ChatMessage>();
    }

    ChatMessage(const ChatMessage &) = default;
    ChatMessage &operator=(const ChatMessage &) = default;
    ChatMessage(ChatMessage &&) noexcept = default;
    ChatMessage &operator=(ChatMessage &&) noexcept = default;

    [[nodiscard]] ChatRole role() const noexcept
    {
        return m_role;
    }

    void setRole(ChatRole role) noexcept
    {
        m_role = role;
    }

    [[nodiscard]] const std::string &content() const noexcept
    {
        return m_content;
    }

    void setContent(const std::string &content)
    {
        m_content = content;
    }

    [[nodiscard]] const std::string &customRole() const noexcept
    {
        return m_customRole;
    }

    void setCustomRole(const std::string &customRole)
    {
        m_customRole = customRole;
    }

    [[nodiscard]] const std::string &name() const noexcept
    {
        return m_name;
    }

    void setName(const std::string &name)
    {
        m_name = name;
    }

    [[nodiscard]] std::string_view roleName() const noexcept
    {
        if (m_role == ChatRole::Custom && !m_customRole.empty())
            return m_customRole;

        return roleToString(m_role);
    }

    void clear() noexcept
    {
        m_role = ChatRole::User;
        m_content.clear();
        m_customRole.clear();
        m_name.clear();
    }

private:
    ChatRole    m_role{ChatRole::User};
    std::string m_content;
    std::string m_customRole;
    std::string m_name;
};

} // namespace job::token