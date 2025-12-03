#include "frame_replay.h"

#include <chrono>
#include <thread>
#include <cmath>
#include <memory>

#ifdef ENABLE_LOGGING
    #include <job_logger.h>
#endif

namespace job::science::frames {

using namespace std::chrono_literals;

FrameReplay::FrameReplay(const std::filesystem::path& inputPath,  FrameReader::Ptr reader) :
    m_inputPath(inputPath),
    m_reader(std::move(reader))
{
    // Pre-create the thread object with a low-priority, normal option
    JobThreadOptions opts = JobThreadOptions::normal();
    std::snprintf(opts.name.data(), opts.name.size(), "FrameReplay");
    m_thread = std::make_shared(JobThread{opts});
}

FrameReplay::~FrameReplay()
{
    stop();
}

void FrameReplay::setCallback(ReplayCallback cb)
{
    m_callback = std::move(cb);
}

void FrameReplay::setSpeedMultiplier(double multiplier) noexcept
{
    if (multiplier > 0.0)
        m_speedMultiplier.store(multiplier);
}

[[nodiscard]] bool FrameReplay::start() noexcept
{
    if (m_running.load())
        return false;

    if (!m_reader) {
#ifdef ENABLE_LOGGING
        JOB_LOG_ERROR("[FrameReplay] Cannot start: FrameReader not set.");
#endif
        return false;
    }

    // Reset timestamp for the start of a new playback sequence
    m_lastTimestampNs = 0;

    m_thread->setRunFunction([this](std::stop_token token) {
        this->playbackLoop(token);
    });

    if (m_thread->start() == JobThread::StartResult::Started) {
        m_running.store(true);
        return true;
    }

    return false;
}

void FrameReplay::stop() noexcept
{
    if (m_running.exchange(false)) {
        m_thread->requestStop();
        (void)m_thread->join();
    }
}

[[nodiscard]] bool FrameReplay::isRunning() const noexcept
{
    return m_running.load();
}

[[nodiscard]] double FrameReplay::getSpeedMultiplier() const noexcept
{
    return m_speedMultiplier.load();
}

[[nodiscard]] uint64_t FrameReplay::getCurrentFrameId() const noexcept
{
    return m_currentFrameId.load();
}


// DEMO ONLY ATM
// NOTE: This loop assumes the inputPath points to a single serialized frame file.
// Future work would involve iterating a directory of sequential frame files.
void FrameReplay::playbackLoop(std::stop_token token) noexcept
{
    // Variables for the frame data/config we will read
    FrameData frameData{};
    DiskModelPDL diskModel;
    ZonesPDL zones;


    if (token.stop_requested())
        return;

    // Read the Frame
    // FIXME FrameReader::readFrame needs to be called, passing the configuration struct pointers.
    frameData.particles = m_reader->readFrame(&diskModel, &zones, m_inputPath);

    // Check for I/O failure or empty data
    if (frameData.particles.empty()) {
#ifdef ENABLE_LOGGING
        JOB_LOGGER_ERROR("[FrameReplay] Failed to read frame or file is empty: {}", m_inputPath);
#endif
        m_running.store(false);
        return;
    }

    // NOTE: We need the FrameHeader to calculate time deltas.
    // The current FrameReader doesn't expose the header; we will assume
    // for this loop that the FrameReader is updated to pass the header back
    // or we read it separately. For now, we stub the frame ID and timestamp.

    // STUBS
    frameData.header.frameId = m_currentFrameId.load() + 1; // Simulate next frame ID
    frameData.header.timestampNs = m_lastTimestampNs + 10000000; // Simulate 10ms step
    // END STUBS

    frameData.diskModel = diskModel;
    frameData.zones = zones;
    frameData.header.particleCount = static_cast<uint32_t>(frameData.particles.size());


    // Real-Time Control
    uint64_t currentTimestampNs = frameData.header.timestampNs;

    if (m_lastTimestampNs != 0) {
        // Calculate the difference in time between this frame and the last one
        uint64_t deltaNs = currentTimestampNs - m_lastTimestampNs;

        // Apply the speed multiplier
        double multiplier = m_speedMultiplier.load();
        long long sleepNs = static_cast<long long>(std::round(static_cast<double>(deltaNs) / multiplier));

        // Ensure sleep time is non-negative
        if (sleepNs > 0) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    }

    m_lastTimestampNs = currentTimestampNs;
    m_currentFrameId.store(frameData.header.frameId);

    if (m_callback) {
        m_callback(frameData);
    }

    m_running.store(false);
}

}
