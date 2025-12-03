#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <data/disk.h>
#include <data/particle.h>
#include <data/zones.h>

#include "frames/frame_header.h"
#include "frames/iframe_sink.h"

namespace job::science::frames {

using namespace job::science::data;

class FrameSerializer final {
public:
    using Ptr    = std::shared_ptr<FrameSerializer>;
    using FsPath = std::filesystem::path; // kept in case callers still use this typedef

    explicit FrameSerializer(IFrameSink::Ptr sink) noexcept;
    ~FrameSerializer() = default;

    FrameSerializer(const FrameSerializer &)            = delete;
    FrameSerializer &operator=(const FrameSerializer &) = delete;

    // Encode + write a frame to the attached sink.
    [[nodiscard]] bool writeFrame(std::uint64_t       frameId,
                                  const Particles    &particles,
                                  const DiskModel    &diskModel,
                                  const Zones        &zones);

    // Simple plumbing helpers around the sink
    [[nodiscard]] bool isOpen() const noexcept;
    void               flush();

    [[nodiscard]] IFrameSink::Ptr sink() const noexcept
    {
        return m_sink;
    }

private:
    IFrameSink::Ptr m_sink;
};

} // namespace job::science::frames
