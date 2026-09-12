#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <sys/epoll.h>

#include "job_async_event_loop.h"
#include "job_io_event.h"
#include "jobthreads_export.h"

namespace job::threads {

using IOEventCallback = std::function<void(IOEvent events)>;

class JOBTHREADS_EXPORT JobIoAsyncThread : public AsyncEventLoop {
public:
    using Ptr = std::shared_ptr<JobIoAsyncThread>;
    using UPtr = std::unique_ptr<JobIoAsyncThread>;
    using WPtr = std::weak_ptr<JobIoAsyncThread>;
    using EventCallback = std::vector<std::pair<IOEventCallback, IOEvent>>;

    JobIoAsyncThread();
    ~JobIoAsyncThread() noexcept override;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static UPtr createUniq();

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

    struct Backend {
        int epollFd{-1};
        int eventFd{-1};
        std::vector<epoll_event> epollEvents;
    };
    std::unique_ptr<Backend> m_backend;

    std::unordered_map<int, IOEventCallback> m_fdCallbacks;
    mutable std::mutex m_ioMutex;
};

} // namespace job::threads