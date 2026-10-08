#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputSwitch
{
public:
    // #define SW_MAX 0x11
    inline static constexpr JobInputCode kSwitchMax = SW_MAX;

    // #define SW_CNT (SW_MAX + 1)
    inline static constexpr std::size_t kSwitchCount = SW_CNT;

    constexpr JobInputSwitch() noexcept = delete;
    ~JobInputSwitch() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kSwitchMax)
            return false;

        return lid(code)                ||
               tabletMode(code)         ||
               headphoneInsert(code)    ||
               rfkillAll(code)          ||
               microphoneInsert(code)   ||
               dock(code)               ||
               lineoutInsert(code)      ||
               jackPhysicalInsert(code) ||
               videooutInsert(code)     ||
               cameraLensCover(code)    ||
               keypadSlide(code)        ||
               frontProximity(code)     ||
               rotateLock(code)         ||
               lineinInsert(code)       ||
               muteDevice(code)         ||
               penInserted(code)        ||
               machineCover(code)       ||
               usbInsert(code);
    }

    [[nodiscard]] static constexpr bool lid(JobInputCode code) noexcept
    {
        // #define SW_LID 0x00 /* set = lid shut */
        return code == SW_LID;
    }

    [[nodiscard]] static constexpr bool tabletMode(JobInputCode code) noexcept
    {
        // #define SW_TABLET_MODE 0x01 /* set = tablet mode */
        return code == SW_TABLET_MODE;
    }

    [[nodiscard]] static constexpr bool headphoneInsert(JobInputCode code) noexcept
    {
        // #define SW_HEADPHONE_INSERT 0x02 /* set = inserted */
        return code == SW_HEADPHONE_INSERT;
    }

    [[nodiscard]] static constexpr bool rfkillAll(JobInputCode code) noexcept
    {
        // #define SW_RFKILL_ALL 0x03
        // #define SW_RADIO SW_RFKILL_ALL /* deprecated */
        return code == SW_RFKILL_ALL;
    }

    [[nodiscard]] static constexpr bool microphoneInsert(JobInputCode code) noexcept
    {
        // #define SW_MICROPHONE_INSERT 0x04 /* set = inserted */
        return code == SW_MICROPHONE_INSERT;
    }

    [[nodiscard]] static constexpr bool dock(JobInputCode code) noexcept
    {
        // #define SW_DOCK 0x05 /* set = plugged into dock */
        return code == SW_DOCK;
    }

    [[nodiscard]] static constexpr bool lineoutInsert(JobInputCode code) noexcept
    {
        // #define SW_LINEOUT_INSERT 0x06 /* set = inserted */
        return code == SW_LINEOUT_INSERT;
    }

    [[nodiscard]] static constexpr bool jackPhysicalInsert(JobInputCode code) noexcept
    {
        // #define SW_JACK_PHYSICAL_INSERT 0x07 /* set = mechanical switch set */
        return code == SW_JACK_PHYSICAL_INSERT;
    }

    [[nodiscard]] static constexpr bool videooutInsert(JobInputCode code) noexcept
    {
        // #define SW_VIDEOOUT_INSERT 0x08 /* set = inserted */
        return code == SW_VIDEOOUT_INSERT;
    }

    [[nodiscard]] static constexpr bool cameraLensCover(JobInputCode code) noexcept
    {
        // #define SW_CAMERA_LENS_COVER 0x09 /* set = lens covered */
        return code == SW_CAMERA_LENS_COVER;
    }

    [[nodiscard]] static constexpr bool keypadSlide(JobInputCode code) noexcept
    {
        // #define SW_KEYPAD_SLIDE 0x0a /* set = keypad slide out */
        return code == SW_KEYPAD_SLIDE;
    }

    [[nodiscard]] static constexpr bool frontProximity(JobInputCode code) noexcept
    {
        // #define SW_FRONT_PROXIMITY 0x0b
        return code == SW_FRONT_PROXIMITY;
    }

    [[nodiscard]] static constexpr bool rotateLock(JobInputCode code) noexcept
    {
        // #define SW_ROTATE_LOCK 0x0c /* set = rotate locked/disabled */
        return code == SW_ROTATE_LOCK;
    }

    [[nodiscard]] static constexpr bool lineinInsert(JobInputCode code) noexcept
    {
        // #define SW_LINEIN_INSERT 0x0d /* set = inserted */
        return code == SW_LINEIN_INSERT;
    }

    [[nodiscard]] static constexpr bool muteDevice(JobInputCode code) noexcept
    {
        // #define SW_MUTE_DEVICE 0x0e /* set = device disabled */
        return code == SW_MUTE_DEVICE;
    }

    [[nodiscard]] static constexpr bool penInserted(JobInputCode code) noexcept
    {
        // #define SW_PEN_INSERTED 0x0f /* set = pen inserted */
        return code == SW_PEN_INSERTED;
    }

    [[nodiscard]] static constexpr bool machineCover(JobInputCode code) noexcept
    {
        // #define SW_MACHINE_COVER 0x10 /* set = cover closed */
        return code == SW_MACHINE_COVER;
    }

    [[nodiscard]] static constexpr bool usbInsert(JobInputCode code) noexcept
    {
        // #define SW_USB_INSERT 0x11 /* set = USB audio device connected */
        return code == SW_USB_INSERT;
    }
};

} // namespace job::input