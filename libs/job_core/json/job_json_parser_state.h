#pragma once

#include <cstdint>

namespace job::json {

enum class JsonParserState : std::uint8_t
{
    ExpectValue = 0,

    ExpectObjectKeyOrEnd,
    ExpectNameSeparator,
    ExpectObjectValue,
    ExpectObjectSeparatorOrEnd,

    ExpectArrayValueOrEnd,
    ExpectArraySeparatorOrEnd,

    Done,
    Error,
};

enum class JsonParserFrameType : std::uint8_t
{
    Object = 0,
    Array,
};

class JsonParserFrame
{
public:
    constexpr JsonParserFrame() noexcept = default;

    constexpr JsonParserFrame(
        JsonParserFrameType type,
        JsonParserState state) noexcept :
        m_type(type),
        m_state(state)
    {
    }

    constexpr ~JsonParserFrame() = default;

    constexpr JsonParserFrame(const JsonParserFrame &) noexcept = default;
    constexpr JsonParserFrame &operator=(const JsonParserFrame &) noexcept = default;
    constexpr JsonParserFrame(JsonParserFrame &&) noexcept = default;
    constexpr JsonParserFrame &operator=(JsonParserFrame &&) noexcept = default;

    [[nodiscard]] constexpr JsonParserFrameType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr JsonParserState state() const noexcept
    {
        return m_state;
    }

    constexpr void setState(JsonParserState state) noexcept
    {
        m_state = state;
    }

private:
    JsonParserFrameType m_type{JsonParserFrameType::Object};
    JsonParserState     m_state{JsonParserState::ExpectValue};
};

} // namespace job::json
