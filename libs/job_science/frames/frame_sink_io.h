#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <io_base.h>

#include "iframe_sink.h"
#include "frame_header.h"

namespace job::science::frames {

class FrameSinkIO final : public IFrameSink {
public:
    using Ptr         = std::shared_ptr<FrameSinkIO>;
    using IODevicePtr = std::shared_ptr<job::core::IODevice>;

    explicit FrameSinkIO(IODevicePtr device) noexcept;
    ~FrameSinkIO() override = default;

    FrameSinkIO(const FrameSinkIO &)            = delete;
    FrameSinkIO &operator=(const FrameSinkIO &) = delete;

    FrameSinkIO(FrameSinkIO &&) noexcept        = default;
    FrameSinkIO &operator=(FrameSinkIO &&) noexcept = default;

    // IFrameSink contract
    bool writeFrame(const FrameHeader &header, const std::uint8_t *payload, std::size_t payloadSize) override;

    [[nodiscard]] bool isReady() const noexcept override;

    void flush() override;

    // Extra helpers (not part of IFrameSink interface)
    void close();

    [[nodiscard]] IODevicePtr device() const noexcept
    {
        return m_device;
    }

private:
    IODevicePtr m_device;
};

} // namespace job::science::frames
