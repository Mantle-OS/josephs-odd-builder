#include "token_shm.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <job_logger.h>

namespace job::token {

TokenShm::~TokenShm()
{
    close();
}

bool TokenShm::openProducer(std::string_view shmKey, std::size_t ringBufferSize)
{
    if (shmKey.empty() || ringBufferSize == 0)
        return false;

    if (isConnected()) {
        JOB_LOG_WARN("[TokenShm] Cannot open producer while already connected");
        return false;
    }

    m_key = shmKey;
    m_role = TokenShmRole::Producer;

    m_shm.setKey(m_key);
    m_shm.setSize(ringBufferSize);
    m_shm.setMode(io::SharedMemoryMode::Write);
    m_shm.setNonBlocking(m_nonBlocking);

    if (!m_shm.openDevice()) {
        JOB_LOG_ERROR("[TokenShm] Failed to open producer shared memory '{}'", m_key);
        m_key.clear();
        return false;
    }

    return true;
}

bool TokenShm::openConsumer(
    std::string_view shmKey)
{
    if (shmKey.empty())
        return false;

    if (isConnected()) {
        JOB_LOG_WARN("[TokenShm] Cannot open consumer while already connected");
        return false;
    }

    m_key = shmKey;
    m_role = TokenShmRole::Consumer;

    m_shm.setKey(m_key);
    m_shm.setSize(0);
    m_shm.setMode(io::SharedMemoryMode::Read);
    m_shm.setNonBlocking(m_nonBlocking);

    if (!m_shm.openDevice()) {
        JOB_LOG_ERROR("[TokenShm] Failed to open consumer shared memory '{}'", m_key);
        m_key.clear();
        return false;
    }

    return true;
}

void TokenShm::close()
{
    if (m_shm.isOpen())
        m_shm.closeDevice();

    m_key.clear();
}

bool TokenShm::isConnected() const noexcept
{
    return m_shm.isOpen();
}

void TokenShm::setNonBlocking(bool nonBlocking)
{
    m_nonBlocking = nonBlocking;
    m_shm.setNonBlocking(nonBlocking);
}

bool TokenShm::writeToken(TokenId tokenId)
{
    return writePacket(TokenPacketType::Tokens, &tokenId, sizeof(tokenId));
}

bool TokenShm::writeTokens(std::span<const TokenId> tokens)
{
    if (tokens.empty())
        return true;

    return writePacket(TokenPacketType::Tokens,
                       tokens.data(),
                       tokens.size_bytes());
}

bool TokenShm::writeText(std::string_view text)
{
    if (text.empty())
        return true;

    return writePacket(TokenPacketType::Text,
                       text.data(),
                       text.size());
}

bool TokenShm::writeEos()
{
    return writePacket(TokenPacketType::Eos, nullptr, 0);
}

bool TokenShm::writeReset()
{
    return writePacket(TokenPacketType::Reset, nullptr, 0);
}

bool TokenShm::writeHeartbeat()
{
    return writePacket(TokenPacketType::Heartbeat, nullptr, 0);
}

bool TokenShm::writePacket(TokenPacketType type, const void *data, std::size_t bytes)
{
    if (!isConnected()) {
        JOB_LOG_WARN("[TokenShm] Cannot write packet: not connected");
        return false;
    }

    if (m_role != TokenShmRole::Producer) {
        JOB_LOG_WARN("[TokenShm] Cannot write packet from consumer");
        return false;
    }

    if (bytes > 0 && !data) {
        JOB_LOG_ERROR("[TokenShm] Packet payload pointer is null");
        return false;
    }

    if (bytes > std::numeric_limits<std::uint32_t>::max()) {
        JOB_LOG_ERROR("[TokenShm] Packet payload is too large: {} bytes", bytes);
        return false;
    }

    TokenPacketHeader header{};
    header.type = static_cast<std::uint8_t>(type);
    header.payloadBytes = static_cast<std::uint32_t>(bytes);

    //
    // Build one complete frame before touching the ring.
    //
    // JobSharedMemory::write() is all-or-nothing when enough ring space is
    // available, so this prevents us from successfully writing a header and
    // then discovering there is no room for its payload.
    //
    std::vector<std::uint8_t> frame;
    frame.resize(sizeof(TokenPacketHeader) + bytes);

    std::memcpy(frame.data(), &header, sizeof(header));
    if (bytes > 0) {
        std::memcpy(frame.data() + sizeof(header), data, bytes);
    }

    const ssize_t written = m_shm.write(reinterpret_cast<const char *>(frame.data()), frame.size());

    if (written != static_cast<ssize_t>(frame.size())) {
        if (written < 0)
            JOB_LOG_WARN("[TokenShm] Failed to write packet");
        else
            JOB_LOG_WARN("[TokenShm] Shared memory ring does not have enough space for {} byte packet", frame.size());

        return false;
    }

    return true;
}

std::optional<TokenShm::Packet> TokenShm::readNextPacket()
{
    if (!isConnected()) {
        JOB_LOG_WARN("[TokenShm] Cannot read packet: not connected");
        return std::nullopt;
    }

    if (m_role != TokenShmRole::Consumer) {
        JOB_LOG_WARN("[TokenShm] Cannot read packet from producer");
        return std::nullopt;
    }

    const auto readExact =[this](void *destination, std::size_t bytes) -> bool {
        auto *out = static_cast<std::uint8_t *>(destination);
        std::size_t total{0};
        while (total < bytes) {
            const ssize_t count = m_shm.read(reinterpret_cast<char *>(out + total), bytes - total);
            if (count < 0) {
                if (errno == EAGAIN)
                    return false;

                JOB_LOG_WARN("[TokenShm] Shared memory read failed");
                return false;
            }

            if (count == 0)
                return false;

            total += static_cast<std::size_t>(count);
        }

        return true;
    };

    TokenPacketHeader header{};
    if (!readExact(&header, sizeof(header)))
        return std::nullopt;


    const TokenPacketType type = static_cast<TokenPacketType>(header.type);

    switch (type) {
    case TokenPacketType::Tokens:
    case TokenPacketType::Text:
    case TokenPacketType::Eos:
    case TokenPacketType::Reset:
    case TokenPacketType::Heartbeat:
        break;
    default:
        JOB_LOG_ERROR("[TokenShm] Invalid packet type: {}", header.type);
        return std::nullopt;
    }

    Packet packet;
    packet.type = type;

    if (header.payloadBytes == 0)
        return packet;

    packet.payload.resize(header.payloadBytes);

    if (!readExact(packet.payload.data(), packet.payload.size())) {
        JOB_LOG_ERROR("[TokenShm] Failed to read {} byte packet payload", header.payloadBytes);
        return std::nullopt;
    }

    return packet;
}

ssize_t TokenShm::readTokens(std::span<TokenId> outTokens)
{
    if (outTokens.empty())
        return 0;

    const std::optional<Packet> packet = readNextPacket();
    if (!packet)
        return -1;

    if (packet->type != TokenPacketType::Tokens) {
        JOB_LOG_WARN("[TokenShm] Expected token packet but received type {}", static_cast<std::uint32_t>(packet->type));
        return -1;
    }

    if (packet->payload.size() % sizeof(TokenId) != 0) {
        JOB_LOG_ERROR(
            "[TokenShm] Token packet payload size {} is not aligned "
            "to TokenId size {}",
            packet->payload.size(),
            sizeof(TokenId));

        return -1;
    }

    const std::size_t tokenCount = packet->payload.size() / sizeof(TokenId);
    if (tokenCount > outTokens.size()) {
        JOB_LOG_ERROR("[TokenShm] Output span is too small "
                      "(Need: {}, Have: {})",
                      tokenCount,
                      outTokens.size());

        return -1;
    }

    if (!packet->payload.empty()) {
        std::memcpy(outTokens.data(),
                    packet->payload.data(),
                    packet->payload.size());
    }

    return static_cast<ssize_t>(tokenCount);
}

std::vector<TokenId> TokenShm::readAvailableTokens()
{
    std::vector<TokenId> tokens;

    if (!isConnected() || m_role != TokenShmRole::Consumer)
        return tokens;

    //
    // This method means "what is available right now", so temporarily make
    // the underlying device non-blocking regardless of the normal setting.
    //
    m_shm.setNonBlocking(true);
    while (true) {
        const std::optional<Packet> packet = readNextPacket();
        if (!packet)
            break;

        if (packet->type != TokenPacketType::Tokens)
            continue;

        if (packet->payload.size() % sizeof(TokenId) != 0) {
            JOB_LOG_WARN("[TokenShm] Ignoring malformed token packet");
            continue;
        }

        const std::size_t count = packet->payload.size() / sizeof(TokenId);
        const std::size_t oldSize = tokens.size();
        tokens.resize( oldSize + count);
        if (!packet->payload.empty()) {
            std::memcpy(tokens.data() + oldSize,
                        packet->payload.data(),
                        packet->payload.size());
        }
    }

    m_shm.setNonBlocking(m_nonBlocking);

    return tokens;
}

} // namespace job::token