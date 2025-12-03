#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "frame_header.h"

namespace job::science::frames {

// Where frames come *from* (files, sockets, shared memory, carrier pigeons).
struct IFrameSource {
    using Ptr = std::shared_ptr<IFrameSource>;
    virtual ~IFrameSource() = default;

    // "Chunked" API in my head:
    [[nodiscard]] virtual bool isReady() const noexcept = 0;

    [[nodiscard]] virtual std::optional<FrameHeader> readHeader() = 0;

    [[nodiscard]] virtual std::size_t readPayload(std::uint8_t *dst,
                                                  std::size_t maxSize) = 0;

    virtual void reset() {}
protected:
    IFrameSource() = default;
    IFrameSource(const IFrameSource &) = default;
    IFrameSource &operator=(const IFrameSource &) = default;
};

} // namespace job::science::frames
