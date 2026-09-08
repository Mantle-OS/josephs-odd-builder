#pragma once

#include <cstddef>
#include <string_view>

namespace job::yaml {

class YamlCursor
{
public:
    explicit constexpr YamlCursor(std::string_view source) noexcept :
        m_source(source)
    {
    }

    constexpr ~YamlCursor() = default;
    constexpr YamlCursor(const YamlCursor &) noexcept = default;
    constexpr YamlCursor &operator=(const YamlCursor &) noexcept = default;
    constexpr YamlCursor(YamlCursor &&) noexcept = default;
    constexpr YamlCursor &operator=(YamlCursor &&) noexcept = default;

    constexpr bool empty() const noexcept
    {
        return m_offset >= m_source.size();
    }

    constexpr explicit operator bool() const noexcept
    {
        return !empty();
    }

    constexpr char current() const noexcept
    {
        return empty() ? '\0' : m_source[m_offset];
    }

    constexpr char peek(std::size_t distance = 1) const noexcept
    {
        const std::size_t pos = m_offset + distance;
        return pos < m_source.size() ? m_source[pos] : '\0';
    }

    constexpr void advance(std::size_t count = 1) noexcept
    {
        const std::size_t remaining = m_source.size() - m_offset;
        m_offset += count < remaining ? count : remaining;
    }

    constexpr bool consume(char expected) noexcept
    {
        if (current() != expected)
            return false;

        advance();
        return true;
    }

    constexpr bool consume(std::string_view expected) noexcept
    {
        if (!startsWith(expected))
            return false;

        advance(expected.size());
        return true;
    }

    constexpr bool startsWith(std::string_view value) const noexcept
    {
        return m_source.substr(m_offset).starts_with(value);
    }

    constexpr std::string_view remaining() const noexcept
    {
        return m_source.substr(m_offset);
    }

    constexpr std::string_view source() const noexcept
    {
        return m_source;
    }

    constexpr std::size_t offset() const noexcept
    {
        return m_offset;
    }

    constexpr std::size_t size() const noexcept
    {
        return m_source.size();
    }

    constexpr void reset() noexcept
    {
        m_offset = 0;
    }

    constexpr void reset(std::size_t offset) noexcept
    {
        m_offset = offset <= m_source.size() ? offset : m_source.size();
    }

private:
    std::string_view    m_source;
    std::size_t         m_offset{};
};

} // namespace job::yaml