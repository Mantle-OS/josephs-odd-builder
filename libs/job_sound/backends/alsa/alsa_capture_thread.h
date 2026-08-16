#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <job_thread.h>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaCaptureThread
{
public:
    using Ptr  = std::shared_ptr<AlsaCaptureThread>;
    using WPtr = std::weak_ptr<AlsaCaptureThread>;
    using UPtr = std::unique_ptr<AlsaCaptureThread>;

    using SamplesReady = std::function<void(const std::vector<float> &samples)>;

    static Ptr createShared(const std::string &device = "default")
    {
        return std::make_shared<AlsaCaptureThread>(device);
    }

    static UPtr createUnique(const std::string &device = "default")
    {
        return std::make_unique<AlsaCaptureThread>(device);
    }

    explicit AlsaCaptureThread(const std::string &device = "default");

    AlsaCaptureThread(const AlsaCaptureThread &) = delete;
    AlsaCaptureThread(AlsaCaptureThread &&) = delete;

    AlsaCaptureThread &operator=(const AlsaCaptureThread &) = delete;
    AlsaCaptureThread &operator=(AlsaCaptureThread &&) = delete;

    ~AlsaCaptureThread();
    [[nodiscard]] job::threads::JobThread::StartResult start();
    void stop() noexcept;
    [[nodiscard]] bool join() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    [[nodiscard]] const std::string &deviceName() const noexcept;
    void setDeviceName(const std::string &device);
    void setSamplesReady(SamplesReady callback);

private:
    void run(std::stop_token token);

private:
    SamplesReady            m_samplesReady;
    std::string             m_deviceName{"default"};
    job::threads::JobThread m_thread;
};

} // namespace job::sound