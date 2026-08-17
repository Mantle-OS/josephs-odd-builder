#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <job_shared_memory.h>

#include "job_token_types.h"
#include "jobtoken_export.h"

namespace job::token {

enum class TokenShmRole : std::uint8_t {
    Producer = 0,
    Consumer
};

enum class TokenPacketType : std::uint8_t {
    Tokens = 1,
    Text,
    Eos,
    Reset,
    Heartbeat
};

struct TokenPacketHeader
{
    std::uint32_t payloadBytes{0};
    std::uint16_t reserved{0};
    std::uint8_t  type{static_cast<std::uint8_t>(TokenPacketType::Tokens)};
    std::uint8_t  flags{0};
};

static_assert(sizeof(TokenPacketHeader) == 8);

class JOBTOKEN_EXPORT TokenShm
{
public:
    using Ptr  = std::shared_ptr<TokenShm>;
    using WPtr = std::weak_ptr<TokenShm>;
    using UPtr = std::unique_ptr<TokenShm>;

    struct Packet
    {
        TokenPacketType type{TokenPacketType::Tokens};
        std::vector<std::uint8_t> payload;
    };

    TokenShm() = default;
    virtual ~TokenShm();

    TokenShm(const TokenShm &) = delete;
    TokenShm &operator=(const TokenShm &) = delete;
    TokenShm(TokenShm &&) noexcept = delete;
    TokenShm &operator=(TokenShm &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<TokenShm>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<TokenShm>();
    }

    [[nodiscard]] bool openProducer(
        std::string_view shmKey,
        std::size_t ringBufferSize = 1024 * 1024);

    [[nodiscard]] bool openConsumer(
        std::string_view shmKey);

    void close();

    [[nodiscard]] bool isConnected() const noexcept;

    [[nodiscard]] TokenShmRole role() const noexcept
    {
        return m_role;
    }

    [[nodiscard]] std::string_view key() const noexcept
    {
        return m_key;
    }

    void setNonBlocking(bool nonBlocking);

    [[nodiscard]] bool writeToken(TokenId tokenId);
    [[nodiscard]] bool writeTokens(std::span<const TokenId> tokens);
    [[nodiscard]] bool writeText(std::string_view text);

    [[nodiscard]] bool writeEos();
    [[nodiscard]] bool writeReset();
    [[nodiscard]] bool writeHeartbeat();

    [[nodiscard]] ssize_t readTokens(
        std::span<TokenId> outTokens);

    [[nodiscard]] std::vector<TokenId> readAvailableTokens();
    [[nodiscard]] std::optional<Packet> readNextPacket();
    [[nodiscard]] const io::JobSharedMemory &device() const noexcept
    {
        return m_shm;
    }

    [[nodiscard]] io::JobSharedMemory &device() noexcept
    {
        return m_shm;
    }

private:
    [[nodiscard]] bool writePacket(TokenPacketType type, const void *data, std::size_t bytes);
    io::JobSharedMemory m_shm;
    std::string         m_key;
    TokenShmRole        m_role{TokenShmRole::Consumer};
    bool                m_nonBlocking{false};
};

} // namespace job::token