#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputDeviceProperties {
public:

    // #define INPUT_PROP_MAX 0x1f
    inline static constexpr JobInputCode kInputPropertyMax = INPUT_PROP_MAX;

    // #define INPUT_PROP_CNT (INPUT_PROP_MAX + 1)
    inline static constexpr std::size_t kInputPropertyCount = INPUT_PROP_CNT;
    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if(code > kInputPropertyMax)
            return false;

        return pointer(code) ||
               direct(code) ||
               buttonPad(code) ||
               semiMt(code) ||
               topButtonPad(code) ||
               pointingStick(code) ||
               accelerometer(code) ||
               pressurePad(code);
    }

    constexpr JobInputDeviceProperties() noexcept = delete;
    ~JobInputDeviceProperties() = delete;


    [[nodiscard]] static constexpr bool pointer(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_POINTER		0x00	/* needs a pointer */
        return property == INPUT_PROP_POINTER;
    }

    [[nodiscard]] static constexpr bool direct(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_DIRECT		0x01	/* direct input devices */
        return property == INPUT_PROP_DIRECT;
    }

    [[nodiscard]] static constexpr bool buttonPad(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_BUTTONPAD		0x02	/* has button(s) under pad */
        return property == INPUT_PROP_BUTTONPAD;
    }

    [[nodiscard]] static constexpr bool semiMt(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_SEMI_MT		0x03	/* touch rectangle only */
        return property == INPUT_PROP_SEMI_MT;
    }

    [[nodiscard]] static constexpr bool topButtonPad(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_TOPBUTTONPAD		0x04	/* softbuttons at top of pad */
        return property == INPUT_PROP_TOPBUTTONPAD;
    }

    [[nodiscard]] static constexpr bool pointingStick(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_POINTING_STICK	0x05	/* is a pointing stick */
        return property == INPUT_PROP_POINTING_STICK;
    }

    [[nodiscard]] static constexpr bool accelerometer(JobInputCode property) noexcept
    {
        // #define INPUT_PROP_ACCELEROMETER	0x06	/* has accelerometer */
        return property == INPUT_PROP_ACCELEROMETER;
    }

    [[nodiscard]] static constexpr bool pressurePad(JobInputCode property) noexcept {
        // #define INPUT_PROP_PRESSUREPAD		0x07	/* pressure triggers clicks */
        return property == INPUT_PROP_PRESSUREPAD;
    }
};

} // namespace job::input
