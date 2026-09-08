#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "job_json_source_range.h"

    namespace job::json {

    enum class JsonDiagnosticCode : std::uint16_t
    {
        None = 0,

        UnexpectedEnd,
        UnexpectedToken,
        InvalidToken,
        InvalidString,
        InvalidEscape,
        InvalidUnicode,
        InvalidNumber,

        ExpectedValue,
        ExpectedObjectKey,
        ExpectedNameSeparator,
        ExpectedValueSeparator,
        ExpectedObjectEnd,
        ExpectedArrayEnd,

        UnknownMember,
        TypeMismatch,
        NumberOutOfRange,
        InvalidNull,
        ContainerError,

        MaxDepthExceeded,
    };

    class JsonDiagnostic
    {
    public:
        constexpr JsonDiagnostic() noexcept = default;

        constexpr JsonDiagnostic(JsonDiagnosticCode code, JsonSourceRange range, std::string_view comment = {}) noexcept :
            m_code(code),
            m_range(range),
            m_comment(comment)
        {
        }

        constexpr ~JsonDiagnostic() = default;

        constexpr JsonDiagnostic(const JsonDiagnostic &) noexcept = default;
        constexpr JsonDiagnostic &operator=(const JsonDiagnostic &) noexcept = default;
        constexpr JsonDiagnostic(JsonDiagnostic &&) noexcept = default;
        constexpr JsonDiagnostic &operator=(JsonDiagnostic &&) noexcept = default;

        [[nodiscard]] constexpr JsonDiagnosticCode code() const noexcept
        {
            return m_code;
        }

        [[nodiscard]] constexpr const JsonSourceRange &range() const noexcept
        {
            return m_range;
        }

        [[nodiscard]] constexpr std::string_view comment() const noexcept
        {
            return m_comment;
        }

        [[nodiscard]] constexpr bool hasError() const noexcept
        {
            return m_code != JsonDiagnosticCode::None;
        }

        [[nodiscard]] constexpr std::size_t offset() const noexcept
        {
            return m_range.begin();
        }

        [[nodiscard]] constexpr std::size_t line(std::string_view source) const noexcept
            pre(m_range.validFor(source))
        {
            std::size_t line = 1;

            for (std::size_t i = 0; i < m_range.begin(); ++i) {
                if (source[i] == '\n')
                    ++line;
            }

            return line;
        }

        [[nodiscard]] constexpr std::size_t column(std::string_view source) const noexcept
            pre(m_range.validFor(source))
        {
            std::size_t column = 1;

            for (std::size_t i = m_range.begin(); i > 0; --i) {
                if (source[i - 1] == '\n')
                    break;

                ++column;
            }

            return column;
        }

    private:
        JsonDiagnosticCode  m_code{JsonDiagnosticCode::None};
        JsonSourceRange     m_range{};
        std::string_view    m_comment{};
    };

} // namespace job::json
