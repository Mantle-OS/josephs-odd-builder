#pragma once

#include <bitset>
#include <utility>

#include <linux/input-event-codes.h>
#include <linux/input.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputCapability
{
public:
    // We follow the order from input-event-codes.h. This way updating is not
    // a nightmare later on because the kernel never changes. :P
    using DeviceProperties = std::bitset<INPUT_PROP_CNT>;
    using EventTypes       = std::bitset<EV_CNT>;
    using Keys             = std::bitset<KEY_CNT>;
    using RelativeAxes     = std::bitset<REL_CNT>;
    using AbsoluteAxes     = std::bitset<ABS_CNT>;
    using Switches         = std::bitset<SW_CNT>;
    using MiscEvents       = std::bitset<MSC_CNT>;
    using Leds             = std::bitset<LED_CNT>;
    using Sounds           = std::bitset<SND_CNT>;
    using ForceFeedback    = std::bitset<FF_CNT>;

    constexpr JobInputCapability() noexcept = default;
    constexpr JobInputCapability(DeviceProperties deviceProperties,
                                 EventTypes eventTypes,
                                 Keys keys,
                                 RelativeAxes relativeAxes,
                                 AbsoluteAxes absoluteAxes,
                                 Switches switches,
                                 MiscEvents miscEvents,
                                 Leds leds,
                                 Sounds sounds,
                                 ForceFeedback forceFeedback) noexcept :
        m_deviceProperties(std::move(deviceProperties)),
        m_eventTypes(std::move(eventTypes)),
        m_keys(std::move(keys)),
        m_relativeAxes(std::move(relativeAxes)),
        m_absoluteAxes(std::move(absoluteAxes)),
        m_switches(std::move(switches)),
        m_miscEvents(std::move(miscEvents)),
        m_leds(std::move(leds)),
        m_sounds(std::move(sounds)),
        m_forceFeedback(std::move(forceFeedback))
    {
    }

    constexpr ~JobInputCapability() = default;

    constexpr JobInputCapability(const JobInputCapability &) noexcept = default;
    constexpr JobInputCapability &operator=(const JobInputCapability &) noexcept = default;

    constexpr JobInputCapability(JobInputCapability &&) noexcept = default;
    constexpr JobInputCapability &operator=(JobInputCapability &&) noexcept = default;

    [[nodiscard]] constexpr bool deviceProperty(JobInputCode code) const noexcept
    {
        return code < INPUT_PROP_CNT && m_deviceProperties.test(code);
    }

    [[nodiscard]] constexpr bool eventType(JobInputType type) const noexcept
    {
        return type < EV_CNT && m_eventTypes.test(type);
    }

    [[nodiscard]] constexpr bool key(JobInputCode code) const noexcept
    {
        return code < KEY_CNT && m_keys.test(code);
    }

    [[nodiscard]] constexpr bool relativeAxis(JobInputCode code) const noexcept
    {
        return code < REL_CNT && m_relativeAxes.test(code);
    }

    [[nodiscard]] constexpr bool absoluteAxis(JobInputCode code) const noexcept
    {
        return code < ABS_CNT && m_absoluteAxes.test(code);
    }

    [[nodiscard]] constexpr bool sw(JobInputCode code) const noexcept
    {
        return code < SW_CNT && m_switches.test(code);
    }

    [[nodiscard]] constexpr bool misc(JobInputCode code) const noexcept
    {
        return code < MSC_CNT && m_miscEvents.test(code);
    }

    [[nodiscard]] constexpr bool led(JobInputCode code) const noexcept
    {
        return code < LED_CNT && m_leds.test(code);
    }

    [[nodiscard]] constexpr bool sound(JobInputCode code) const noexcept
    {
        return code < SND_CNT && m_sounds.test(code);
    }

    [[nodiscard]] constexpr bool forceFeedback(JobInputCode code) const noexcept
    {
        return code < FF_CNT && m_forceFeedback.test(code);
    }

    [[nodiscard]] constexpr const DeviceProperties &deviceProperties() const noexcept
    {
        return m_deviceProperties;
    }

    [[nodiscard]] constexpr const EventTypes &eventTypes() const noexcept
    {
        return m_eventTypes;
    }

    [[nodiscard]] constexpr const Keys &keys() const noexcept
    {
        return m_keys;
    }

    [[nodiscard]] constexpr const RelativeAxes &relativeAxes() const noexcept
    {
        return m_relativeAxes;
    }

    [[nodiscard]] constexpr const AbsoluteAxes &absoluteAxes() const noexcept
    {
        return m_absoluteAxes;
    }

    [[nodiscard]] constexpr const Switches &switches() const noexcept
    {
        return m_switches;
    }

    [[nodiscard]] constexpr const MiscEvents &miscEvents() const noexcept
    {
        return m_miscEvents;
    }

    [[nodiscard]] constexpr const Leds &leds() const noexcept
    {
        return m_leds;
    }

    [[nodiscard]] constexpr const Sounds &sounds() const noexcept
    {
        return m_sounds;
    }

    [[nodiscard]] constexpr const ForceFeedback &forceFeedback() const noexcept
    {
        return m_forceFeedback;
    }

private:
    DeviceProperties m_deviceProperties{};
    EventTypes       m_eventTypes{};
    Keys             m_keys{};
    RelativeAxes     m_relativeAxes{};
    AbsoluteAxes     m_absoluteAxes{};
    Switches         m_switches{};
    MiscEvents       m_miscEvents{};
    Leds             m_leds{};
    Sounds           m_sounds{};
    ForceFeedback    m_forceFeedback{};
};

} // namespace job::input