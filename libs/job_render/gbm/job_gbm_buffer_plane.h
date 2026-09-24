#pragma once

#include <cstdint>
#include <memory>

#include <gbm.h>

namespace job::render {

class JobGbmBufferPlane
{
public:
    using Ptr  = std::shared_ptr<JobGbmBufferPlane>;
    using WPtr = std::weak_ptr<JobGbmBufferPlane>;
    using UPtr = std::unique_ptr<JobGbmBufferPlane>;

    using Index  = std::uint32_t;
    using Handle = std::uint32_t;
    using Stride = std::uint32_t;
    using Offset = std::uint32_t;

    JobGbmBufferPlane() = default;

    constexpr JobGbmBufferPlane(Index index, Handle handle, Stride stride, Offset offset) noexcept :
        m_index(index),
        m_handle(handle),
        m_stride(stride),
        m_offset(offset)
    {
    }

    ~JobGbmBufferPlane() = default;

    JobGbmBufferPlane(const JobGbmBufferPlane &) = default;
    JobGbmBufferPlane &operator=(const JobGbmBufferPlane &) = default;

    JobGbmBufferPlane(JobGbmBufferPlane &&) noexcept = default;
    JobGbmBufferPlane &operator=(JobGbmBufferPlane &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Index index, Handle handle, Stride stride, Offset offset)
    {
        return std::make_shared<JobGbmBufferPlane>(index, handle, stride, offset);
    }

    [[nodiscard]] static UPtr createUniq(Index index, Handle handle, Stride stride, Offset offset)
    {
        return std::make_unique<JobGbmBufferPlane>(index, handle, stride, offset);
    }

    [[nodiscard]] constexpr Index index() const noexcept
    {
        return m_index;
    }

    [[nodiscard]] constexpr Handle handle() const noexcept
    {
        return m_handle;
    }

    [[nodiscard]] constexpr Stride stride() const noexcept
    {
        return m_stride;
    }

    [[nodiscard]] constexpr Offset offset() const noexcept
    {
        return m_offset;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_handle != 0 && m_stride != 0;
    }

    [[nodiscard]] friend constexpr bool operator==(const JobGbmBufferPlane &, const JobGbmBufferPlane &) = default;

private:
    Index  m_index{};
    Handle m_handle{};
    Stride m_stride{};
    Offset m_offset{};
};

}