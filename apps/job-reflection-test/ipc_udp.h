#pragma once

#include <cstddef>
#include <cstring>
#include <memory>

#include "packed.h"

class PackedUdp
{
public:
    using Ptr = std::shared_ptr<PackedUdp>;
    using WPtr = std::weak_ptr<PackedUdp>;
    using UPtr = std::unique_ptr<PackedUdp>;

    PackedUdp() = default;
    ~PackedUdp() = default;

    PackedUdp(const PackedUdp &) = delete;
    PackedUdp &operator=(const PackedUdp &) = delete;
    PackedUdp(PackedUdp &&) = delete;
    PackedUdp &operator=(PackedUdp &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<PackedUdp>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<PackedUdp>();
    }

    [[nodiscard]] static const char *data(const Packed &packed) noexcept
    {
        return reinterpret_cast<const char *>(&packed);
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept
    {
        return sizeof(Packed);
    }

    [[nodiscard]] static bool read(const char *data, std::size_t len, Packed &packed) noexcept
    {
        if (!data || len != sizeof(Packed))
            return false;

        std::memcpy(&packed, data, sizeof(Packed));
        return true;
    }
};