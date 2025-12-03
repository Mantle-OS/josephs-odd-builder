#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>

#include <servers/tcp_server.h>
#include <servers/udp_server.h>
#include <servers/unix_socket_server.h>

#include "frames/frame_header.h"
#include "frames/iframe_sink.h"

namespace job::net {
    class ISocketIO;
}

namespace job::science::frames {
class FrameSinkNet : public IFrameSink {
public:
    using Ptr = std::shared_ptr<FrameSinkNet>;
    using Socket = std::shared_ptr<job::net::ISocketIO>;

    explicit FrameSinkNet(Socket socket, bool autoOpen = true) noexcept;
    ~FrameSinkNet() override = default;

    [[nodiscard]] bool open();
    void close();

    [[nodiscard]] bool isReady() const noexcept override;

    [[nodiscard]] bool writeFrame(const FrameHeader &header, const std::uint8_t *data, std::size_t payloadSize) override;
    void flush() override;

    // Accessors
    [[nodiscard]] Socket socket() const noexcept
    {
        return m_socket;
    }

private:
    Socket              m_socket;
    bool                m_autoOpen{true};
    std::atomic<bool>   m_open{false};
    mutable std::mutex  m_mutex;
};

} // namespace job::science::frames
