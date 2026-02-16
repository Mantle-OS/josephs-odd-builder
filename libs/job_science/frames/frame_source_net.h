#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

// job::net
#include <isocket_io.h>

#include "frames/iframe_source.h"
#include "frames/frame_header.h"

namespace job::science::frames {
using job::net::ISocketIO;
class FrameSourceNet final : public IFrameSource {
public:
    using Ptr    = std::shared_ptr<FrameSourceNet>;
    explicit FrameSourceNet(ISocketIO::Ptr socket) noexcept;
    [[nodiscard]] bool isReady() const noexcept override;
    [[nodiscard]] std::optional<FrameHeader> readHeader() override;
    [[nodiscard]] std::size_t readPayload(std::uint8_t *dst,
                                          std::size_t maxSize) override;
    void reset() override;

    [[nodiscard]] ISocketIO::Ptr socket() const noexcept
    {
        return m_socket;
    }

private:
    [[nodiscard]] bool readExact(std::uint8_t *dst, std::size_t size);

    ISocketIO::Ptr              m_socket;
    std::optional<FrameHeader>  m_lastHeader;
    std::size_t                 m_payloadRemaining{0};
};

} // namespace job::science::frames
