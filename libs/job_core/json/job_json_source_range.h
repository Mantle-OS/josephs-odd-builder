#pragma once

#include <cstddef>
#include <string_view>

#include "job_json_contracts.h"

namespace job::json {

class JsonSourceRange
{
public:
    constexpr JsonSourceRange() noexcept = default;
    constexpr JsonSourceRange(std::size_t begin, std::size_t end) noexcept :
        m_begin(begin),
        m_end(end)
    {
    }

    constexpr ~JsonSourceRange() = default;

    constexpr JsonSourceRange(const JsonSourceRange &) noexcept = default;
    constexpr JsonSourceRange &operator=(const JsonSourceRange &) noexcept = default;
    constexpr JsonSourceRange(JsonSourceRange &&) noexcept = default;
    constexpr JsonSourceRange &operator=(JsonSourceRange &&) noexcept = default;

    [[nodiscard]] constexpr std::size_t begin() const noexcept
    {
        return m_begin;
    }

    [[nodiscard]] constexpr std::size_t end() const noexcept
    {
        return m_end;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return m_end - m_begin;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_begin == m_end;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_begin <= m_end;
    }

    [[nodiscard]] constexpr bool validFor(std::string_view source) const noexcept
    {
        return validSourceRange(source, m_begin, m_end);
    }

    [[nodiscard]] constexpr std::string_view view(std::string_view source) const noexcept pre(validFor(source))
    {
        return source.substr(m_begin, size());
    }

private:
    std::size_t m_begin{};
    std::size_t m_end{};
};

} // namespace job::json

