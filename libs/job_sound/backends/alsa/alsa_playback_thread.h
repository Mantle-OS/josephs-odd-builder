#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


#include <job_thread.h>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaPlaybackThread
{
public:
    using Ptr  = std::shared_ptr<AlsaPlaybackThread>;
    using WPtr = std::weak_ptr<AlsaPlaybackThread>;
    using UPtr = std::unique_ptr<AlsaPlaybackThread>;

    using PlayedSamples = std::function<void(const std::vector<float> &samples)>;

    static Ptr createShared(const std::string &device = "default")
    {
        return std::make_shared<AlsaPlaybackThread>(device);
    }

    static UPtr createUnique(const std::string &device = "default")
    {
        return std::make_unique<AlsaPlaybackThread>(device);
    }

    explicit AlsaPlaybackThread(const std::string &device = "default");

    AlsaPlaybackThread(const AlsaPlaybackThread &) = delete;
    AlsaPlaybackThread(AlsaPlaybackThread &&) = delete;

    AlsaPlaybackThread &operator=(const AlsaPlaybackThread &) = delete;
    AlsaPlaybackThread &operator=(AlsaPlaybackThread &&) = delete;

    ~AlsaPlaybackThread();

    [[nodiscard]] job::threads::JobThread::StartResult start();
    void stop() noexcept;
    [[nodiscard]] bool join() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    [[nodiscard]] const std::string &deviceName() const noexcept;
    void setDeviceName(const std::string &device);
    void setPlayedSamples(PlayedSamples callback);
    void enqueue(const std::vector<float> &samples);

private:
    void run(std::stop_token token);

private:
    std::mutex                     m_mutex;
    std::vector<float>             m_queue;
    std::string                    m_deviceName{"default"};
    PlayedSamples                  m_playedSamples;
    job::threads::JobThread        m_thread;
};

} // namespace job::sound