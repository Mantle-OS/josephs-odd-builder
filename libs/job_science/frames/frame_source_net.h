#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "frames/iframe_source.h"
#include "frames/frame_header.h"

namespace job::net {
class ISocketIO;
}

namespace job::science::frames {

// Stream frames from a network socket (TCP / UDP / Unix).
class FrameSourceNet final : public IFrameSource {
public:
    using Ptr    = std::shared_ptr<FrameSourceNet>;
    using Socket = std::shared_ptr<job::net::ISocketIO>;

    explicit FrameSourceNet(Socket socket) noexcept;

    [[nodiscard]] bool isReady() const noexcept override;
    [[nodiscard]] std::optional<FrameHeader> readHeader() override;
    [[nodiscard]] std::size_t readPayload(std::uint8_t *dst,
                                          std::size_t maxSize) override;
    void reset() override;

    [[nodiscard]] Socket socket() const noexcept
    {
        return m_socket;
    }

private:
    [[nodiscard]] bool readExact(std::uint8_t *dst, std::size_t size);

    Socket                      m_socket;
    std::optional<FrameHeader>  m_lastHeader;
    std::size_t                 m_payloadRemaining{0};
};

} // namespace job::science::frames
