#pragma once


#include <functional>
#include <memory>
#include <string_view>

#include <libinput.h>
#include <libudev.h>

#include <job_io_async_thread.h>

#include "job_input_event.h"

namespace job::input {

class JobLibinputContext
{
public:
    using Ptr           = std::shared_ptr<JobLibinputContext>;
    using WPtr          = std::weak_ptr<JobLibinputContext>;
    using UPtr          = std::unique_ptr<JobLibinputContext>;
    using Loop          = std::weak_ptr<threads::JobIoAsyncThread>;
    using EventCallback = std::function<void(const InputEvent &)>;

    JobLibinputContext();
    ~JobLibinputContext();

    JobLibinputContext(const JobLibinputContext &) = delete;
    JobLibinputContext &operator=(const JobLibinputContext &) = delete;
    JobLibinputContext(JobLibinputContext &&) = delete;
    JobLibinputContext &operator=(JobLibinputContext &&) = delete;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static UPtr createUniq();

    // [[nodiscard]] bool start(threads::JobIoAsyncThread::Ptr loop, const EventCallback &callback);
    [[nodiscard]] bool start(threads::JobIoAsyncThread::Ptr loop, const EventCallback &callback, std::string_view seat = "seat0");
    void stop();

    [[nodiscard]] bool running() const noexcept;

private:
    // libinput open_restricted callback
    static int open(const char *path, int flags, void *userData);

    // libinput close_restricted callback
    static void close(int fd, void *userData);

    // Entry point for events from the async I/O loop.
    void onEvents(threads::IOEvent events);

    // Drain all pending libinput events.
    void drainEvents();

    // Parse one libinput event.
    void parseEvent(struct libinput_event *event);

    void deviceAdded(struct libinput_event *event);
    void deviceRemoved(struct libinput_event *event);

    void key(struct libinput_event *event);

    void pointerMotion(struct libinput_event *event);
    void pointerMotionAbsolute(struct libinput_event *event);
    void pointerButton(struct libinput_event *event);
    void pointerScroll(struct libinput_event *event);

    void touchDown(struct libinput_event *event);
    void touchMotion(struct libinput_event *event);
    void touchUp(struct libinput_event *event);
    void touchCancel(struct libinput_event *event);

    [[nodiscard]] InputDeviceId deviceId(struct libinput_device *device) const noexcept;
    [[nodiscard]] InputDeviceType deviceType(struct libinput_device *device) const noexcept;

    Loop            m_loop;
    EventCallback   m_callback;
    struct udev     *m_udev = nullptr;
    struct libinput *m_context = nullptr;

    int m_fd = -1;
};

} // namespace job::input