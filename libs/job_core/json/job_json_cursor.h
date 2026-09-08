#pragma once

#include <cstddef>
#include <string_view>

#include "job_json_contracts.h"
#include "job_json_source_range.h"

namespace job::json {

class JsonCursor
{
public:
    constexpr explicit JsonCursor(std::string_view source) noexcept :
        m_source(source)
    {
    }

    constexpr ~JsonCursor() = default;

    JsonCursor(const JsonCursor &) = delete;
    JsonCursor &operator=(const JsonCursor &) = delete;
    constexpr JsonCursor(JsonCursor &&) noexcept = default;
    constexpr JsonCursor &operator=(JsonCursor &&) noexcept = default;

    [[nodiscard]] constexpr std::string_view source() const noexcept
    {
        return m_source;
    }

    [[nodiscard]] constexpr std::size_t offset() const noexcept
    {
        return m_offset;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return m_source.size();
    }

    [[nodiscard]] constexpr std::size_t remaining() const noexcept
    {
        return m_source.size() - m_offset;
    }

    [[nodiscard]] constexpr bool atEnd() const noexcept
    {
        return m_offset == m_source.size();
    }

    [[nodiscard]] constexpr bool has(std::size_t count = 1) const noexcept
    {
        return count <= remaining();
    }

    [[nodiscard]] constexpr char peek(std::size_t lookahead = 0) const noexcept pre(lookahead < remaining())
    {
        return m_source[m_offset + lookahead];
    }

    // TODO(job_json): Restore pre(!atEnd()) once GCC contracts/constexpr
    // handling no longer rejects this valid runtime contract path.
    [[nodiscard]] constexpr char current() const noexcept
        // pre(!atEnd())
    {
        return m_source[m_offset];
    }

    constexpr void advance(std::size_t count = 1) noexcept pre(count <= remaining())
    {
        m_offset += count;
    }

    [[nodiscard]] constexpr bool consume(char expected) noexcept
    {
        if (atEnd() || current() != expected)
            return false;

        ++m_offset;
        return true;
    }

    constexpr void reset() noexcept
    {
        m_offset = 0;
    }

    constexpr void seek(std::size_t offset) noexcept
        pre(validSourceOffset(m_source, offset))
    {
        m_offset = offset;
    }

    [[nodiscard]] constexpr JsonSourceRange rangeFrom(std::size_t begin) const noexcept pre(begin <= m_offset)
    {
        return JsonSourceRange{begin, m_offset};
    }

    [[nodiscard]] constexpr JsonSourceRange range(std::size_t begin, std::size_t end) const noexcept pre(validSourceRange(m_source, begin, end))
    {
        return JsonSourceRange{begin, end};
    }

private:
    std::string_view    m_source;
    std::size_t         m_offset{};
};

} // namespace job::json

