#include "frame_source_net.h"

#include <algorithm>
#include <cstring>

// job::core
#include <job_logger.h>


namespace job::science::frames {


FrameSourceNet::FrameSourceNet(ISocketIO::Ptr socket) noexcept :
    m_socket(std::move(socket))
{
}

bool FrameSourceNet::isReady() const noexcept
{
    if (!m_socket)
        return false;

    const auto st = m_socket->state();
    using S = ISocketIO::SocketState;

    // "Good enough" notion of readable:
    // - Connected: TCP/Unix connections
    // - Bound/Listening: UDP/Unix servers
    // Everything else is "nope".
    return st == S::Connected ||
           st == S::Bound ||
           st == S::Listening;
}

std::optional<FrameHeader> FrameSourceNet::readHeader()
{
    if (!isReady())
        return std::nullopt;

    std::uint8_t raw[FrameHeader::kAlignedSize];

    if (!readExact(raw, FrameHeader::kAlignedSize))
        return std::nullopt; // EOF / disconnect / error

    FrameHeader hdr{};
    static_assert(sizeof(FrameHeader) <= FrameHeader::kAlignedSize,
                  "FrameHeader larger than kAlignedSize; wire format mismatch");

    std::memcpy(&hdr, raw, sizeof(FrameHeader));

    if (!hdr.validateMagicAndSize()) {
        JOB_LOG_ERROR("[FrameSourceNet] Invalid frame header (magic/size mismatch). "
                      "magic=0x{:08x}, version=0x{:04x}, headerSize={}, byteLength={}",
                      hdr.magic, hdr.version, hdr.headerSize, hdr.byteLength);
        m_lastHeader.reset();
        m_payloadRemaining = 0;
        return std::nullopt;
    }

    m_lastHeader       = hdr;
    m_payloadRemaining = hdr.byteLength;

    return hdr;
}

std::size_t FrameSourceNet::readPayload(std::uint8_t *dst, std::size_t maxSize)
{
    if (!dst || maxSize == 0)
        return 0;

    if (!m_lastHeader.has_value() || m_payloadRemaining == 0)
        return 0; // no active frame or already finished

    const std::size_t toRead = std::min<std::size_t>(maxSize, m_payloadRemaining);
    if (toRead == 0)
        return 0;

    if (!readExact(dst, toRead)) {
        JOB_LOG_ERROR("[FrameSourceNet] Failed while reading payload (truncated frame?)");
        m_lastHeader.reset();
        m_payloadRemaining = 0;
        return 0;
    }

    m_payloadRemaining -= toRead;

    if (m_payloadRemaining == 0)
        m_lastHeader.reset();

    return toRead;
}

void FrameSourceNet::reset()
{
    // Logical reset of framing state; underlying socket lifetime is owned elsewhere.
    m_lastHeader.reset();
    m_payloadRemaining = 0;
}

bool FrameSourceNet::readExact(std::uint8_t *dst, std::size_t size)
{
    if (!m_socket)
        return false;

    std::size_t total = 0;

    while (total < size) {
        const std::size_t want = size - total;
        const ssize_t n = m_socket->read(dst + total, want);

        if (n < 0) {
            JOB_LOG_ERROR("[FrameSourceNet] read() returned error (errno-like: {})",
                          static_cast<int>(m_socket->lastError()));
            return false;
        }

        if (n == 0) {
            // Remote closed or EOF; not enough bytes to satisfy request.
            return false;
        }

        total += static_cast<std::size_t>(n);
    }

    return true;
}

} // namespace job::science::frames
