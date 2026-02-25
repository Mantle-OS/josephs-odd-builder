#pragma once

#include <map>
#include <functional>

#include <sys/epoll.h>

#include "job_async_event_loop.h"

namespace job::threads {

using IOEventCallback = std::function<void(uint32_t events)>;

class JobIoAsyncThread : public AsyncEventLoop {
public:
    using Ptr = std::shared_ptr<JobIoAsyncThread>;
    JobIoAsyncThread();
    virtual ~JobIoAsyncThread() noexcept override;

    JobIoAsyncThread(const JobIoAsyncThread &) = delete;
    JobIoAsyncThread &operator=(const JobIoAsyncThread &) = delete;
    void stop() override;
    bool registerFD(int fd, uint32_t events, IOEventCallback callback);
    bool unregisterFD(int fd);
    void post(std::function<void()> task, int priority = 0) override;

private:
    void loop(std::stop_token token, std::chrono::milliseconds idle_heartbeat) override;
    void processIOEvents(int event_count);

    int                             m_epollFd{-1};
    int                             m_eventFd{-1}; // wake up sleepy head
    std::vector<epoll_event>        m_epollEvents;
    std::map<int, IOEventCallback>  m_fdCallbacks;
    mutable std::mutex              m_ioMutex;
};

} // namespace job::threads
