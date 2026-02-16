#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>

// job::net
#include <servers/tcp_server.h>
#include <servers/udp_server.h>
#include <servers/unix_socket_server.h>

#include "frames/frame_header.h"
#include "frames/iframe_sink.h"

namespace job::science::frames {
using job::net::ISocketIO;

class FrameSinkNet : public IFrameSink {
public:
    using Ptr = std::shared_ptr<FrameSinkNet>;

    explicit FrameSinkNet(ISocketIO::Ptr socket, bool autoOpen = true) noexcept;
    ~FrameSinkNet() override = default;

    [[nodiscard]] bool open();
    void close();

    [[nodiscard]] bool isReady() const noexcept override;

    [[nodiscard]] bool writeFrame(const FrameHeader &header, const std::uint8_t *data, std::size_t payloadSize) override;
    void flush() override;

    // Accessors
    [[nodiscard]] ISocketIO::Ptr socket() const noexcept;

private:
    ISocketIO::Ptr      m_socket;
    bool                m_autoOpen{true};
    std::atomic<bool>   m_open{false};
    mutable std::mutex  m_mutex;
};

} // namespace job::science::frames
