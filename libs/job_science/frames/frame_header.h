#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <endian_utils.h>

namespace job::science::frames {

#pragma pack(push, 1)
struct FrameHeader final {
    static constexpr uint32_t kMagic        = 0x50505346;   // 'PPSF'
    static constexpr uint16_t kVersion      = 0x0100;       // format version 1.0
    static constexpr uint32_t kAlignedSize  = 48;           // Aligned size of header + padding

    uint32_t magic{kMagic};
    uint16_t version{kVersion};
    uint16_t headerSize{sizeof(FrameHeader)};
    uint64_t frameId{0};
    uint64_t timestampNs{0};
    uint32_t particleCount{0};
    uint32_t byteLength{0}; // Total size of payload (Config + Particles + Comps)
    uint32_t crc32{0};
    uint32_t reserved{0};

    [[nodiscard]] constexpr size_t totalBytes() const noexcept
    {
        return static_cast<size_t>(byteLength) + kAlignedSize;
    }

    [[nodiscard]] static constexpr FrameHeader create(uint64_t frameId, uint64_t timestampNs,
                                                      uint32_t particleCount, uint32_t byteLength) noexcept
    {
        FrameHeader hdr{};
        hdr.frameId         = frameId;
        hdr.timestampNs     = timestampNs;
        hdr.particleCount   = particleCount;
        hdr.byteLength      = byteLength;
        return hdr;
    }

    // primary check: Only validates fields that !!!!!!!! MUST !!!!!!!!! be correct upon read.
    [[nodiscard]] constexpr bool validateMagicAndSize() const noexcept
    {
        const uint32_t magicLE = core::fromLE32(magic);

        return magicLE == kMagic &&
               version == kVersion &&
               headerSize == sizeof(FrameHeader) &&
               kAlignedSize >= sizeof(FrameHeader);
    }
};
#pragma pack(pop)

static_assert(sizeof(FrameHeader) <= FrameHeader::kAlignedSize, "FrameHeader must remain compact and aligned.");

}
