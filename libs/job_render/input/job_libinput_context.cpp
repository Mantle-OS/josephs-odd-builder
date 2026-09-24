#include "job_libinput_context.h"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <string>

namespace job::input {

JobLibinputContext::JobLibinputContext() = default;

JobLibinputContext::~JobLibinputContext()
{
    stop();
}

JobLibinputContext::Ptr JobLibinputContext::createShared()
{
    return std::make_shared<JobLibinputContext>();
}

JobLibinputContext::UPtr JobLibinputContext::createUniq()
{
    return std::make_unique<JobLibinputContext>();
}
/*
bool JobLibinputContext::start(threads::JobIoAsyncThread::Ptr loop, const EventCallback &callback)
{
    if (running())
        return true;

    if (!loop || !loop->isRunning())
        return false;

    static const libinput_interface interface = {
        .open_restricted = &JobLibinputContext::open,
        .close_restricted = &JobLibinputContext::close
    };

    m_udev = udev_new();

    if (!m_udev)
        return false;

    m_context = libinput_udev_create_context(&interface, this, m_udev);

    if (!m_context) {
        udev_unref(m_udev);
        m_udev = nullptr;
        return false;
    }

    if (libinput_udev_assign_seat(m_context, "seat0") != 0) {
        stop();
        return false;
    }

    m_fd = libinput_get_fd(m_context);

    if (m_fd < 0) {
        stop();
        return false;
    }

    m_loop = loop;
    m_callback = callback;

    if (!loop->registerFD(m_fd, threads::IOEvent::Read, [this](threads::IOEvent events) {
            onEvents(events);
        })) {
        stop();
        return false;
    }

    drainEvents();

    return true;
}
*/
bool JobLibinputContext::start(threads::JobIoAsyncThread::Ptr loop, const EventCallback &callback, std::string_view seat)
{
    // if(seat == "seat0")
    //     return start(loop, callback);

    // FIXME
    if (running())
        return true;

    if (!loop || !loop->isRunning())
        return false;

    static const libinput_interface interface = {
        .open_restricted = &JobLibinputContext::open,
        .close_restricted = &JobLibinputContext::close
    };

    m_udev = udev_new();

    if (!m_udev)
        return false;

    m_context = libinput_udev_create_context(&interface, this, m_udev);

    if (!m_context) {
        udev_unref(m_udev);
        m_udev = nullptr;
        return false;
    }

    const std::string seatName{seat};
    if (libinput_udev_assign_seat(m_context, seatName.c_str()) != 0) {
        stop();
        return false;
    }

    m_fd = libinput_get_fd(m_context);

    if (m_fd < 0) {
        stop();
        return false;
    }

    m_loop = loop;
    m_callback = callback;

    if (!loop->registerFD(m_fd, threads::IOEvent::Read, [this](threads::IOEvent events) {
            onEvents(events);
        })) {
        stop();
        return false;
    }

    drainEvents();

    return true;

}


void JobLibinputContext::stop()
{
    if (m_fd >= 0) {
        if (auto loop = m_loop.lock())
            (void)loop->unregisterFD(m_fd);
    }

    m_loop.reset();
    m_callback = nullptr;

    m_fd = -1;

    if (m_context) {
        libinput_unref(m_context);
        m_context = nullptr;
    }

    if (m_udev) {
        udev_unref(m_udev);
        m_udev = nullptr;
    }
}

bool JobLibinputContext::running() const noexcept
{
    return m_context != nullptr && m_fd >= 0 && !m_loop.expired();
}

int JobLibinputContext::open(const char *path, int flags, void *)
{
    const int fd = ::open(path, flags | O_CLOEXEC);

    if (fd < 0)
        return -errno;

    return fd;
}

void JobLibinputContext::close(int fd, void *)
{
    if (fd >= 0)
        ::close(fd);
}

void JobLibinputContext::onEvents(threads::IOEvent events)
{
    if ((events & threads::IOEvent::Error) == threads::IOEvent::Error) {
        stop();
        return;
    }

    if ((events & threads::IOEvent::HangUp) == threads::IOEvent::HangUp) {
        stop();
        return;
    }

    if ((events & threads::IOEvent::Read) == threads::IOEvent::Read)
        drainEvents();
}

void JobLibinputContext::drainEvents()
{
    if (!m_context)
        return;

    if (libinput_dispatch(m_context) != 0)
        return;

    while (libinput_event *event = libinput_get_event(m_context)) {
        parseEvent(event);
        libinput_event_destroy(event);
    }
}

void JobLibinputContext::parseEvent(libinput_event *event)
{
    if (!event)
        return;

    switch (libinput_event_get_type(event)) {
    case LIBINPUT_EVENT_DEVICE_ADDED:
        deviceAdded(event);
        break;

    case LIBINPUT_EVENT_DEVICE_REMOVED:
        deviceRemoved(event);
        break;

    case LIBINPUT_EVENT_KEYBOARD_KEY:
        key(event);
        break;

    case LIBINPUT_EVENT_POINTER_MOTION:
        pointerMotion(event);
        break;

    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        pointerMotionAbsolute(event);
        break;

    case LIBINPUT_EVENT_POINTER_BUTTON:
        pointerButton(event);
        break;

    case LIBINPUT_EVENT_POINTER_SCROLL_WHEEL:
    case LIBINPUT_EVENT_POINTER_SCROLL_FINGER:
    case LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS:
        pointerScroll(event);
        break;

    case LIBINPUT_EVENT_TOUCH_DOWN:
        touchDown(event);
        break;

    case LIBINPUT_EVENT_TOUCH_MOTION:
        touchMotion(event);
        break;

    case LIBINPUT_EVENT_TOUCH_UP:
        touchUp(event);
        break;

    case LIBINPUT_EVENT_TOUCH_CANCEL:
        touchCancel(event);
        break;

    default:
        break;
    }
}

void JobLibinputContext::deviceAdded(libinput_event *event)
{
    auto *device = libinput_event_get_device(event);

    DeviceEvent inputEvent{
        .event = {
            .type = InputEventType::DeviceAdded,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = 0
        }
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::deviceRemoved(libinput_event *event)
{
    auto *device = libinput_event_get_device(event);

    DeviceEvent inputEvent{
        .event = {
            .type = InputEventType::DeviceRemoved,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = 0
        }
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::key(libinput_event *event)
{
    auto *keyboard = libinput_event_get_keyboard_event(event);

    if (!keyboard)
        return;

    auto *device = libinput_event_get_device(event);

    const auto state = libinput_event_keyboard_get_key_state(keyboard);

    KeyEvent inputEvent{
        .event = {
            .type = state == LIBINPUT_KEY_STATE_PRESSED ? InputEventType::KeyPress : InputEventType::KeyRelease,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_keyboard_get_time_usec(keyboard)
        },
        .key = libinput_event_keyboard_get_key(keyboard)
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::pointerMotion(libinput_event *event)
{
    auto *pointer = libinput_event_get_pointer_event(event);

    if (!pointer)
        return;

    auto *device = libinput_event_get_device(event);

    PointerMotionEvent inputEvent{
        .event = {
            .type = InputEventType::PointerMotion,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_pointer_get_time_usec(pointer)
        },
        .x = 0.0,
        .y = 0.0,
        .deltaX = libinput_event_pointer_get_dx(pointer),
        .deltaY = libinput_event_pointer_get_dy(pointer)
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::pointerMotionAbsolute(libinput_event *event)
{
    auto *pointer = libinput_event_get_pointer_event(event);

    if (!pointer)
        return;

    auto *device = libinput_event_get_device(event);

    PointerMotionEvent inputEvent{
        .event = {
            .type = InputEventType::PointerMotion,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_pointer_get_time_usec(pointer)
        },
        .x = libinput_event_pointer_get_absolute_x(pointer),
        .y = libinput_event_pointer_get_absolute_y(pointer),
        .deltaX = 0.0,
        .deltaY = 0.0
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::pointerButton(libinput_event *event)
{
    auto *pointer = libinput_event_get_pointer_event(event);

    if (!pointer)
        return;

    auto *device = libinput_event_get_device(event);

    const auto state = libinput_event_pointer_get_button_state(pointer);

    PointerButtonEvent inputEvent{
        .event = {
            .type = state == LIBINPUT_BUTTON_STATE_PRESSED
                        ? InputEventType::PointerButtonPress
                        : InputEventType::PointerButtonRelease,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_pointer_get_time_usec(pointer)
        },
        .button = libinput_event_pointer_get_button(pointer)
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::pointerScroll(libinput_event *event)
{
    auto *pointer = libinput_event_get_pointer_event(event);

    if (!pointer)
        return;

    auto *device = libinput_event_get_device(event);

    PointerScrollEvent inputEvent{
        .event = {
            .type = InputEventType::PointerScroll,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_pointer_get_time_usec(pointer)
        },
        .horizontal = libinput_event_pointer_get_scroll_value(pointer, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL),
        .vertical = libinput_event_pointer_get_scroll_value(pointer, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::touchDown(libinput_event *event)
{
    auto *touch = libinput_event_get_touch_event(event);

    if (!touch)
        return;

    auto *device = libinput_event_get_device(event);

    TouchEvent inputEvent{
        .event = {
            .type = InputEventType::TouchDown,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_touch_get_time_usec(touch)
        },
        .slot = libinput_event_touch_get_seat_slot(touch),
        .x = libinput_event_touch_get_x(touch),
        .y = libinput_event_touch_get_y(touch)
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::touchMotion(libinput_event *event)
{
    auto *touch = libinput_event_get_touch_event(event);

    if (!touch)
        return;

    auto *device = libinput_event_get_device(event);

    TouchEvent inputEvent{
        .event = {
            .type = InputEventType::TouchMotion,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_touch_get_time_usec(touch)
        },
        .slot = libinput_event_touch_get_seat_slot(touch),
        .x = libinput_event_touch_get_x(touch),
        .y = libinput_event_touch_get_y(touch)
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::touchUp(libinput_event *event)
{
    auto *touch = libinput_event_get_touch_event(event);

    if (!touch)
        return;

    auto *device = libinput_event_get_device(event);

    TouchEvent inputEvent{
        .event = {
            .type = InputEventType::TouchUp,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_touch_get_time_usec(touch)
        },
        .slot = libinput_event_touch_get_seat_slot(touch),
        .x = 0.0,
        .y = 0.0
    };

    if (m_callback)
        m_callback(inputEvent);
}

void JobLibinputContext::touchCancel(libinput_event *event)
{
    auto *touch = libinput_event_get_touch_event(event);

    if (!touch)
        return;

    auto *device = libinput_event_get_device(event);

    TouchEvent inputEvent{
        .event = {
            .type = InputEventType::TouchCancel,
            .deviceType = deviceType(device),
            .deviceId = deviceId(device),
            .timestamp = libinput_event_touch_get_time_usec(touch)
        },
        .slot = libinput_event_touch_get_seat_slot(touch),
        .x = 0.0,
        .y = 0.0
    };

    if (m_callback)
        m_callback(inputEvent);
}

InputDeviceId JobLibinputContext::deviceId(libinput_device *device) const noexcept
{
    return static_cast<InputDeviceId>(
        reinterpret_cast<std::uintptr_t>(device)
        );
}

InputDeviceType JobLibinputContext::deviceType(libinput_device *device) const noexcept
{
    if (!device)
        return InputDeviceType::Unknown;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH))
        return InputDeviceType::Touch;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TABLET_TOOL))
        return InputDeviceType::Tablet;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD))
        return InputDeviceType::Keyboard;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER))
        return InputDeviceType::Pointer;

    return InputDeviceType::Unknown;
}

} // namespace job::input