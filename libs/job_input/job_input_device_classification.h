#pragma once

#include <cstdint>
#include <type_traits>

#include "job_input_capability.h"

namespace job::input {

enum class JobInputDeviceType : std::uint32_t
{
    None          = 0,

    Keyboard      = 1u << 0,

    Pointer       = 1u << 1,
    Trackball     = 1u << 2,
    Touchpad      = 1u << 3,

    Touch         = 1u << 4,

    TabletTool    = 1u << 5,
    TabletPad     = 1u << 6,

    Gamepad       = 1u << 7,
    Joystick      = 1u << 8,

    Switch        = 1u << 9,
    Dial          = 1u << 10,
    Accelerometer = 1u << 11,

    All           = (1u << 12) - 1u
};

[[nodiscard]] constexpr JobInputDeviceType operator|(JobInputDeviceType a, JobInputDeviceType b) noexcept
{
    using Type = std::underlying_type_t<JobInputDeviceType>;

    return static_cast<JobInputDeviceType>(static_cast<Type>(a) | static_cast<Type>(b));
}

[[nodiscard]] constexpr JobInputDeviceType operator&(JobInputDeviceType a, JobInputDeviceType b) noexcept
{
    using Type = std::underlying_type_t<JobInputDeviceType>;

    return static_cast<JobInputDeviceType>(static_cast<Type>(a) & static_cast<Type>(b));
}

[[nodiscard]] constexpr JobInputDeviceType operator~(JobInputDeviceType value) noexcept
{
    using Type = std::underlying_type_t<JobInputDeviceType>;

    return static_cast<JobInputDeviceType>((~static_cast<Type>(value)) & static_cast<Type>(JobInputDeviceType::All));
}

constexpr JobInputDeviceType &operator|=(JobInputDeviceType &a, JobInputDeviceType b) noexcept
{
    a = a | b;
    return a;
}

constexpr JobInputDeviceType &operator&=(JobInputDeviceType &a, JobInputDeviceType b) noexcept
{
    a = a & b;
    return a;
}

class JobInputDeviceClassification
{
public:
    using Type = JobInputDeviceType;

    constexpr JobInputDeviceClassification() noexcept = default;

    explicit constexpr JobInputDeviceClassification(Type types) noexcept :
        m_types(types)
    {
    }

    explicit JobInputDeviceClassification(const JobInputCapability &capability) noexcept;

    constexpr ~JobInputDeviceClassification() = default;

    constexpr JobInputDeviceClassification(const JobInputDeviceClassification &) noexcept = default;
    constexpr JobInputDeviceClassification &operator=(const JobInputDeviceClassification &) noexcept = default;

    constexpr JobInputDeviceClassification(JobInputDeviceClassification &&) noexcept = default;
    constexpr JobInputDeviceClassification &operator=(JobInputDeviceClassification &&) noexcept = default;

    [[nodiscard]] constexpr Type types() const noexcept
    {
        return m_types;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_types == Type::None;
    }

    [[nodiscard]] constexpr bool has(Type type) const noexcept
    {
        if (type == Type::None)
            return false;

        return (m_types & type) == type;
    }

    [[nodiscard]] constexpr bool keyboard() const noexcept
    {
        return has(Type::Keyboard);
    }

    [[nodiscard]] constexpr bool pointer() const noexcept
    {
        return has(Type::Pointer);
    }

    [[nodiscard]] constexpr bool trackball() const noexcept
    {
        return has(Type::Trackball);
    }

    [[nodiscard]] constexpr bool touchpad() const noexcept
    {
        return has(Type::Touchpad);
    }

    [[nodiscard]] constexpr bool touch() const noexcept
    {
        return has(Type::Touch);
    }

    [[nodiscard]] constexpr bool tabletTool() const noexcept
    {
        return has(Type::TabletTool);
    }

    [[nodiscard]] constexpr bool tabletPad() const noexcept
    {
        return has(Type::TabletPad);
    }

    [[nodiscard]] constexpr bool gamepad() const noexcept
    {
        return has(Type::Gamepad);
    }

    [[nodiscard]] constexpr bool joystick() const noexcept
    {
        return has(Type::Joystick);
    }

    [[nodiscard]] constexpr bool sw() const noexcept
    {
        return has(Type::Switch);
    }

    [[nodiscard]] constexpr bool dial() const noexcept
    {
        return has(Type::Dial);
    }

    [[nodiscard]] constexpr bool accelerometer() const noexcept
    {
        return has(Type::Accelerometer);
    }

private:
    [[nodiscard]] static Type classify(const JobInputCapability &capability) noexcept;

    [[nodiscard]] static bool isKeyboard(const JobInputCapability &capability) noexcept;

    [[nodiscard]] static bool isPointer(const JobInputCapability &capability) noexcept;
    [[nodiscard]] static bool isTrackball(const JobInputCapability &capability) noexcept;
    [[nodiscard]] static bool isTouchpad(const JobInputCapability &capability) noexcept;

    [[nodiscard]] static bool isTouch(const JobInputCapability &capability) noexcept;

    [[nodiscard]] static bool isTabletTool(const JobInputCapability &capability) noexcept;
    [[nodiscard]] static bool isTabletPad(const JobInputCapability &capability) noexcept;

    [[nodiscard]] static bool isGamepad(const JobInputCapability &capability) noexcept;
    [[nodiscard]] static bool isJoystick(const JobInputCapability &capability) noexcept;

    [[nodiscard]] static bool isSwitch(const JobInputCapability &capability) noexcept;
    [[nodiscard]] static bool isDial(const JobInputCapability &capability) noexcept;
    [[nodiscard]] static bool isAccelerometer(const JobInputCapability &capability) noexcept;

    Type m_types{Type::None};
};

} // namespace job::input