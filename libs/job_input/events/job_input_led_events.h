#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputLedEvent
{
public:
    inline static constexpr JobInputCode kLedEventMax = LED_MAX; // #define LED_MAX 0x0f
    inline static constexpr std::size_t kLedEventCount = LED_CNT;

    constexpr JobInputLedEvent() noexcept = delete;
    ~JobInputLedEvent() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kLedEventMax)
            return false;

        return numLock(code)    ||
               capsLock(code)   ||
               scrollLock(code) ||
               compose(code)    ||
               kana(code)       ||
               sleep(code)      ||
               suspend(code)    ||
               mute(code)       ||
               misc(code)       ||
               mail(code)       ||
               charging(code);
    }

    [[nodiscard]] static constexpr bool numLock(JobInputCode code) noexcept
    {
        // #define LED_NUML 0x00
        return code == LED_NUML;
    }

    [[nodiscard]] static constexpr bool capsLock(JobInputCode code) noexcept
    {
        // #define LED_CAPSL 0x01
        return code == LED_CAPSL;
    }

    [[nodiscard]] static constexpr bool scrollLock(JobInputCode code) noexcept
    {
        // #define LED_SCROLLL 0x02
        return code == LED_SCROLLL;
    }

    [[nodiscard]] static constexpr bool compose(JobInputCode code) noexcept
    {
        // #define LED_COMPOSE 0x03
        return code == LED_COMPOSE;
    }

    [[nodiscard]] static constexpr bool kana(JobInputCode code) noexcept
    {
        // #define LED_KANA 0x04
        return code == LED_KANA;
    }

    [[nodiscard]] static constexpr bool sleep(JobInputCode code) noexcept
    {
        // #define LED_SLEEP 0x05
        return code == LED_SLEEP;
    }

    [[nodiscard]] static constexpr bool suspend(JobInputCode code) noexcept
    {
        // #define LED_SUSPEND 0x06
        return code == LED_SUSPEND;
    }

    [[nodiscard]] static constexpr bool mute(JobInputCode code) noexcept
    {
        // #define LED_MUTE 0x07
        return code == LED_MUTE;
    }

    [[nodiscard]] static constexpr bool misc(JobInputCode code) noexcept
    {
        // #define LED_MISC 0x08
        return code == LED_MISC;
    }

    [[nodiscard]] static constexpr bool mail(JobInputCode code) noexcept
    {
        // #define LED_MAIL 0x09
        return code == LED_MAIL;
    }

    [[nodiscard]] static constexpr bool charging(JobInputCode code) noexcept
    {
        // #define LED_CHARGING 0x0a
        return code == LED_CHARGING;
    }
};

} // namespace job::input