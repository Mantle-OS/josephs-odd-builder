#pragma once
#include <map>
#include <memory>
#include <functional>

#include "job_async_event_loop.h"
#include "job_io_event.h"
#include "jobthreads_export.h"

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
    bool modifyFD(int fd, IOEvent events);
    bool unregisterFD(int fd);

    void post(std::function<void()> task, int priority = 0) override;

private:
    void loop(std::stop_token token, std::chrono::milliseconds idle_heartbeat) override;
    void processIOEvents(int event_count);

    // Linux(): an epoll fd + an eventfd + a growable event buffer;
    // Windows(job_io_async_thread_win32.cpp): a WSAPoll fd set + a loopback wake-socket pair
    struct Backend;
    std::unique_ptr<Backend>            m_backend;

    std::map<int, IOEventCallback>      m_fdCallbacks;
    mutable std::mutex                  m_ioMutex;
};

} // namespace job::threads
