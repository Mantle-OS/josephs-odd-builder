#include "frames/frame_deserializer.h"

#include <cstring>

#ifdef ENABLE_LOGGING
#include <job_logger.h>
#endif

#include <crc32.h>
#include <endian_utils.h>

namespace job::science::frames {

using namespace job::science::data;

FrameDeserializer::FrameDeserializer(IFrameSource::Ptr source) noexcept :
    m_source(std::move(source))
{
#ifdef ENABLE_LOGGING
    if (!m_source)
        JOB_LOG_WARN("[FrameDeserializer] constructed with null source. readFrame() will fail.");
#endif
}

bool FrameDeserializer::readFrame(DiskModel &diskOut,
                                  Zones     &zonesOut,
                                  Particles &particlesOut)
{
    if (!m_source || !m_source->isReady()) {
#ifdef ENABLE_LOGGING
        JOB_LOG_WARN("[FrameDeserializer] readFrame called but source is not ready");
#endif
        return false;
    }

    // Read header
    auto hdrOpt = m_source->readHeader();
    if (!hdrOpt.has_value()) {
#ifdef ENABLE_LOGGING
        JOB_LOG_WARN("[FrameDeserializer] readHeader() returned no header");
#endif
        return false;
    }

    FrameHeader hdr = *hdrOpt;

    if (!hdr.validateMagicAndSize()) {
#ifdef ENABLE_LOGGING
        JOB_LOG_ERROR("[FrameDeserializer] invalid frame header (magic/size)");
#endif
        return false;
    }

    // decide payload size
    const std::size_t payloadSize = static_cast<std::size_t>(hdr.byteLength);
    if (payloadSize == 0) {
#ifdef ENABLE_LOGGING
        JOB_LOG_ERROR("[FrameDeserializer] header.byteLength is zero");
#endif
        return false;
    }

    std::vector<std::uint8_t> payload;
    payload.resize(payloadSize);

    // read payload in chunks
    std::size_t totalRead = 0;
    while (totalRead < payloadSize) {
        const std::size_t toRead = payloadSize - totalRead;
        const std::size_t got    = m_source->readPayload(payload.data() + totalRead, toRead);

        if (got == 0) {
#ifdef ENABLE_LOGGING
            JOB_LOG_ERROR("[FrameDeserializer] truncated payload: expected {} bytes, got {}",
                          payloadSize, totalRead);
#endif
            return false;
        }
        totalRead += got;
    }

    // 4) CRC check
    const std::uint32_t crcComputed =
        job::core::Crc32::compute(payload.data(), payload.size());

    if (crcComputed != hdr.crc32) {
#ifdef ENABLE_LOGGING
        JOB_LOG_ERROR("[FrameDeserializer] CRC mismatch: header=0x{:08X}, computed=0x{:08X}",
                      hdr.crc32, crcComputed);
#endif
        return false;
    }

    // decode into physics types
    return decodePayload(hdr, payload, diskOut, zonesOut, particlesOut);
}

bool FrameDeserializer::isReady() const noexcept
{
    return m_source && m_source->isReady();
}

void FrameDeserializer::reset()
{
    if (m_source)
        m_source->reset();
}

IFrameSource::Ptr FrameDeserializer::source() const noexcept
{
    return m_source;
}

bool FrameDeserializer::decodePayload(const FrameHeader               &hdr,
                                      const std::vector<std::uint8_t> &payload,
                                      DiskModel                       &diskOut,
                                      Zones                           &zonesOut,
                                      Particles                       &particlesOut)
{
    const std::uint32_t count = hdr.particleCount; // host-endian in current format

    const std::size_t configBytes        = sizeof(DiskModel) + sizeof(Zones);
    const std::size_t particleBytes      = static_cast<std::size_t>(count) * sizeof(Particle);
    const std::size_t expectedPayloadLen = configBytes + particleBytes;

    if (payload.size() != expectedPayloadLen) {
#ifdef ENABLE_LOGGING
        JOB_LOG_ERROR("[FrameDeserializer] expected payload={}B but got={}B (particleCount={})",
                      expectedPayloadLen, payload.size(), count);
#endif
        return false;
    }

    const std::uint8_t *currentPtr = payload.data();

    DiskModel diskTmp{};
    std::memcpy(&diskTmp, currentPtr, sizeof(DiskModel));
    currentPtr += sizeof(DiskModel);

    Zones zonesTmp{};
    std::memcpy(&zonesTmp, currentPtr, sizeof(Zones));
    currentPtr += sizeof(Zones);

    Particles particlesTmp;
    particlesTmp.resize(count);

    if (count > 0)
        std::memcpy(particlesTmp.data(), currentPtr, particleBytes);

    // Commit to outputs only after everything checks out
    diskOut      = diskTmp;
    zonesOut     = zonesTmp;
    particlesOut = std::move(particlesTmp);

    return true;
}

} // namespace job::science::frames
