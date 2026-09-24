#pragma once

#include <cstddef>
#include <cstdint>

namespace job::render {

class JobRenderBufferPlane
{
public:
    JobRenderBufferPlane() = default;

    constexpr JobRenderBufferPlane(std::uint32_t index, std::size_t stride, std::size_t offset) noexcept :
        m_index(index),
        m_stride(stride),
        m_offset(offset)
    {
    }

    ~JobRenderBufferPlane() = default;

    JobRenderBufferPlane(const JobRenderBufferPlane &) = default;
    JobRenderBufferPlane &operator=(const JobRenderBufferPlane &) = default;

    JobRenderBufferPlane(JobRenderBufferPlane &&) noexcept = default;
    JobRenderBufferPlane &operator=(JobRenderBufferPlane &&) noexcept = default;

    [[nodiscard]] constexpr std::uint32_t index() const noexcept
    {
        return m_index;
    }

    [[nodiscard]] constexpr std::size_t stride() const noexcept
    {
        return m_stride;
    }

    [[nodiscard]] constexpr std::size_t offset() const noexcept
    {
        return m_offset;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_stride != 0;
    }

private:
    std::uint32_t m_index{};
    std::size_t m_stride{};
    std::size_t m_offset{};
};

}