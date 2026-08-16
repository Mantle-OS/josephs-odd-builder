#include "formats/tokenizer_shm.h"

#include <cstring>
#include <job_logger.h>

namespace job::token {

TokenizerShm::TokenizerShm() = default;

TokenizerShm::~TokenizerShm()
{
    close();
}

bool TokenizerShm::openProducer(std::string_view shmKey, size_t ringBufferSize)
{
    close();

    if (shmKey.empty() || shmKey[0] != '/') {
        JOB_LOG_ERROR("[TokenizerShm] SHM key must begin with '/' (got '{}')", shmKey);
        return false;
    }

    if (ringBufferSize == 0) {
        JOB_LOG_ERROR("[TokenizerShm] Producer ring buffer size cannot be 0");
        return false;
    }

    m_key = std::string(shmKey);
    m_role = TokenShmRole::Producer;

    m_shm.setKey(m_key);
    m_shm.setSize(ringBufferSize);
    m_shm.setMode(io::SharedMemoryMode::Write);
    m_shm.setNonBlocking(m_nonBlocking);

    if (!m_shm.openDevice()) {
        JOB_LOG_ERROR("[TokenizerShm] Failed to open producer shared memory device on key '{}'", m_key);
        return false;
    }

    JOB_LOG_INFO("[TokenizerShm] Opened producer SHM on '{}' (Buffer: {} KB)", m_key, ringBufferSize / 1024);
    return true;
}

bool TokenizerShm::openConsumer(std::string_view shmKey)
{
    close();

    if (shmKey.empty() || shmKey[0] != '/') {
        JOB_LOG_ERROR("[TokenizerShm] SHM key must begin with '/' (got '{}')", shmKey);
        return false;
    }

    m_key = std::string(shmKey);
    m_role = TokenShmRole::Consumer;

    m_shm.setKey(m_key);
    m_shm.setMode(io::SharedMemoryMode::Read);
    m_shm.setNonBlocking(m_nonBlocking);

    if (!m_shm.openDevice()) {
        JOB_LOG_ERROR("[TokenizerShm] Failed to attach consumer to SHM key '{}'", m_key);
        return false;
    }

    JOB_LOG_INFO("[TokenizerShm] Attached consumer to SHM on '{}' (Total size: {} KB)",
                 m_key, m_shm.size() / 1024);
    return true;
}

void TokenizerShm::close()
{
    if (m_shm.isOpen()) {
        m_shm.closeDevice();
        JOB_LOG_DEBUG("[TokenizerShm] Closed SHM connection on '{}'", m_key);
    }
    m_key.clear();
}

bool TokenizerShm::isConnected() const noexcept
{
    return m_shm.isOpen();
}

void TokenizerShm::setNonBlocking(bool nonBlocking)
{
    m_nonBlocking = nonBlocking;
    m_shm.setNonBlocking(nonBlocking);
}

bool TokenizerShm::writePacket(TokenPacketType type, const void* data, size_t bytes)
{
    if (!isConnected() || m_role != TokenShmRole::Producer) {
        JOB_LOG_ERROR("[TokenizerShm] Cannot write: device not connected as producer");
        return false;
    }

    TokenPacketHeader header{};
    header.type = static_cast<uint8_t>(type);
    header.flags = 0;
    header.reserved = 0;
    header.payloadBytes = static_cast<uint32_t>(bytes);

    size_t totalBytes = sizeof(TokenPacketHeader) + bytes;
    if (m_shm.availableToWrite() < totalBytes) {
        JOB_LOG_WARN("[TokenizerShm] Insufficient ring space to write packet (needs {} bytes, has {})",
                     totalBytes, m_shm.availableToWrite());
        return false;
    }

    // Write header followed by payload
    if (m_shm.write(reinterpret_cast<const char*>(&header), sizeof(header)) != static_cast<ssize_t>(sizeof(header))) {
        JOB_LOG_ERROR("[TokenizerShm] Failed to write packet header to SHM");
        return false;
    }

    if (bytes > 0 && data != nullptr) {
        if (m_shm.write(reinterpret_cast<const char*>(data), bytes) != static_cast<ssize_t>(bytes)) {
            JOB_LOG_ERROR("[TokenizerShm] Failed to write packet payload of {} bytes to SHM", bytes);
            return false;
        }
    }

    return true;
}

bool TokenizerShm::writeToken(int32_t tokenId)
{
    return writeTokens(std::span<const int32_t>(&tokenId, 1));
}

bool TokenizerShm::writeTokens(std::span<const int32_t> tokens)
{
    if (tokens.empty()) return true;
    return writePacket(TokenPacketType::Tokens, tokens.data(), tokens.size_bytes());
}

bool TokenizerShm::writeText(std::string_view text)
{
    if (text.empty()) return true;
    return writePacket(TokenPacketType::Text, text.data(), text.size());
}

bool TokenizerShm::writeEos()
{
    return writePacket(TokenPacketType::Eos, nullptr, 0);
}

bool TokenizerShm::writeReset()
{
    return writePacket(TokenPacketType::Reset, nullptr, 0);
}

std::optional<TokenizerShm::Packet> TokenizerShm::readNextPacket()
{
    if (!isConnected() || m_role != TokenShmRole::Consumer) {
        JOB_LOG_ERROR("[TokenizerShm] Cannot read: device not connected as consumer");
        return std::nullopt;
    }

    if (m_shm.availableToRead() < sizeof(TokenPacketHeader)) {
        if (m_nonBlocking) {
            return std::nullopt;
        }
    }

    TokenPacketHeader header{};
    ssize_t hdrRead = m_shm.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (hdrRead != static_cast<ssize_t>(sizeof(header))) {
        return std::nullopt;
    }

    Packet pkt;
    pkt.type = static_cast<TokenPacketType>(header.type);

    if (header.payloadBytes > 0) {
        pkt.payload.resize(header.payloadBytes);
        size_t totalPayloadRead = 0;

        while (totalPayloadRead < header.payloadBytes) {
            ssize_t bytesRead = m_shm.read(
                reinterpret_cast<char*>(pkt.payload.data() + totalPayloadRead),
                header.payloadBytes - totalPayloadRead);

            if (bytesRead <= 0) {
                JOB_LOG_ERROR("[TokenizerShm] Unexpected EOF or error while reading payload chunk");
                return std::nullopt;
            }
            totalPayloadRead += static_cast<size_t>(bytesRead);
        }
    }

    return pkt;
}

ssize_t TokenizerShm::readTokens(std::span<int32_t> outTokens)
{
    if (outTokens.empty()) return 0;

    auto pkt = readNextPacket();
    if (!pkt.has_value()) {
        return -1;
    }

    if (pkt->type != TokenPacketType::Tokens) {
        JOB_LOG_DEBUG("[TokenizerShm] Received non-token packet type: {}", static_cast<int>(pkt->type));
        return 0;
    }

    size_t incomingTokenCount = pkt->payload.size() / sizeof(int32_t);
    size_t copyCount = std::min(outTokens.size(), incomingTokenCount);

    std::memcpy(outTokens.data(), pkt->payload.data(), copyCount * sizeof(int32_t));
    return static_cast<ssize_t>(copyCount);
}

std::vector<int32_t> TokenizerShm::readAvailableTokens()
{
    std::vector<int32_t> tokens;

    while (m_shm.availableToRead() >= sizeof(TokenPacketHeader)) {
        auto pkt = readNextPacket();
        if (!pkt.has_value()) break;

        if (pkt->type == TokenPacketType::Tokens) {
            size_t count = pkt->payload.size() / sizeof(int32_t);
            const auto* ptr = reinterpret_cast<const int32_t*>(pkt->payload.data());
            tokens.insert(tokens.end(), ptr, ptr + count);
        } else if (pkt->type == TokenPacketType::Eos) {
            break;
        }
    }

    return tokens;
}

} // namespace job::token