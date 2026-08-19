#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <map>

#include <job_ggml_backend_buffer_type.h>

#include "job_iio_reader.h"
#include "job_iio_writer.h"

#include "job_batch_alloc.h"
#include "job_imem_ctx.h"

#include "job_ai_ctx.h"

namespace job::model {

class JobIMem {
    using Ptr                   = std::shared_ptr<JobIMem>;
    using WPtr                  = std::weak_ptr<JobIMem>;
    using UPtr                  = std::unique_ptr<JobIMem>;
    using LayerFilterCallback   = std::function<bool(std::int32_t il)>;
    using LayerReuseCallback    = std::function<std::int32_t(std::int32_t il)>;
    using LayerShareCallback    = std::function<std::int32_t(std::int32_t il)>;
    using MemBreakdown          = std::map<ggml::JobGgmlBackendBufferType, std::size_t>;

    virtual ~JobIMem() = default;
    virtual JobIMemCtx::UPtr init(JobBatchAlloc &batchAlloc,
                                 std::uint32_t nUBatch,
                                 bool embdAll) = 0;

    [[nodiscard]] virtual Ptr createShared() const = 0;
    [[nodiscard]] virtual UPtr createUniq() const = 0;

    JobIMem(const JobIMem &) = delete;
    JobIMem &operator=(const JobIMem &) = delete;

    JobIMem(JobIMem &&) noexcept = delete;
    JobIMem &operator=(JobIMem &&) noexcept = delete;

    // simulate full cache, used for allocating worst-case compute buffers
    virtual JobIMemCtx::UPtr initFull() = 0;

    // prepare for any pending memory updates, such as shifts, copies, etc. status == JobMemStatus::NoUpdate if there is nothing to update
    virtual JobIMemCtx::UPtr initUpdate(JobAiCtx *lctx, bool optimize) = 0;

    virtual bool canShift() const = 0;

    // if data == true, the data buffers will also be cleared together with the metadata
    virtual void clear(bool data) = 0;

    virtual bool seqRemove(std::int32_t seqId,
                       std::int32_t p0,
                       std::int32_t p1) = 0;

    virtual void seqCopy(std::int32_t seqIdSrc,
                       std::int32_t seqIdDst,
                       std::int32_t p0,
                       std::int32_t p1) = 0;

    virtual void seqKeep(std::int32_t seqId) = 0;

    virtual void seqAdd(std::int32_t seqId,
                        std::int32_t p0,
                        std::int32_t p1,
                        std::int32_t shift) = 0;

    virtual void seqDivide(std::int32_t seqId,
                        std::int32_t p0,
                        std::int32_t p1, int d) = 0;

    virtual std::int32_t seqPosMin(std::int32_t seqId) const = 0;
    virtual std::int32_t seqPosMax(std::int32_t seqId) const = 0;

    // ggml_backend_buffer_type_t not sure if we should use a View here
    virtual MemBreakdown memBreakdown() const = 0;

    virtual void writeState(JobITensorWriter &io,
                             std::int32_t seqId = -1,
                             std::uint32_t flags = 0) const = 0;

    virtual void readState(JobITensorReader &io,
                           std::int32_t seqId = -1,
                           std::uint32_t flags = 0) = 0;
};
}

