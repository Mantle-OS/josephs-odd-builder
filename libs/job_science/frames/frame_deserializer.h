#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <data/disk.h>
#include <data/particle.h>
#include <data/zones.h>

#include "frames/frame_header.h"
#include "frames/iframe_source.h"

namespace job::science::frames {

using namespace job::science::data;

// Decode frames coming from an IFrameSource into typed physics data.
class FrameDeserializer final {
public:
    using Ptr = std::shared_ptr<FrameDeserializer>;

    explicit FrameDeserializer(IFrameSource::Ptr source) noexcept;

    FrameDeserializer(const FrameDeserializer &)            = delete;
    FrameDeserializer &operator=(const FrameDeserializer &) = delete;

    // Read and decode the next frame from the source.
    //
    // Returns true on success and fills:
    //   - diskOut
    //   - zonesOut
    //   - particlesOut
    //
    // On failure, outputs are left unchanged.
    [[nodiscard]] bool readFrame(DiskModel &diskOut,
                                 Zones     &zonesOut,
                                 Particles &particlesOut);

    [[nodiscard]] bool isReady() const noexcept;
    void               reset();

    [[nodiscard]] IFrameSource::Ptr source() const noexcept
    {
        return m_source;
    }

private:
    static bool decodePayload(const FrameHeader                  &hdr,
                              const std::vector<std::uint8_t>    &payload,
                              DiskModel                          &diskOut,
                              Zones                              &zonesOut,
                              Particles                          &particlesOut);

    IFrameSource::Ptr m_source;
};

} // namespace job::science::frames
