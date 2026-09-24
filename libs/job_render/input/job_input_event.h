#pragma once

#include <cstdint>
#include <variant>

namespace job::input {

using InputDeviceId  = std::uint64_t;
using InputTimestamp = std::uint64_t;

enum class InputEventType : std::uint8_t
{
    Unknown,

    DeviceAdded,
    DeviceRemoved,

    KeyPress,
    KeyRelease,

    PointerMotion,
    PointerButtonPress,
    PointerButtonRelease,
    PointerScroll,

    TouchDown,
    TouchMotion,
    TouchUp,
    TouchCancel
};

enum class InputDeviceType : std::uint8_t
{
    Unknown,

    Keyboard,
    Pointer,
    Touch,
    Tablet
};

struct InputEventHeader
{
    InputEventType  type{InputEventType::Unknown};
    InputDeviceType deviceType{InputDeviceType::Unknown};
    InputDeviceId   deviceId{};
    InputTimestamp  timestamp{};
};

struct KeyEvent
{
    InputEventHeader event{};
    std::uint32_t    key{};
};

struct PointerMotionEvent
{
    InputEventHeader event{};
    double           x{};
    double           y{};
    double           deltaX{};
    double           deltaY{};
};

struct PointerButtonEvent
{
    InputEventHeader event{};
    std::uint32_t    button{};
};

struct PointerScrollEvent
{
    InputEventHeader event{};
    double           horizontal{};
    double           vertical{};
};

struct TouchEvent
{
    InputEventHeader event{};
    std::int32_t     slot{-1};
    double           x{};
    double           y{};
};

struct DeviceEvent
{
    InputEventHeader event{};
};

using InputEvent = std::variant<
    DeviceEvent,
    KeyEvent,
    PointerMotionEvent,
    PointerButtonEvent,
    PointerScrollEvent,
    TouchEvent
    >;

} // namespace job::input