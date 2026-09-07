#include "frame_source_net.h"

#include <algorithm>
#include <cstring>

#include <poll.h>

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

    if (size == 0)
        return true;

    if (!dst)
        return false;

    constexpr int kReadableTimeoutMs = 5000;

    std::size_t total = 0;

    while (total < size) {
        const std::size_t want = size - total;
        const net::NetIoResult n = m_socket->read(dst + total, want);

        if (n.ok()) {
            // Ok does not mean complete: a short read is normal on a stream.
            total += n.bytes;
            continue;
        }

        if (n.closed()) {
            // Remote closed; not enough bytes to satisfy the request.
            JOB_LOG_DEBUG("[FrameSourceNet] peer closed with {} of {} bytes read", total, size);
            return false;
        }

        if (n.failed()) {
            JOB_LOG_ERROR("[FrameSourceNet] read() returned error (errno-like: {})",
                          static_cast<int>(m_socket->lastError()));
            return false;
        }

        /*
         * WouldBlock: the socket is non-blocking and the rest of the frame has
         * not arrived. Wait for readability rather than spinning -- the old
         * `n == 0` branch treated this as EOF and abandoned a frame that was
         * merely split across packets.
         */
        pollfd pfd{};
        pfd.fd = m_socket->fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        const int ready = ::poll(&pfd, 1, kReadableTimeoutMs);

        if (ready <= 0) {
            JOB_LOG_ERROR("[FrameSourceNet] timed out waiting for {} more bytes", size - total);
            return false;
        }

        if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
            return false;
    }

    return true;
}

} // namespace job::science::frames
