#pragma once

#include <cstddef>
#include <cstdint>
#include <inplace_vector>
#include <memory>
#include <string_view>
#include <utility>

#include "job_yaml_cursor.h"
#include "job_yaml_indent_stack.h"

namespace job::yaml {

enum class YamlParserContext : std::uint8_t {
    Mapping,
    Sequence
};

struct YamlParserFrame
{
    YamlParserContext context;
    std::size_t indent;
};

class YamlParserState
{
public:
    using Ptr = std::shared_ptr<YamlParserState>;
    using WPtr = std::weak_ptr<YamlParserState>;
    using UPtr = std::unique_ptr<YamlParserState>;
    using Frames = std::inplace_vector<YamlParserFrame, YamlIndentStack::MaxDepth>;

    explicit constexpr YamlParserState(std::string_view source) noexcept :
        m_cursor(source)
    {
    }

    ~YamlParserState() = default;

    YamlParserState(const YamlParserState &) = delete;
    YamlParserState &operator=(const YamlParserState &) = delete;
    YamlParserState(YamlParserState &&) noexcept = default;
    YamlParserState &operator=(YamlParserState &&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<YamlParserState>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<YamlParserState>(std::forward<Args>(args)...);
    }

    [[nodiscard]] constexpr YamlCursor &cursor() noexcept
    {
        return m_cursor;
    }

    [[nodiscard]] constexpr const YamlCursor &cursor() const noexcept
    {
        return m_cursor;
    }

    [[nodiscard]] constexpr YamlIndentStack &indents() noexcept
    {
        return m_indents;
    }

    [[nodiscard]] constexpr const YamlIndentStack &indents() const noexcept
    {
        return m_indents;
    }

    [[nodiscard]] constexpr bool atRoot() const noexcept
    {
        return m_frames.empty();
    }

    [[nodiscard]] constexpr std::size_t depth() const noexcept
    {
        return m_frames.size();
    }

    [[nodiscard]] constexpr YamlParserContext context() const noexcept pre(!m_frames.empty())
    {
        return m_frames.back().context;
    }

    [[nodiscard]] constexpr std::size_t indent() const noexcept pre(!m_frames.empty())
    {
        return m_frames.back().indent;
    }

    [[nodiscard]] constexpr const YamlParserFrame &frame() const noexcept pre(!m_frames.empty())
    {
        return m_frames.back();
    }

    constexpr bool push(YamlParserContext context, std::size_t indent) noexcept
    {
        if (m_frames.size() == m_frames.capacity())
            return false;

        m_frames.push_back({
            .context = context,
            .indent = indent
        });
        return true;
    }

    constexpr void pop() noexcept pre(!m_frames.empty())
    {
        m_frames.pop_back();
    }

    constexpr void clearFrames() noexcept
    {
        m_frames.clear();
    }

    constexpr void mark() noexcept
    {
        m_mark = m_cursor.offset();
    }

    [[nodiscard]] constexpr std::size_t markOffset() const noexcept
    {
        return m_mark;
    }

    [[nodiscard]] constexpr std::string_view markedView() const noexcept pre(m_mark <= m_cursor.offset())
    {
        return m_cursor.source().substr(m_mark, m_cursor.offset() - m_mark);
    }

    constexpr void rewindToMark() noexcept
    {
        m_cursor.reset(m_mark);
    }

private:
    YamlCursor          m_cursor;
    YamlIndentStack     m_indents;
    Frames              m_frames;
    std::size_t         m_mark{};
};

} // namespace job::yaml