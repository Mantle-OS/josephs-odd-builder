#include "frame_source_io.h"

#include <algorithm>
#include <cstring>

#include <job_logger.h>

namespace job::science::frames {

using namespace job::core;

FrameSourceIO::FrameSourceIO(IODevice::Ptr dev, bool autoOpen) noexcept
    : m_device(std::move(dev))
    , m_autoOpen(autoOpen)
{
    if (m_autoOpen && m_device) {
        if (m_device->openDevice()) {
            m_open.store(true, std::memory_order_release);
        } else {
            JOB_LOG_WARN("[FrameSourceIO] autoOpen is true but openDevice() failed");
        }
    }
}

bool FrameSourceIO::open()
{
    if (!m_device) {
        JOB_LOG_ERROR("[FrameSourceIO] open() called with null device");
        return false;
    }

    bool alreadyOpen = m_open.load(std::memory_order_acquire);
    if (alreadyOpen && m_device->isOpen())
        return true;

    if (!m_device->openDevice()) {
        JOB_LOG_ERROR("[FrameSourceIO] openDevice() failed");
        m_open.store(false, std::memory_order_release);
        return false;
    }

    m_open.store(true, std::memory_order_release);
    return true;
}

void FrameSourceIO::close()
{
    if (!m_device)
        return;

    if (!m_open.exchange(false, std::memory_order_acq_rel))
        return;

    m_device->closeDevice();
    m_lastHeader.reset();
    m_payloadRemaining = 0;
}

bool FrameSourceIO::isReady() const noexcept
{
    return m_device && m_open.load(std::memory_order_acquire) && m_device->isOpen();
}

std::optional<FrameHeader> FrameSourceIO::readHeader()
{
    // Lazy-open if requested.
    if (!isReady()) {
        if (!m_autoOpen || !open())
            return std::nullopt;
    }

    // Read the aligned header blob from the device.
    std::uint8_t raw[FrameHeader::kAlignedSize];

    if (!readExact(raw, FrameHeader::kAlignedSize))
        return std::nullopt; // EOF or error

    FrameHeader hdr{};
    static_assert(sizeof(FrameHeader) <= FrameHeader::kAlignedSize,
                  "FrameHeader larger than kAlignedSize; wire format mismatch");

    std::memcpy(&hdr, raw, sizeof(FrameHeader));

    if (!hdr.validateMagicAndSize()) {
        JOB_LOG_ERROR("[FrameSourceIO] Invalid frame header (magic/size mismatch). "
                      "magic=0x{:08x}, version=0x{:04x}, headerSize={}, byteLength={}",
                      hdr.magic, hdr.version, hdr.headerSize, hdr.byteLength);
        return std::nullopt;
    }

    // Prepare internal state for payload reads.
    m_lastHeader       = hdr;
    m_payloadRemaining = hdr.byteLength;

    return hdr;
}

std::size_t FrameSourceIO::readPayload(std::uint8_t *dst, std::size_t maxSize)
{
    if (!dst || maxSize == 0)
        return 0;

    if (!m_lastHeader.has_value() || m_payloadRemaining == 0)
        return 0; // no active frame or already consumed

    const std::size_t toRead = std::min<std::size_t>(maxSize, m_payloadRemaining);
    if (toRead == 0)
        return 0;

    if (!readExact(dst, toRead)) {
        // Treat any failure here as “frame truncated”.
        JOB_LOG_ERROR("[FrameSourceIO] Failed while reading payload (truncated frame?)");
        m_lastHeader.reset();
        m_payloadRemaining = 0;
        return 0;
    }

    m_payloadRemaining -= toRead;
    if (m_payloadRemaining == 0) {
        // We’ve finished this frame’s payload; caller may check CRC elsewhere.
        m_lastHeader.reset();
    }

    return toRead;
}

void FrameSourceIO::reset()
{
    // Logical reset of frame state. We *don’t* rewind the underlying device
    // because IODevice doesn’t expose seek/rewind generically.
    m_lastHeader.reset();
    m_payloadRemaining = 0;
}

IODevice::Ptr FrameSourceIO::device() const noexcept
{
    return m_device;
}

bool FrameSourceIO::readExact(std::uint8_t *dst, std::size_t size)
{
    std::size_t total = 0;

    while (total < size) {
        if (!m_device || !m_device->isOpen()) {
            m_open.store(false, std::memory_order_release);
            return false;
        }

        const std::size_t want = size - total;
        const ssize_t n = m_device->read(reinterpret_cast<char *>(dst + total), want);

        if (n < 0) {
            JOB_LOG_ERROR("[FrameSourceIO] read() returned error");
            return false;
        }

        if (n == 0) {
            // EOF before we got all bytes.
            return false;
        }

        total += static_cast<std::size_t>(n);
    }

    return true;
}

} // namespace job::science::frames
