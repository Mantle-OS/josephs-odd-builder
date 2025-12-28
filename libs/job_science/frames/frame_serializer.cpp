#include "frames/frame_serializer.h"

#include <array>
#include <chrono>
#include <cstring>

#include <job_logger.h>

#include <crc32.h>
#include <endian_utils.h>

namespace job::science::frames {

using namespace job::science::data;

FrameSerializer::FrameSerializer(IFrameSink::Ptr sink) noexcept :
    m_sink(std::move(sink))
{

    if (!m_sink)
        JOB_LOG_WARN("[FrameSerializer] constructed with null sink. writeFrame() will fail.");
}

bool FrameSerializer::writeFrame(std::uint64_t    frameId,
                                 const Particles &particles,
                                 const DiskModel &diskModel,
                                 const Zones     &zones)
{
    if (!m_sink || !m_sink->isReady()) {
        JOB_LOG_WARN("[FrameSerializer] writeFrame called with no open sink");
        return false;
    }

    if (particles.empty())
        return false;

    const std::uint64_t timestampNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    const std::size_t configBytes   = sizeof(DiskModel) + sizeof(Zones);
    const std::size_t particleBytes = particles.size() * sizeof(Particle);
    const std::uint32_t totalPayloadSize =
        static_cast<std::uint32_t>(configBytes + particleBytes);

    std::vector<std::uint8_t> buffer;
    buffer.resize(totalPayloadSize);

    std::uint8_t *currentPtr = buffer.data();

    std::memcpy(currentPtr, &diskModel, sizeof(DiskModel));
    currentPtr += sizeof(DiskModel);

    std::memcpy(currentPtr, &zones, sizeof(Zones));
    currentPtr += sizeof(Zones);

    std::memcpy(currentPtr, particles.data(), particleBytes);

    FrameHeader header = FrameHeader::create(
        frameId,
        timestampNs,
        static_cast<std::uint32_t>(particles.size()),
        totalPayloadSize);

    // Magic is stored little-endian on disk
    header.magic = job::core::toLE32(FrameHeader::kMagic);

    // CRC over the payload
    header.crc32 = job::core::Crc32::compute(
        reinterpret_cast<const std::uint8_t *>(buffer.data()),
        static_cast<std::size_t>(header.byteLength));

    // Let the sink deal with *how* this goes out (file, socket, shm, etc.)
    return m_sink->writeFrame(header, buffer.data(), buffer.size());
}

bool FrameSerializer::isOpen() const noexcept
{
    return m_sink && m_sink->isReady();
}

void FrameSerializer::flush()
{
    if (m_sink)
        m_sink->flush();
}

} // namespace job::science::frames
