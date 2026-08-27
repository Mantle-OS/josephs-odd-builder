#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <functional>
#include <utility>

#include "packed.h"

class PackedTcpReader
{
public:
    using Callback = std::function<void(const Packed &)>;

    PackedTcpReader() = default;
    ~PackedTcpReader() = default;

    PackedTcpReader(const PackedTcpReader &) = delete;
    PackedTcpReader &operator=(const PackedTcpReader &) = delete;
    PackedTcpReader(PackedTcpReader &&) = delete;
    PackedTcpReader &operator=(PackedTcpReader &&) = delete;

    void setCallback(Callback callback)
    {
        m_callback = std::move(callback);
    }

    void read(const char *data, std::size_t len)
    {
        while (len > 0) {
            const std::size_t remaining = sizeof(Packed) - m_offset;
            const std::size_t count = std::min(remaining, len);

            std::memcpy(m_buffer.data() + m_offset, data, count);

            m_offset += count;
            data += count;
            len -= count;

            if (m_offset != sizeof(Packed))
                continue;

            Packed packed;
            std::memcpy(&packed, m_buffer.data(), sizeof(Packed));
            m_offset = 0;

            if (m_callback)
                m_callback(packed);
        }
    }

    [[nodiscard]] static const char *data(const Packed &packed) noexcept
    {
        return reinterpret_cast<const char *>(&packed);
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept
    {
        return sizeof(Packed);
    }

private:
    std::array<char, sizeof(Packed)> m_buffer{};
    std::size_t m_offset{0};
    Callback m_callback;
};