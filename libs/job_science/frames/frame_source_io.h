// frames/frame_source_io.h

#pragma once

#include <memory>
#include <atomic>
#include <optional>

#include <io_base.h>            // job::core::IODevice
#include "frames/iframe_source.h"
#include "frames/frame_header.h"

namespace job::science::frames {

class FrameSourceIo : public IFrameSource {
public:
    using Ptr      = std::shared_ptr<FrameSourceIo>;
    using Device   = job::core::IODevice;
    using DevicePtr = std::shared_ptr<Device>;

    explicit FrameSourceIo(DevicePtr dev, bool autoOpen = true) noexcept;

    bool open();
    void close();

    [[nodiscard]] bool isReady() const noexcept override;
    [[nodiscard]] std::optional<FrameHeader> readHeader() override;
    [[nodiscard]] std::size_t readPayload(std::uint8_t *dst,
                                          std::size_t maxSize) override;
    void reset() override;

    [[nodiscard]] DevicePtr device() const noexcept { return m_device; }

private:
    [[nodiscard]] bool readExact(std::uint8_t *dst, std::size_t size);

    DevicePtr                    m_device;
    bool                         m_autoOpen{true};
    std::atomic<bool>            m_open{false};
    std::optional<FrameHeader>   m_lastHeader;
    std::size_t                  m_payloadRemaining{0};
};

} // namespace job::science::frames
