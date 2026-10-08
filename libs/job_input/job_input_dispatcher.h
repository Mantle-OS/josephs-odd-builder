#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "job_input_device_classification.h"

#include "job_input_event.h"
#include "job_input_event_concept.h"

#include "job_input_key_event.h"

#include "job_pointer_button_event.h"
#include "job_pointer_motion_event.h"
#include "job_pointer_scroll_event.h"

#include "job_gamepad_button_event.h"
#include "job_gamepad_axis_event.h"

#include "job_touch_event.h"

#include "job_switch_event.h"
#include "job_dial_event.h"
#include "job_tablet_tool_event.h"

namespace job::input {
// proper multi-slot multitouch                          later
// gamepad button range audit                            soon
// ABS metadata / normalization                          later
// proper SYN_DROPPED resync from device                 later
class JobInputDispatcher
{
public:
    using Ptr  = std::shared_ptr<JobInputDispatcher>;
    using WPtr = std::weak_ptr<JobInputDispatcher>;
    using UPtr = std::unique_ptr<JobInputDispatcher>;

    using EventCallback         = std::function<void(const JobInputEvent &)>;
    using KeyCallback           = std::function<void(const JobKeyEvent &)>;

    using PointerButtonCallback = std::function<void(const JobPointerButtonEvent &)>;
    using MotionCallback        = std::function<void(const JobPointerMotionEvent &)>;
    using ScrollCallback        = std::function<void(const JobPointerScrollEvent &)>;

    using GamepadButtonCallback = std::function<void(const JobGamepadButtonEvent &)>;
    using GamepadAxisCallback   = std::function<void(const JobGamepadAxisEvent &)>;

    using TouchCallback         = std::function<void(const JobTouchEvent &)>;

    using SwitchCallback        = std::function<void(const JobSwitchEvent &)>;
    using DialCallback          = std::function<void(const JobDialEvent &)>;
    using TabletToolCallback    = std::function<void(const JobTabletToolEvent &)>;

    JobInputDispatcher() = default;

    explicit constexpr JobInputDispatcher(JobInputDeviceClassification classification) noexcept :
        m_classification(classification)
    {
    }

    ~JobInputDispatcher() = default;

    JobInputDispatcher(const JobInputDispatcher &) = delete;
    JobInputDispatcher &operator=(const JobInputDispatcher &) = delete;

    JobInputDispatcher(JobInputDispatcher &&) noexcept = default;
    JobInputDispatcher &operator=(JobInputDispatcher &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobInputDispatcher>();
    }

    [[nodiscard]] static Ptr createShared(JobInputDeviceClassification classification)
    {
        return std::make_shared<JobInputDispatcher>(classification);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobInputDispatcher>();
    }

    [[nodiscard]] static UPtr createUniq(JobInputDeviceClassification classification)
    {
        return std::make_unique<JobInputDispatcher>(classification);
    }

    void dispatch(const JobInputEvent &event);

    template<JobInputConcept EV>
    void dispatch(const EV &event)
    {
        dispatch(JobInputEvent(event.time(), event.type(), event.code(), event.value()));
    }

    void setClassification(JobInputDeviceClassification classification) noexcept;

    [[nodiscard]] constexpr const JobInputDeviceClassification &classification() const noexcept
    {
        return m_classification;
    }

    void setEventCallback(EventCallback callback);
    void setKeyCallback(KeyCallback callback);

    void setPointerButtonCallback(PointerButtonCallback callback);
    void setMotionCallback(MotionCallback callback);
    void setScrollCallback(ScrollCallback callback);

    void setGamepadButtonCallback(GamepadButtonCallback callback);
    void setGamepadAxisCallback(GamepadAxisCallback callback);

    void setTouchCallback(TouchCallback callback);

    void setSwitchCallback(SwitchCallback callback);
    void setDialCallback(DialCallback callback);
    void setTabletToolCallback(TabletToolCallback callback);

    void reset() noexcept;

private:
    void dispatchSync(const JobInputEvent &event);
    void dispatchKey(const JobInputEvent &event);
    void dispatchRelative(const JobInputEvent &event);
    void dispatchAbsolute(const JobInputEvent &event);
    void dispatchSwitch(const JobInputEvent &event);

    void flush(const JobInputEvent &syncEvent);

    void flushMotion(const JobInputEvent &syncEvent);
    void flushScroll(const JobInputEvent &syncEvent);
    void flushTouch(const JobInputEvent &syncEvent);
    void flushTabletTool(const JobInputEvent &syncEvent);

    void discardPending() noexcept;

    JobInputDeviceClassification m_classification;

    EventCallback         m_eventCallback;
    KeyCallback           m_keyCallback;

    PointerButtonCallback m_pointerButtonCallback;
    MotionCallback        m_motionCallback;
    ScrollCallback        m_scrollCallback;

    GamepadButtonCallback m_gamepadButtonCallback;
    GamepadAxisCallback   m_gamepadAxisCallback;

    TouchCallback         m_touchCallback;

    SwitchCallback        m_switchCallback;
    DialCallback          m_dialCallback;
    TabletToolCallback    m_tabletToolCallback;

    std::int32_t m_relativeX{0};
    std::int32_t m_relativeY{0};

    double m_pointerX{0.0};
    double m_pointerY{0.0};

    std::int32_t m_scrollHorizontal{0};
    std::int32_t m_scrollVertical{0};

    std::int32_t m_touchX{0};
    std::int32_t m_touchY{0};
    std::int32_t m_touchSlot{-1};
    std::int32_t m_touchTrackingId{-1};

    JobTouchState m_touchState{JobTouchState::Unknown};

    std::int32_t m_tabletX{0};
    std::int32_t m_tabletY{0};
    std::int32_t m_tabletPressure{0};

    JobInputCode       m_tabletToolCode{};
    JobTabletToolState m_tabletToolState{JobTabletToolState::Unknown};

    bool m_motionPending{false};
    bool m_scrollPending{false};
    bool m_touchPending{false};
    bool m_tabletToolPending{false};
};

} // namespace job::input