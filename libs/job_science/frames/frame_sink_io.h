#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

// job::core
#include <io_base.h>

#include "iframe_sink.h"
#include "frame_header.h"

namespace job::science::frames {
using job::core::IODevice;

class FrameSinkIO final : public IFrameSink {
public:
    using Ptr = std::shared_ptr<FrameSinkIO>;

    explicit FrameSinkIO(IODevice::Ptr device) noexcept;
    ~FrameSinkIO() override = default;

    FrameSinkIO(const FrameSinkIO &) = delete;
    FrameSinkIO &operator=(const FrameSinkIO &) = delete;

    FrameSinkIO(FrameSinkIO &&) noexcept = default;
    FrameSinkIO &operator=(FrameSinkIO &&) noexcept = default;

    // IFrameSink contract
    bool writeFrame(const FrameHeader &header, const std::uint8_t *payload, std::size_t payloadSize) override;

    [[nodiscard]] bool isReady() const noexcept override;

    void flush() override;
    void close();

    [[nodiscard]] IODevice::Ptr device() const noexcept;

private:
    IODevice::Ptr m_device;
};

} // namespace job::science::frames
