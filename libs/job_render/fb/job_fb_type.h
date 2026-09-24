#pragma once

#include <cstdint>

#include <linux/fb.h>

namespace job::render {

enum class FbType : std::uint32_t
{
    PackedPixels      = FB_TYPE_PACKED_PIXELS,
    Planes            = FB_TYPE_PLANES,
    InterleavedPlanes = FB_TYPE_INTERLEAVED_PLANES,
    Text              = FB_TYPE_TEXT,
    VgaPlanes         = FB_TYPE_VGA_PLANES,
    FourCc            = FB_TYPE_FOURCC
};

class JobFbType
{
public:
    using Value = std::uint32_t;

    constexpr JobFbType() = default;

    explicit constexpr JobFbType(Value value) noexcept :
        m_value(value)
    {
    }

    explicit constexpr JobFbType(FbType type) noexcept :
        m_value(static_cast<Value>(type))
    {
    }

    ~JobFbType() = default;

    JobFbType(const JobFbType &) = default;
    JobFbType &operator=(const JobFbType &) = default;

    JobFbType(JobFbType &&) noexcept = default;
    JobFbType &operator=(JobFbType &&) noexcept = default;

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr bool is(FbType type) const noexcept
    {
        return m_value == static_cast<Value>(type);
    }

    [[nodiscard]] constexpr bool packedPixels() const noexcept
    {
        return is(FbType::PackedPixels);
    }

    [[nodiscard]] constexpr bool planes() const noexcept
    {
        return is(FbType::Planes);
    }

    [[nodiscard]] constexpr bool interleavedPlanes() const noexcept
    {
        return is(FbType::InterleavedPlanes);
    }

    [[nodiscard]] constexpr bool text() const noexcept
    {
        return is(FbType::Text);
    }

    [[nodiscard]] constexpr bool vgaPlanes() const noexcept
    {
        return is(FbType::VgaPlanes);
    }

    [[nodiscard]] constexpr bool fourCc() const noexcept
    {
        return is(FbType::FourCc);
    }

    [[nodiscard]] constexpr bool known() const noexcept
    {
        return packedPixels() || planes() || interleavedPlanes() || text() || vgaPlanes() || fourCc();
    }

    [[nodiscard]] friend constexpr bool operator==(const JobFbType &, const JobFbType &) noexcept = default;

private:
    Value m_value{};
};

}