#pragma once

#include <cstddef>
#include <memory>
#include <string_view>

#include "job_yaml_source_range.h"

namespace job::yaml {

enum class YamlDiagnosticKind : unsigned char
{
    None,

    UnexpectedToken,
    UnexpectedEnd,
    InvalidIndentation,

    InvalidAnchor,
    InvalidAlias,
    InvalidTag,

    InvalidScalar,
    InvalidFlowCollection,
    InvalidBlockScalar,

    InvalidDirective,
    InvalidDocument,

    DestinationRejected
};

class YamlDiagnostic
{
public:
    using Ptr = std::shared_ptr<YamlDiagnostic>;
    using WPtr = std::weak_ptr<YamlDiagnostic>;
    using UPtr = std::unique_ptr<YamlDiagnostic>;

    YamlDiagnostic() = default;
    ~YamlDiagnostic() = default;

    YamlDiagnostic(const YamlDiagnostic &) = default;
    YamlDiagnostic &operator=(const YamlDiagnostic &) = default;

    YamlDiagnostic(YamlDiagnostic &&) noexcept = default;
    YamlDiagnostic &operator=(YamlDiagnostic &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<YamlDiagnostic>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<YamlDiagnostic>();
    }

    [[nodiscard]] constexpr YamlDiagnosticKind kind() const noexcept
    {
        return m_kind;
    }

    [[nodiscard]] constexpr YamlSourceRange range() const noexcept
    {
        return m_range;
    }

    [[nodiscard]] constexpr std::size_t offset() const noexcept
    {
        return m_range.offset;
    }

    [[nodiscard]] constexpr std::size_t line() const noexcept
    {
        return m_line;
    }

    [[nodiscard]] constexpr std::size_t column() const noexcept
    {
        return m_column;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_kind != YamlDiagnosticKind::None;
    }

    [[nodiscard]] constexpr std::string_view message() const noexcept
    {
        switch (m_kind) {
        case YamlDiagnosticKind::None:
            return {};
        case YamlDiagnosticKind::UnexpectedToken:
            return "unexpected token";
        case YamlDiagnosticKind::UnexpectedEnd:
            return "unexpected end of YAML input";
        case YamlDiagnosticKind::InvalidIndentation:
            return "invalid indentation";
        case YamlDiagnosticKind::InvalidAnchor:
            return "invalid anchor";
        case YamlDiagnosticKind::InvalidAlias:
            return "invalid alias";
        case YamlDiagnosticKind::InvalidTag:
            return "invalid tag";
        case YamlDiagnosticKind::InvalidScalar:
            return "invalid scalar";
        case YamlDiagnosticKind::InvalidFlowCollection:
            return "invalid flow collection";
        case YamlDiagnosticKind::InvalidBlockScalar:
            return "invalid block scalar";
        case YamlDiagnosticKind::InvalidDirective:
            return "invalid directive";
        case YamlDiagnosticKind::InvalidDocument:
            return "invalid document structure";
        case YamlDiagnosticKind::DestinationRejected:
            return "destination rejected YAML value";
        }

        return {};
    }

    void set(YamlDiagnosticKind kind, YamlSourceRange range, std::string_view source) noexcept
    {
        m_kind  = kind;
        m_range = range;
        locate(source, range.offset, m_line, m_column);
    }

    void clear() noexcept
    {
        m_kind   = YamlDiagnosticKind::None;
        m_range  = {};
        m_line   = 0;
        m_column = 0;
    }

private:
    static constexpr void locate(std::string_view source, std::size_t offset, std::size_t &line, std::size_t &column) noexcept
    {
        if (offset > source.size()) {
            line = 0;
            column = 0;
            return;
        }

        line = 1;
        column = 1;
        std::size_t i = 0;
        while (i < offset) {
            if (source[i] == '\r') {
                if (i + 1 < offset && source[i + 1] == '\n')
                    ++i;

                ++line;
                column = 1;
            } else if (source[i] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }

            ++i;
        }
    }

    YamlDiagnosticKind  m_kind{YamlDiagnosticKind::None};
    YamlSourceRange     m_range{};
    std::size_t         m_line{};
    std::size_t         m_column{};
};

} // namespace job::yaml