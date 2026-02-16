#include "frame_sink_io.h"

#include <array>
#include <cstring>

namespace job::science::frames {

using job::core::IODevice;

FrameSinkIO::FrameSinkIO(IODevice::Ptr device) noexcept :
    m_device(std::move(device))
{
}

static bool ensureOpen(const IODevice::Ptr &dev)
{
    if (!dev)
        return false;

    if (!dev->isOpen())
        return dev->openDevice();

    return true;
}

bool FrameSinkIO::writeFrame(const FrameHeader &header,
                             const std::uint8_t *payload,
                             std::size_t payloadSize)
{
    if (!ensureOpen(m_device))
        return false;

    // Header must be written as a fixed-size, aligned block.
    std::array<std::uint8_t, FrameHeader::kAlignedSize> headerBuf{};
    static_assert(sizeof(FrameHeader) <= FrameHeader::kAlignedSize,
                  "FrameHeader larger than kAlignedSize");

    // Copy the header bytes into the aligned buffer.
    std::memcpy(headerBuf.data(), &header, sizeof(FrameHeader));

    // Write header block.
    {
        const auto *data = reinterpret_cast<const char *>(headerBuf.data());
        const std::size_t total = headerBuf.size();
        std::size_t written = 0;

        while (written < total) {
            const auto rc = m_device->write(data + written, total - written);
            if (rc <= 0)
                return false; // error or EOF-ish

            written += static_cast<std::size_t>(rc);
        }
    }

    // Write payload (if any).
    if (payload && payloadSize > 0) {
        const auto *data = reinterpret_cast<const char *>(payload);
        const std::size_t total = payloadSize;
        std::size_t written = 0;

        while (written < total) {
            const auto rc = m_device->write(data + written, total - written);
            if (rc <= 0)
                return false; // error or EOF-ish

            written += static_cast<std::size_t>(rc);
        }
    }

    return true;
}

bool FrameSinkIO::isReady() const noexcept
{
    return m_device && m_device->isOpen();
}

void FrameSinkIO::flush()
{
    if (!m_device)
        return;

    // IODevice::flush() is usually bool, but we don't care about the result here ?
    (void)m_device->flush();
}

void FrameSinkIO::close()
{
    if (!m_device)
        return;

    if (m_device->isOpen())
        m_device->closeDevice();
}

IODevice::Ptr FrameSinkIO::device() const noexcept
{
    return m_device;
}

} // namespace job::science::frames
