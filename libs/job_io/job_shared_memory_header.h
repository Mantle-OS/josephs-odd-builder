#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>

#include <endian_utils.h>

namespace job::threads {
struct alignas(64)  JobSharedMemoryHeader {
    // static vars
    static constexpr uint32_t       kSmMagic            = 0x534D454D;           // "SMEM" (shared memory)
    static constexpr uint16_t       kSmVersion          = 0x0100;               // 1.0
    static constexpr uint32_t       kSmAlignedSize      = 48;                   // Aligned size of header + padding
    static constexpr std::size_t    kSmMaxHeaderSize    = 64;                   // Max size of the header
    // ctor
    JobSharedMemoryHeader() = default;
    JobSharedMemoryHeader(const JobSharedMemoryHeader&)            = delete;
    JobSharedMemoryHeader &operator=(const JobSharedMemoryHeader&) = delete;
    JobSharedMemoryHeader(JobSharedMemoryHeader&&)                 = delete;
    JobSharedMemoryHeader &operator=(JobSharedMemoryHeader&&)      = delete;
    // members
    uint32_t                        magic{kSmMagic};                            // magic numbner LE32
    uint16_t                        version{kSmVersion};                        // version of the header
    uint16_t                        headerSize{sizeof(JobSharedMemoryHeader)};  // The size of this header
    uint64_t                        headerId{0};                                // The shm cookie
    uint32_t                        totalSize{0};                               // Total usable bytes in the ring buffer region *after* this header. actual shared memory size should be: sizeof(JobSharedMemoryHeader) + totalSize.
    std::atomic<std::size_t>        writePos{0};                                // index where the producer will write next byte.
    std::atomic<std::size_t>        readPos{0};                                 // index where the consumer will read next byte.
    uint32_t                        padding{0};                                 // padding for the header to get to 64

    // Utils
    [[nodiscard]] constexpr size_t totalBytes() const noexcept
    {
        return static_cast<size_t>(totalSize) + kSmAlignedSize;
    }

    [[nodiscard]] constexpr bool validSharedMemMagicAndSize() const noexcept
    {
        const std::uint32_t magicLE = core::fromLE32(magic);
        return magicLE == kSmMagic &&
               version == kSmVersion &&
               headerSize == sizeof(JobSharedMemoryHeader) &&
               sizeof(JobSharedMemoryHeader) <= kSmMaxHeaderSize &&
               totalSize > 0;
    }

    static void create(JobSharedMemoryHeader &header, uint64_t headerId, uint32_t ringBytes) noexcept
    {
        header.magic        = kSmMagic;
        header.version      = kSmVersion;
        header.headerSize   = static_cast<std::uint16_t>(sizeof(JobSharedMemoryHeader));
        header.headerId     = headerId;
        header.totalSize    = ringBytes;
        header.writePos.store(0, std::memory_order_relaxed);
        header.readPos.store(0,std::memory_order_relaxed);
        header.padding      = 0;
    }
};
static_assert(sizeof(JobSharedMemoryHeader) <= JobSharedMemoryHeader::kSmMaxHeaderSize,
              "JobSharedMemoryHeader must remain compact and aligned. (<= 64 bytes)");
}

