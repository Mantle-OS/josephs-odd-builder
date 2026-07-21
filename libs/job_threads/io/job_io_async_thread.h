#pragma once
#include "jobthreads_export.h"
#include <map>
#include <memory>
#include <functional>
#include "job_async_event_loop.h"
#include "job_io_event.h"

namespace job::threads {

using IOEventCallback = std::function<void(IOEvent events)>;

class JOBTHREADS_EXPORT JobIoAsyncThread : public AsyncEventLoop {
public:
    using Ptr = std::shared_ptr<JobIoAsyncThread>;
    using Event_Callback = std::vector<std::pair<IOEventCallback, IOEvent>>;
    JobIoAsyncThread();
    virtual ~JobIoAsyncThread() noexcept override;

    JobIoAsyncThread(const JobIoAsyncThread &) = delete;
    JobIoAsyncThread &operator=(const JobIoAsyncThread &) = delete;

    void stop() override;
    bool registerFD(int fd, IOEvent events, IOEventCallback callback);
    bool unregisterFD(int fd);
    void post(std::function<void()> task, int priority = 0) override;

private:
    void loop(std::stop_token token, std::chrono::milliseconds idle_heartbeat) override;
    void processIOEvents(int event_count);

    // posix: an epoll fd + an eventfd + a growable event buffer;
    // windows: a WSAPoll fd set + a loopback wake-socket pair
    struct Backend;
    std::unique_ptr<Backend> m_backend;

    std::map<int, IOEventCallback>  m_fdCallbacks;
    mutable std::mutex              m_ioMutex;
};

} // namespace job::threads




// #pragma once

// #include <map>
// #include <functional>

// #include <sys/epoll.h>

// #include "job_async_event_loop.h"
// #include "jobthreads_export.h"
// namespace job::threads {

// using IOEventCallback = std::function<void(uint32_t events)>;

// class JOBTHREADS_EXPORT JobIoAsyncThread : public AsyncEventLoop {
// public:
//     using Ptr = std::shared_ptr<JobIoAsyncThread>;
//     JobIoAsyncThread();
//     virtual ~JobIoAsyncThread() noexcept override;

//     JobIoAsyncThread(const JobIoAsyncThread &) = delete;
//     JobIoAsyncThread &operator=(const JobIoAsyncThread &) = delete;
//     void stop() override;
//     bool registerFD(int fd, uint32_t events, IOEventCallback callback);
//     bool unregisterFD(int fd);
//     void post(std::function<void()> task, int priority = 0) override;

// private:
//     void loop(std::stop_token token, std::chrono::milliseconds idle_heartbeat) override;
//     void processIOEvents(int event_count);

//     int                             m_epollFd{-1};
//     int                             m_eventFd{-1}; // wake up sleepy head
//     std::vector<epoll_event>        m_epollEvents;
//     std::map<int, IOEventCallback>  m_fdCallbacks;
//     mutable std::mutex              m_ioMutex;
// };

// } // namespace job::threads
