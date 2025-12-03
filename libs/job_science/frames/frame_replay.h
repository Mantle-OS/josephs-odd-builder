#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <functional>
#include <atomic>

#include <data/particle.h>
#include <data/disk.h>
#include <data/zones.h>

#include <job_thread.h>

#include "frame_header.h"
#include "frame_deserializer.h"

namespace job::science::frames {
using namespace job::threads;

using DiskModelPDL = job::science::data::DiskModel;
using ZonesPDL = job::science::data::Zones;
// callback for GUI or IPC etc
struct FrameData final {
    FrameHeader header;
    DiskModelPDL diskModel;
    ZonesPDL zones;
    std::vector<data::Particle> particles;
};

using ReplayCallback = std::function<void(const FrameData&)>;

class FrameReplay final {
public:
    using Ptr = std::shared_ptr<FrameReplay>;

    explicit FrameReplay(const std::filesystem::path& inputPath,
                         FrameReader::Ptr reader);
    ~FrameReplay();

    FrameReplay(const FrameReplay&) = delete;
    FrameReplay& operator=(const FrameReplay&) = delete;

    // Pacing
    void setCallback(ReplayCallback cb);
    void setSpeedMultiplier(double multiplier) noexcept;

    [[nodiscard]] bool start() noexcept;
    void stop() noexcept;

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] double getSpeedMultiplier() const noexcept;
    [[nodiscard]] uint64_t getCurrentFrameId() const noexcept;

private:
    // Core playback loop running in a separate thread
    void playbackLoop(std::stop_token token) noexcept;

    std::filesystem::path   m_inputPath;
    FrameReader::Ptr        m_reader;
    ReplayCallback          m_callback;
    JobThread::Ptr          m_thread;
    std::atomic<bool>       m_running{false};
    std::atomic<uint64_t>   m_currentFrameId{0};
    std::atomic<double>     m_speedMultiplier{1.0};
    // Tracks time delta between frames
    uint64_t                m_lastTimestampNs{0};
};

}
