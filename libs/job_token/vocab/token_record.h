#pragma once

#include <memory>
#include <string>

#include "job_token_types.h"
#include "job_token_enums.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT TokenRecord
{
public:
    using Ptr  = std::shared_ptr<TokenRecord>;
    using WPtr = std::weak_ptr<TokenRecord>;
    using UPtr = std::unique_ptr<TokenRecord>;

    TokenRecord() = default;
    ~TokenRecord() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<TokenRecord>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<TokenRecord>();
    }

    TokenRecord(const TokenRecord &) = default;
    TokenRecord &operator=(const TokenRecord &) = default;
    TokenRecord(TokenRecord &&) noexcept = default;
    TokenRecord &operator=(TokenRecord &&) noexcept = default;

    [[nodiscard]] const std::string &text() const noexcept
    {
        return m_text;
    }

    void setText(const std::string &text)
    {
        m_text = text;
    }

    [[nodiscard]] TokenId id() const noexcept
    {
        return m_id;
    }

    void setId(TokenId id) noexcept
    {
        m_id = id;
    }

    [[nodiscard]] float score() const noexcept
    {
        return m_score;
    }

    void setScore(float score) noexcept
    {
        m_score = score;
    }

    [[nodiscard]] StructuralType type() const noexcept
    {
        return m_type;
    }

    void setType(StructuralType type) noexcept
    {
        m_type = type;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_id != kInvalidToken;
    }

    [[nodiscard]] bool isSpecial() const noexcept
    {
        return m_type == StructuralType::Control || m_type == StructuralType::UserDefined;
    }

    [[nodiscard]] bool isByte() const noexcept
    {
        return m_type == StructuralType::Byte;
    }

    [[nodiscard]] bool isUnused() const noexcept
    {
        return m_type == StructuralType::Unused;
    }

    void clear() noexcept
    {
        m_text.clear();
        m_id = kInvalidToken;
        m_score = 0.0f;
        m_type = StructuralType::Normal;
    }

private:
    std::string     m_text;
    TokenId         m_id{kInvalidToken};
    float           m_score{0.0f};
    StructuralType  m_type{StructuralType::Normal};
};

} // namespace job::token