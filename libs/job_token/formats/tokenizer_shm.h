#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <job_shared_memory.h>


#include "jobtoken_export.h"

namespace job::token {

enum class TokenShmRole : uint8_t {
    Producer = 0, // Writes tokens / commands to SHM
    Consumer      // Reads tokens / commands from SHM
};

enum class TokenPacketType : uint8_t {
    Tokens = 1,  // Stream of int32_t token IDs
    Text,        // Raw UTF-8 string chunk
    Eos,         // End of generation / sequence sentinel
    Reset,       // Reset consumer context / clear state
    Heartbeat    // Connection keep-alive
};

#pragma pack(push, 1)
struct TokenPacketHeader {
    uint8_t  type{static_cast<uint8_t>(TokenPacketType::Tokens)};
    uint8_t  flags{0};
    uint16_t reserved{0};
    uint32_t payloadBytes{0};
};
#pragma pack(pop)

static_assert(sizeof(TokenPacketHeader) == 8, "TokenPacketHeader must be exactly 8 bytes");

class JOBTOKEN_EXPORT TokenizerShm {
public:
    TokenizerShm();
    ~TokenizerShm();

    TokenizerShm(const TokenizerShm&) = delete;
    TokenizerShm& operator=(const TokenizerShm&) = delete;
    TokenizerShm(TokenizerShm&&) noexcept = default;
    TokenizerShm& operator=(TokenizerShm&&) noexcept = default;

    // Open as producer (creates SHM segment and ring buffer)
    [[nodiscard]] bool openProducer(
        std::string_view shmKey,
        size_t ringBufferSize = 1024 * 1024);

    // Open as consumer (attaches to existing SHM segment)
    [[nodiscard]] bool openConsumer(std::string_view shmKey);

    void close();

    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] TokenShmRole role() const noexcept { return m_role; }
    [[nodiscard]] std::string_view key() const noexcept { return m_key; }

    void setNonBlocking(bool nonBlocking);

    [[nodiscard]] bool writeToken(int32_t tokenId);
    [[nodiscard]] bool writeTokens(std::span<const int32_t> tokens);
    [[nodiscard]] bool writeText(std::string_view text);
    [[nodiscard]] bool writeEos();
    [[nodiscard]] bool writeReset();

    // Reads raw tokens into caller-provided buffer. Returns count of tokens read, or -1 on error.
    [[nodiscard]] ssize_t readTokens(std::span<int32_t> outTokens);

    // Reads all currently pending tokens into a vector
    [[nodiscard]] std::vector<int32_t> readAvailableTokens();

    // Polls next packet frame. Returns std::nullopt if non-blocking and empty.
    struct Packet {
        TokenPacketType      type{TokenPacketType::Tokens};
        std::vector<uint8_t> payload;
    };
    [[nodiscard]] std::optional<Packet> readNextPacket();

    // Access underlying IO device for diagnostics
    [[nodiscard]] const io::JobSharedMemory& device() const noexcept { return m_shm; }
    [[nodiscard]] io::JobSharedMemory& device() noexcept { return m_shm; }

private:
    [[nodiscard]] bool writePacket(TokenPacketType type, const void* data, size_t bytes);

private:
    io::JobSharedMemory m_shm;
    std::string         m_key;
    TokenShmRole        m_role{TokenShmRole::Consumer};
    bool                m_nonBlocking{false};
};

} // namespace job::token