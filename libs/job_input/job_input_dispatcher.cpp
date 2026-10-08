#include "job_input_dispatcher.h"

#include <linux/input-event-codes.h>

namespace job::input {

void JobInputDispatcher::dispatch(const JobInputEvent &event)
{
    if (m_eventCallback)
        m_eventCallback(event);

    switch (event.type()) {
    case EV_SYN:
        dispatchSync(event);
        break;
    case EV_KEY:
        dispatchKey(event);
        break;
    case EV_REL:
        dispatchRelative(event);
        break;
    case EV_ABS:
        dispatchAbsolute(event);
        break;
    case EV_SW:
        dispatchSwitch(event);
        break;
    default:
        break;
    }
}

void JobInputDispatcher::setClassification(JobInputDeviceClassification classification) noexcept
{
    reset();
    m_classification = classification;
}

void JobInputDispatcher::setEventCallback(EventCallback callback)
{
    m_eventCallback = std::move(callback);
}

void JobInputDispatcher::setKeyCallback(KeyCallback callback)
{
    m_keyCallback = std::move(callback);
}

void JobInputDispatcher::setPointerButtonCallback(PointerButtonCallback callback)
{
    m_pointerButtonCallback = std::move(callback);
}

void JobInputDispatcher::setMotionCallback(MotionCallback callback)
{
    m_motionCallback = std::move(callback);
}

void JobInputDispatcher::setScrollCallback(ScrollCallback callback)
{
    m_scrollCallback = std::move(callback);
}

void JobInputDispatcher::setGamepadButtonCallback(GamepadButtonCallback callback)
{
    m_gamepadButtonCallback = std::move(callback);
}

void JobInputDispatcher::setGamepadAxisCallback(GamepadAxisCallback callback)
{
    m_gamepadAxisCallback = std::move(callback);
}

void JobInputDispatcher::setTouchCallback(TouchCallback callback)
{
    m_touchCallback = std::move(callback);
}

void JobInputDispatcher::setSwitchCallback(SwitchCallback callback)
{
    m_switchCallback = std::move(callback);
}

void JobInputDispatcher::setDialCallback(DialCallback callback)
{
    m_dialCallback = std::move(callback);
}

void JobInputDispatcher::setTabletToolCallback(TabletToolCallback callback)
{
    m_tabletToolCallback = std::move(callback);
}

void JobInputDispatcher::reset() noexcept
{
    m_relativeX = 0;
    m_relativeY = 0;

    m_pointerX = 0.0;
    m_pointerY = 0.0;

    m_scrollHorizontal = 0;
    m_scrollVertical = 0;

    m_touchX = 0;
    m_touchY = 0;
    m_touchSlot = -1;
    m_touchTrackingId = -1;
    m_touchState = JobTouchState::Unknown;

    m_tabletX = 0;
    m_tabletY = 0;
    m_tabletPressure = 0;
    m_tabletToolCode = 0;
    m_tabletToolState = JobTabletToolState::Unknown;

    m_motionPending = false;
    m_scrollPending = false;
    m_touchPending = false;
    m_tabletToolPending = false;
}

void JobInputDispatcher::discardPending() noexcept
{
    m_relativeX = 0;
    m_relativeY = 0;

    m_scrollHorizontal = 0;
    m_scrollVertical = 0;

    m_touchX = 0;
    m_touchY = 0;
    m_touchSlot = -1;
    m_touchTrackingId = -1;
    m_touchState = JobTouchState::Unknown;

    m_tabletX = 0;
    m_tabletY = 0;
    m_tabletPressure = 0;
    m_tabletToolCode = 0;
    m_tabletToolState = JobTabletToolState::Unknown;

    m_motionPending = false;
    m_scrollPending = false;
    m_touchPending = false;
    m_tabletToolPending = false;
}

void JobInputDispatcher::dispatchSync(const JobInputEvent &event)
{
    switch (event.code()) {
    case SYN_REPORT:
        flush(event);
        break;
    case SYN_DROPPED:
        discardPending();
        break;
    default:
        break;
    }
}

void JobInputDispatcher::dispatchKey(const JobInputEvent &event)
{
    const auto code  = event.code();
    const auto value = event.value();

    if (m_classification.touch() && code == BTN_TOUCH) {
        if (value == 0)
            m_touchState = JobTouchState::Ended;
        else if (value == 1)
            m_touchState = JobTouchState::Began;
        else
            m_touchState = JobTouchState::Unknown;

        m_touchPending = true;
        return;
    }

    if (m_classification.tabletTool()) {
        switch (code) {
        case BTN_TOOL_PEN:
        case BTN_TOOL_RUBBER:
        case BTN_TOOL_BRUSH:
        case BTN_TOOL_PENCIL:
        case BTN_TOOL_AIRBRUSH:
        case BTN_TOOL_MOUSE:
        case BTN_TOOL_LENS:
            if (value == 0) {
                m_tabletToolState = JobTabletToolState::OutOfProximity;
            } else if (value == 1) {
                m_tabletToolCode = code;
                m_tabletToolState = JobTabletToolState::InProximity;
            } else {
                m_tabletToolState = JobTabletToolState::Unknown;
            }

            m_tabletToolPending = true;
            return;

        default:
            break;
        }
    }

    if (m_classification.gamepad() || m_classification.joystick()) {
        if (code >= BTN_JOYSTICK && code <= BTN_THUMBR) {
            JobGamepadButtonState state = JobGamepadButtonState::Unknown;

            if (value == 0)
                state = JobGamepadButtonState::Released;
            else if (value == 1)
                state = JobGamepadButtonState::Pressed;

            if (m_gamepadButtonCallback)
                m_gamepadButtonCallback(JobGamepadButtonEvent(event.time(), event.type(), code, value, state));

            return;
        }
    }

    if (m_classification.pointer()) {
        if (code >= BTN_MOUSE && code < BTN_JOYSTICK) {
            JobPointerButtonState state = JobPointerButtonState::Unknown;

            if (value == 0)
                state = JobPointerButtonState::Released;
            else if (value == 1)
                state = JobPointerButtonState::Pressed;

            if (m_pointerButtonCallback)
                m_pointerButtonCallback(JobPointerButtonEvent(event.time(), event.type(), code, value, state));

            return;
        }
    }

    if (!m_classification.keyboard())
        return;

    JobKeyState state = JobKeyState::Unknown;

    if (value == 0)
        state = JobKeyState::Released;
    else if (value == 1)
        state = JobKeyState::Pressed;
    else if (value == 2)
        state = JobKeyState::Repeated;

    if (m_keyCallback)
        m_keyCallback(JobKeyEvent(event.time(), event.type(), code, value, state));
}

void JobInputDispatcher::dispatchRelative(const JobInputEvent &event)
{
    switch (event.code()) {
    case REL_X:
        if (m_classification.pointer()) {
            m_relativeX += event.value();
            m_motionPending = true;
        }
        break;

    case REL_Y:
        if (m_classification.pointer()) {
            m_relativeY += event.value();
            m_motionPending = true;
        }
        break;

    case REL_HWHEEL:
        if (m_classification.pointer()) {
            m_scrollHorizontal += event.value();
            m_scrollPending = true;
        }
        break;

    case REL_WHEEL:
        if (m_classification.pointer()) {
            m_scrollVertical += event.value();
            m_scrollPending = true;
        }
        break;

    case REL_DIAL:
        if (m_classification.dial() && m_dialCallback) {
            m_dialCallback(JobDialEvent(event.time(),
                                        event.type(),
                                        event.code(),
                                        event.value(),
                                        static_cast<double>(event.value())));
        }
        break;

    default:
        break;
    }
}

void JobInputDispatcher::dispatchAbsolute(const JobInputEvent &event)
{
    if (m_classification.touch()) {
        switch (event.code()) {
        case ABS_MT_SLOT:
            m_touchSlot = event.value();
            return;

        case ABS_MT_TRACKING_ID:
            if (event.value() < 0) {
                m_touchState = JobTouchState::Ended;
            } else {
                m_touchTrackingId = event.value();
                m_touchState = JobTouchState::Began;
            }

            m_touchPending = true;
            return;

        case ABS_MT_POSITION_X:
        case ABS_X:
            m_touchX = event.value();

            if (m_touchState != JobTouchState::Began &&
                m_touchState != JobTouchState::Ended) {
                m_touchState = JobTouchState::Updated;
            }

            m_touchPending = true;
            return;

        case ABS_MT_POSITION_Y:
        case ABS_Y:
            m_touchY = event.value();

            if (m_touchState != JobTouchState::Began &&
                m_touchState != JobTouchState::Ended) {
                m_touchState = JobTouchState::Updated;
            }

            m_touchPending = true;
            return;

        default:
            break;
        }
    }

    if (m_classification.tabletTool()) {
        switch (event.code()) {
        case ABS_X:
            m_tabletX = event.value();
            m_tabletToolPending = true;
            return;

        case ABS_Y:
            m_tabletY = event.value();
            m_tabletToolPending = true;
            return;

        case ABS_PRESSURE:
            m_tabletPressure = event.value();
            m_tabletToolPending = true;
            return;

        default:
            break;
        }
    }

    if (m_classification.gamepad() || m_classification.joystick()) {
        if (m_gamepadAxisCallback)
            m_gamepadAxisCallback(JobGamepadAxisEvent(event.time(), event.type(), event.code(), event.value()));
    }
}

void JobInputDispatcher::dispatchSwitch(const JobInputEvent &event)
{
    if (!m_classification.sw())
        return;

    JobSwitchState state = JobSwitchState::Unknown;

    if (event.value() == 0)
        state = JobSwitchState::Off;
    else if (event.value() == 1)
        state = JobSwitchState::On;

    if (m_switchCallback)
        m_switchCallback(JobSwitchEvent(event.time(), event.type(), event.code(), event.value(), state));
}

void JobInputDispatcher::flush(const JobInputEvent &syncEvent)
{
    flushMotion(syncEvent);
    flushScroll(syncEvent);
    flushTouch(syncEvent);
    flushTabletTool(syncEvent);
}

void JobInputDispatcher::flushMotion(const JobInputEvent &syncEvent)
{
    if (!m_motionPending)
        return;

    const double deltaX = static_cast<double>(m_relativeX);
    const double deltaY = static_cast<double>(m_relativeY);

    m_pointerX += deltaX;
    m_pointerY += deltaY;

    if (m_motionCallback) {
        m_motionCallback(JobPointerMotionEvent(syncEvent.time(),
                                               EV_REL,
                                               REL_X,
                                               m_relativeX,
                                               m_pointerX,
                                               m_pointerY,
                                               deltaX,
                                               deltaY));
    }

    m_relativeX = 0;
    m_relativeY = 0;
    m_motionPending = false;
}

void JobInputDispatcher::flushScroll(const JobInputEvent &syncEvent)
{
    if (!m_scrollPending)
        return;

    if (m_scrollCallback) {
        m_scrollCallback(JobPointerScrollEvent(syncEvent.time(),
                                               EV_REL,
                                               REL_WHEEL,
                                               m_scrollVertical,
                                               static_cast<double>(m_scrollHorizontal),
                                               static_cast<double>(m_scrollVertical)));
    }

    m_scrollHorizontal = 0;
    m_scrollVertical = 0;
    m_scrollPending = false;
}

void JobInputDispatcher::flushTouch(const JobInputEvent &syncEvent)
{
    if (!m_touchPending)
        return;

    if (m_touchCallback) {
        m_touchCallback(JobTouchEvent(syncEvent.time(),
                                      EV_ABS,
                                      ABS_MT_POSITION_X,
                                      m_touchX,
                                      static_cast<double>(m_touchX),
                                      static_cast<double>(m_touchY),
                                      m_touchSlot,
                                      m_touchTrackingId,
                                      m_touchState));
    }

    if (m_touchState == JobTouchState::Ended) {
        m_touchTrackingId = -1;
        m_touchState = JobTouchState::Unknown;
    } else if (m_touchState == JobTouchState::Began) {
        m_touchState = JobTouchState::Updated;
    }

    m_touchPending = false;
}

void JobInputDispatcher::flushTabletTool(const JobInputEvent &syncEvent)
{
    if (!m_tabletToolPending)
        return;

    if (m_tabletToolCallback) {
        m_tabletToolCallback(JobTabletToolEvent(syncEvent.time(),
                                                EV_ABS,
                                                ABS_X,
                                                m_tabletX,
                                                m_tabletToolCode,
                                                static_cast<double>(m_tabletX),
                                                static_cast<double>(m_tabletY),
                                                static_cast<double>(m_tabletPressure),
                                                m_tabletToolState));
    }

    if (m_tabletToolState == JobTabletToolState::OutOfProximity) {
        m_tabletToolCode = 0;
        m_tabletToolState = JobTabletToolState::Unknown;
        m_tabletPressure = 0;
    }

    m_tabletToolPending = false;
}

} // namespace job::input