#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <job_io_async_thread.h>

#include "job_input_dispatcher.h"
#include "linux/job_input_linux_device.h"
#include "linux/job_input_linux_reader.h"

namespace job::input {

class JobInputLoop : public std::enable_shared_from_this<JobInputLoop>
{
public:
    using Ptr  = std::shared_ptr<JobInputLoop>;
    using WPtr = std::weak_ptr<JobInputLoop>;
    using UPtr = std::unique_ptr<JobInputLoop>;

    explicit JobInputLoop(threads::JobIoAsyncThread::Ptr loop);

    ~JobInputLoop();

    JobInputLoop(const JobInputLoop &) = delete;
    JobInputLoop &operator=(const JobInputLoop &) = delete;

    JobInputLoop(JobInputLoop &&) = delete;
    JobInputLoop &operator=(JobInputLoop &&) = delete;

    [[nodiscard]] static Ptr createShared(threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_shared<JobInputLoop>(std::move(loop));
    }

    [[nodiscard]] bool addDevice(const JobInputLinuxDevice::Ptr &device);
    bool removeDevice(const JobInputLinuxDevice::Ptr &device);
    void clear();

    [[nodiscard]] std::size_t deviceCount() const noexcept
    {
        return m_devices.size();
    }

private:
    struct DeviceContext
    {
        JobInputLinuxDevice::Ptr device;
        JobInputLinuxReader::Ptr reader;
        JobInputDispatcher::Ptr dispatcher;
    };

    using DeviceContexts = std::vector<DeviceContext>;
    void handleReadable(const JobInputLinuxDevice::Ptr &device);

    [[nodiscard]] DeviceContexts::iterator findDevice(const JobInputLinuxDevice::Ptr &device);
    [[nodiscard]] DeviceContexts::const_iterator findDevice(const JobInputLinuxDevice::Ptr &device) const;

    threads::JobIoAsyncThread::Ptr  m_loop;
    DeviceContexts                  m_devices;
};

} // namespace job::input