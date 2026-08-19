#pragma once

#include <cstdint>
#include <memory>

#include "job_ubatch.h"

namespace job::model {

class JobIMemCtx
{
public:
    using Ptr  = std::shared_ptr<JobIMemCtx>;
    using WPtr = std::weak_ptr<JobIMemCtx>;
    using UPtr = std::unique_ptr<JobIMemCtx>;

    enum class JobMemStatus : uint32_t
    {
        Success = 0,
        NoUpdate,
        PrepareError,
        ComputeError
    };

    JobIMemCtx() = default;
    virtual ~JobIMemCtx() = default;

    [[nodiscard]] virtual Ptr createShared() const = 0;
    [[nodiscard]] virtual UPtr createUniq() const = 0;

    JobIMemCtx(const JobIMemCtx &) = delete;
    JobIMemCtx &operator=(const JobIMemCtx &) = delete;
    JobIMemCtx(JobIMemCtx &&) noexcept = delete;
    JobIMemCtx &operator=(JobIMemCtx &&) noexcept = delete;

    // Return false if we are done.
    [[nodiscard]] virtual bool next() = 0;

    // Apply the memory state for the current ubatch to the memory object.
    // Return false on failure.
    [[nodiscard]] virtual bool apply() = 0;

    // Get the current ubatch.
    [[nodiscard]] virtual const JobUBatch &uBatch() const = 0;

    // Get the status of the memory context.
    // Used for error handling and checking if any updates would be applied.
    [[nodiscard]] virtual JobMemStatus status() const = 0;
};

} // namespace job::model