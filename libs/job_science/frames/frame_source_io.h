#pragma once

#include <memory>
#include <atomic>
#include <optional>

// job::core
#include <io_base.h>

#include "frames/iframe_source.h"
#include "frames/frame_header.h"

namespace job::science::frames {
using job::core::IODevice;

class FrameSourceIO : public IFrameSource {
public:
    using Ptr      = std::shared_ptr<FrameSourceIO>;
    explicit FrameSourceIO(IODevice::Ptr dev, bool autoOpen = true) noexcept;

    bool open();
    void close();

    [[nodiscard]] bool isReady() const noexcept override;
    [[nodiscard]] std::optional<FrameHeader> readHeader() override;
    [[nodiscard]] std::size_t readPayload(std::uint8_t *dst, std::size_t maxSize) override;
    void reset() override;

    [[nodiscard]] IODevice::Ptr device() const noexcept;

private:
    [[nodiscard]] bool readExact(std::uint8_t *dst, std::size_t size);

    IODevice::Ptr                m_device;
    bool                         m_autoOpen{true};
    std::atomic<bool>            m_open{false};
    std::optional<FrameHeader>   m_lastHeader;
    std::size_t                  m_payloadRemaining{0};
};

} // namespace job::science::frames
