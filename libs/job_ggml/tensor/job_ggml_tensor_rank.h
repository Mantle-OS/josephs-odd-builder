#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include <ggml.h>

#include "job_ggml_tensor_extents.h"

namespace job::ggml {

template<std::size_t RankValue>
class JobGgmlTensorRank
{
    static_assert(RankValue > 0 && RankValue <= GGML_MAX_DIMS, "JobGgmlTensorRank requires a rank from 1 through GGML_MAX_DIMS");

public:
    using Ptr  = std::shared_ptr<JobGgmlTensorRank<RankValue>>;
    using UPtr = std::unique_ptr<JobGgmlTensorRank<RankValue>>;

    static constexpr std::size_t Rank = RankValue;

    explicit JobGgmlTensorRank(struct ggml_tensor *tensor) :
        m_tensor{tensor},
        m_extents{JobGgmlTensorExtents::createUniq(tensor)}
    {
        /*
        if (!m_tensor) {
            throw std::invalid_argument{
                "JobGgmlTensorRank requires a valid ggml_tensor"
            };
        }
        */

        if (m_extents->rank() != static_cast<int>(Rank)) {
            throw std::invalid_argument{
                "The GGML tensor does not match the requested tensor rank"
            };
        }
    }

    virtual ~JobGgmlTensorRank() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor)
    {
        return std::make_shared<JobGgmlTensorRank<Rank>>(tensor);
    }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor)
    {
        return std::make_unique<JobGgmlTensorRank<Rank>>(tensor);
    }

    JobGgmlTensorRank(const JobGgmlTensorRank &) = delete;
    JobGgmlTensorRank &operator=(const JobGgmlTensorRank &) = delete;
    JobGgmlTensorRank(JobGgmlTensorRank &&) = delete;
    JobGgmlTensorRank &operator=(JobGgmlTensorRank &&) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_tensor != nullptr &&
               m_extents &&
               m_extents->isValid() &&
               m_extents->rank() == static_cast<int>(Rank);
    }

    [[nodiscard]] static constexpr std::size_t rank() noexcept
    {
        return Rank;
    }

    [[nodiscard]] std::int64_t extent(std::size_t dimension) const noexcept
    {
        if (!m_extents || dimension >= Rank)
            return 0;


        return m_extents->extent(dimension);
    }

    [[nodiscard]] std::size_t stride(std::size_t dimension) const noexcept
    {
        if (!m_extents ||
            dimension >= Rank) {
            return 0;
        }

        return m_extents->stride(dimension);
    }

    [[nodiscard]] std::int64_t volume() const noexcept
    {
        return m_extents ? m_extents->volume() : 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return m_extents ? m_extents->elementCount() : 0;
    }

    [[nodiscard]] std::size_t byteCount() const noexcept
    {
        return m_extents ? m_extents->byteCount() : 0;
    }

    [[nodiscard]] std::size_t paddedByteCount() const noexcept
    {
        return m_extents ? m_extents->paddedByteCount() : 0;
    }

    [[nodiscard]] bool isContiguous() const noexcept
    {
        return m_extents && m_extents->isContiguous();
    }

    [[nodiscard]] bool isTransposed() const noexcept
    {
        return m_extents && m_extents->isTransposed();
    }

    [[nodiscard]] bool isPermuted() const noexcept
    {
        return m_extents && m_extents->isPermuted();
    }

    [[nodiscard]] JobGgmlTensorExtents *extents() noexcept
    {
        return m_extents.get();
    }

    [[nodiscard]] const JobGgmlTensorExtents *extents() const noexcept
    {
        return m_extents.get();
    }

    [[nodiscard]] struct ggml_tensor *tensor() noexcept
    {
        return m_tensor;
    }

    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept
{
        return m_tensor;
    }

protected:
    struct ggml_tensor           *m_tensor{nullptr}; // Borrowed from the owning GGML context.
    JobGgmlTensorExtents::UPtr    m_extents;
};

// These are dumb as we have full classes for them but whatever
using FiberRank     = JobGgmlTensorRank<1>;
using MatrixRank    = JobGgmlTensorRank<2>;
using VolumeRank    = JobGgmlTensorRank<3>;
using BatchRank     = JobGgmlTensorRank<4>;

} // namespace job::ggml