#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "frame_header.h"

namespace job::science::frames {

// Where frames go when they grow up and leave the integrator.
struct IFrameSink {
    using Ptr = std::shared_ptr<IFrameSink>;
    virtual ~IFrameSink() = default;

    // One-shot write of header+payload (or at least header + pointer/size)
    [[nodiscard]] virtual bool writeFrame(const FrameHeader &header,
                                          const std::uint8_t *data,
                                          std::size_t size) = 0;

    // “Ready to accept frames?” / “Open?”
    [[nodiscard]] virtual bool isReady() const noexcept = 0;

    // Flush any buffered data; can be a no-op.
    virtual void flush() = 0;

protected:
    IFrameSink() = default;
    IFrameSink(const IFrameSink &) = default;
    IFrameSink &operator=(const IFrameSink &) = default;
};

} // namespace job::science::frames
