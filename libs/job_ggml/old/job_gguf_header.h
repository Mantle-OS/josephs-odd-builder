#pragma once
#include <cstdint>
// job::core
#include <endian_utils.h>

// [  4 bytes  ][ 4 bytes ][8 bytes][   8 bytes  ] HEADER STOPS HERE   [     ? meta data size ?    ][     tensor info with count ?        ][      rest of file     ]
// [ magic num ][ version ][ count ][ ?kv count? ] HEADER STOPS HERE   [ KV struct{arch, name etc}  ][ tensor info{n_dim, dim,type,offset} ][Quantized weights till off set ? ]

namespace job::ggml {
#pragma pack(push, 1)
struct alignas(32) JobGgufHeader {
    // static
    static constexpr uint32_t kGgufHeaderMagic      = 0x46554747;   // "GGUF" in Little Endian
    static constexpr uint32_t kGgufHeaderVersion    = 3;            // version 3

    // ctor
    JobGgufHeader() = default;
    JobGgufHeader(const JobGgufHeader&)            = delete;
    JobGgufHeader &operator=(const JobGgufHeader&) = delete;
    JobGgufHeader(JobGgufHeader&&)                 = delete;
    JobGgufHeader &operator=(JobGgufHeader&&)      = delete;

    // members
    // start 4 byte
    uint32_t magic{kGgufHeaderMagic};
    uint32_t version{kGgufHeaderVersion};
    // start 8 bytes
    uint64_t tensor_count{0};
    uint64_t kv_count{0};
    uint64_t padding{0};

    // Utils
    [[nodiscard]] constexpr std::size_t tensorAndMetaCount() const noexcept
    {
        return tensor_count + kv_count;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        const uint32_t magicNative   = core::fromLE32(magic);
        const uint32_t versionNative = core::fromLE32(version);
        return (magicNative == kGgufHeaderMagic) && (versionNative == kGgufHeaderVersion);
    }

    static void create(JobGgufHeader &header, uint64_t tensor_count = 0 , uint32_t kv_count = 0) noexcept
    {
        header.magic                = kGgufHeaderMagic;
        header.version              = kGgufHeaderVersion;
        header.tensor_count         = tensor_count;
        header.kv_count             = kv_count;
        header.padding              = 0;
    }
};
#pragma pack(pop)
static_assert(sizeof(JobGgufHeader) == 32, "Header must be exactly 32 bytes");
}
