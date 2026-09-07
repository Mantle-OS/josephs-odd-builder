#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

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
    ~JobIoAsyncThread() noexcept override;

    JobIoAsyncThread(const JobIoAsyncThread &) = delete;
    JobIoAsyncThread &operator=(const JobIoAsyncThread &) = delete;
    JobIoAsyncThread(JobIoAsyncThread &&) = delete;
    JobIoAsyncThread &operator=(JobIoAsyncThread &&) = delete;

    void stop() override;

    [[nodiscard]] bool registerFD(int fd, IOEvent events, IOEventCallback callback);
    [[nodiscard]] bool modifyFD(int fd, IOEvent events);
    [[nodiscard]] bool unregisterFD(int fd);

    void post(std::function<void()> task, int priority = 0) override;

private:
    void loop(std::stop_token token, std::chrono::milliseconds idleHeartbeat) override;
    void processIOEvents(int eventCount);

    // Linux: epoll fd + eventfd + growable event buffer.
    // Windows: WSAPoll fd set + loopback wake-socket pair.
    struct Backend;

    std::unique_ptr<Backend> m_backend;

    std::map<int, IOEventCallback> m_fdCallbacks;
    mutable std::mutex m_ioMutex;
};

} // namespace job::threads