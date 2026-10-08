#include "job_input_device_classification.h"

#include <linux/input-event-codes.h>

namespace job::input {

JobInputDeviceClassification::JobInputDeviceClassification(const JobInputCapability &capability) noexcept :
    m_types(classify(capability))
{
}

JobInputDeviceClassification::Type JobInputDeviceClassification::classify(const JobInputCapability &capability) noexcept
{
    Type types = Type::None;

    const bool keyboard         = isKeyboard(capability);
    const bool pointer          = isPointer(capability);
    const bool trackball        = isTrackball(capability);
    const bool touchpad         = isTouchpad(capability);
    const bool touch            = isTouch(capability);
    const bool tabletTool       = isTabletTool(capability);
    const bool tabletPad        = isTabletPad(capability);
    const bool gamepad          = isGamepad(capability);
    const bool joystick         = isJoystick(capability);
    const bool sw               = isSwitch(capability);
    const bool dial             = isDial(capability);
    const bool accelerometer    = isAccelerometer(capability);

    if (keyboard)
        types |= Type::Keyboard;

    if (pointer || trackball || touchpad)
        types |= Type::Pointer;

    if (trackball)
        types |= Type::Trackball;

    if (touchpad)
        types |= Type::Touchpad;

    if (touch)
        types |= Type::Touch;

    if (tabletTool)
        types |= Type::TabletTool;

    if (tabletPad)
        types |= Type::TabletPad;

    if (gamepad)
        types |= Type::Gamepad;

    if (joystick)
        types |= Type::Joystick;

    if (sw)
        types |= Type::Switch;

    if (dial)
        types |= Type::Dial;

    if (accelerometer)
        types |= Type::Accelerometer;

    return types;
}

bool JobInputDeviceClassification::isKeyboard(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_KEY))
        return false;

    return capability.key(KEY_A) && capability.key(KEY_Z) && capability.key(KEY_ENTER);
}

bool JobInputDeviceClassification::isPointer(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_REL))
        return false;

    return capability.relativeAxis(REL_X) && capability.relativeAxis(REL_Y);
}

bool JobInputDeviceClassification::isTrackball(const JobInputCapability &capability) noexcept
{
    // Shit .... A trackball generally exposes the same evdev capability pattern as a
    // conventional relative mouse. Capability data alone is not sufficient
    // to classify one reliably.
    (void)capability;
    return false;
}

bool JobInputDeviceClassification::isTouchpad(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_ABS))
        return false;

    if (capability.deviceProperty(INPUT_PROP_DIRECT))
        return false;

    if (capability.deviceProperty(INPUT_PROP_BUTTONPAD) ||
        capability.deviceProperty(INPUT_PROP_TOPBUTTONPAD)) {
        return true;
    }

    const bool hasPosition = (capability.absoluteAxis(ABS_X) && capability.absoluteAxis(ABS_Y)) ||
                             (capability.absoluteAxis(ABS_MT_POSITION_X) && capability.absoluteAxis(ABS_MT_POSITION_Y));

    if (!hasPosition)
        return false;

    return capability.deviceProperty(INPUT_PROP_POINTER) && (capability.key(BTN_TOUCH) || capability.key(BTN_TOOL_FINGER));
}

bool JobInputDeviceClassification::isTouch(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_ABS))
        return false;

    if (!capability.deviceProperty(INPUT_PROP_DIRECT))
        return false;

    const bool hasSingleTouchPosition = capability.absoluteAxis(ABS_X) && capability.absoluteAxis(ABS_Y);
    const bool hasMultiTouchPosition = capability.absoluteAxis(ABS_MT_POSITION_X) && capability.absoluteAxis(ABS_MT_POSITION_Y);

    return hasSingleTouchPosition || hasMultiTouchPosition;
}

bool JobInputDeviceClassification::isTabletTool(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_ABS) || !capability.eventType(EV_KEY))
        return false;

    const bool hasPosition = capability.absoluteAxis(ABS_X) && capability.absoluteAxis(ABS_Y);
    if (!hasPosition)
        return false;

    return capability.key(BTN_TOOL_PEN)         ||
           capability.key(BTN_TOOL_RUBBER)      ||
           capability.key(BTN_TOOL_BRUSH)       ||
           capability.key(BTN_TOOL_PENCIL)      ||
           capability.key(BTN_TOOL_AIRBRUSH)    ||
           capability.key(BTN_TOOL_MOUSE)       ||
           capability.key(BTN_TOOL_LENS);
}

bool JobInputDeviceClassification::isTabletPad(const JobInputCapability &capability) noexcept
{
    // Shit again .... Separate tablet-pad interfaces vary considerably between vendors.
    // Buttons, rings and strips can overlap with other EV_KEY/EV_ABS devices,
    // so do not guess from capability bits alone yet.
    (void)capability;
    return false;
}

bool JobInputDeviceClassification::isGamepad(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_KEY))
        return false;

    return capability.key(BTN_SOUTH)    ||
           capability.key(BTN_EAST)     ||
           capability.key(BTN_NORTH)    ||
           capability.key(BTN_WEST)     ||
           capability.key(BTN_TL)       ||
           capability.key(BTN_TR)       ||
           capability.key(BTN_SELECT)   ||
           capability.key(BTN_START)    ||
           capability.key(BTN_MODE)     ||
           capability.key(BTN_THUMBL)   ||
           capability.key(BTN_THUMBR);
}

bool JobInputDeviceClassification::isJoystick(const JobInputCapability &capability) noexcept
{
    if (!capability.eventType(EV_KEY))
        return false;

    return capability.key(BTN_TRIGGER)  ||
           capability.key(BTN_THUMB)    ||
           capability.key(BTN_THUMB2)   ||
           capability.key(BTN_TOP)      ||
           capability.key(BTN_TOP2)     ||
           capability.key(BTN_PINKIE)   ||
           capability.key(BTN_BASE)     ||
           capability.key(BTN_BASE2)    ||
           capability.key(BTN_BASE3)    ||
           capability.key(BTN_BASE4)    ||
           capability.key(BTN_BASE5)    ||
           capability.key(BTN_BASE6)    ||
           capability.key(BTN_DEAD);
}

bool JobInputDeviceClassification::isSwitch(const JobInputCapability &capability) noexcept
{
    return capability.eventType(EV_SW);
}

bool JobInputDeviceClassification::isDial(const JobInputCapability &capability) noexcept
{
    return capability.eventType(EV_REL) && capability.relativeAxis(REL_DIAL);
}

bool JobInputDeviceClassification::isAccelerometer(const JobInputCapability &capability) noexcept
{
    return capability.deviceProperty(INPUT_PROP_ACCELEROMETER);
}

} // namespace job::input