#include "frames/frame_sink_net.h"

#include <job_logger.h>
#include <isocket_io.h>

namespace job::science::frames {

using job::net::ISocketIO;

FrameSinkNet::FrameSinkNet(Socket socket, bool autoOpen) noexcept :
    m_socket(std::move(socket)),
    m_autoOpen(autoOpen)
{
    // We *could* auto-open here, but we keep side-effects out of ctor.
}

bool FrameSinkNet::open()
{
    // If we've already deemed this sink "open", just report success.
    if (m_open.load(std::memory_order_acquire))
        return true;

    std::scoped_lock lock(m_mutex);

    if (!m_socket) {
        JOB_LOG_ERROR("[FrameSinkNet] open() called with null socket");
        return false;
    }

    using State = ISocketIO::SocketState;
    const auto state = m_socket->state();

    // We consider these "good enough" to send bytes:
    // - Connected: normal client socket
    // - Listening / Bound: in case you're using a per-connection session sink,
    //   you should be passing that instead, but we won't over-police here.
    switch (state) {
    case State::Connected:
    case State::Listening:
    case State::Bound:
        m_open.store(true, std::memory_order_release);
        return true;

    default:
        break;
    }

    // We *could* try autoOpen logic here (e.g. reconnect), but we don't know
    // the URL/endpoint. The higher-level client/server owns connection policy.
    if (m_autoOpen) {
        JOB_LOG_WARN("[FrameSinkNet] autoOpen requested, but socket state={} "
                     "and no connection policy is implemented here",
                     static_cast<int>(state));
    }

    return false;
}

void FrameSinkNet::close()
{
    // We *do not* own the socket's lifetime policy; caller decides whether to
    // disconnect()/destroy it. We just mark this sink as closed.
    m_open.store(false, std::memory_order_release);
}

bool FrameSinkNet::isReady() const noexcept
{
    return m_open.load(std::memory_order_acquire);
}

// Small helper to write all bytes or fail.
// NOTE: This assumes ISocketIO::write is reasonably POSIX-like:
//   - returns >0 on bytes written
//   - returns 0 or <0 on error / closed / would-block
static bool writeAll(ISocketIO &sock, const std::uint8_t *data, std::size_t len)
{
    std::size_t total = 0;

    while (total < len) {
        const auto written = sock.write(data + total, len - total);
        if (written <= 0)
            return false;

        total += static_cast<std::size_t>(written);
    }
    return true;
}

bool FrameSinkNet::writeFrame(const FrameHeader  &header,
                              const std::uint8_t *data,
                              std::size_t         payloadSize)
{
    if (!isReady()) {
        // Try to open lazily if autoOpen is set.
        if (!open())
            return false;
    }

    if (!m_socket)
        return false;

    if (!data && payloadSize != 0)
        return false;

    std::scoped_lock lock(m_mutex);

    auto *sock = m_socket.get();
    const auto *hdrBytes = reinterpret_cast<const std::uint8_t*>(&header);
    constexpr std::size_t hdrSize = sizeof(FrameHeader);

    // 1) Header
    if (!writeAll(*sock, hdrBytes, hdrSize)) {
        JOB_LOG_ERROR("[FrameSinkNet] Failed to write frame header");
        return false;
    }

    // 2) Payload
    if (payloadSize > 0 && data) {
        if (!writeAll(*sock, data, payloadSize)) {
            JOB_LOG_ERROR("[FrameSinkNet] Failed to write frame payload ({} bytes)", payloadSize);
            return false;
        }
    }

    return true;
}

void FrameSinkNet::flush()
{
    // Sockets typically don't expose a flush primitive; sending is enough.
    // If later you wrap a buffered sink on top, *that* sink's flush() will
    // do the real work.
}

} // namespace job::science::frames
