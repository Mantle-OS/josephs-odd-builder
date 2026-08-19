#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <concepts>

#include <job_ggml_backend_buffer_type.h>

#include "job_iio_reader.h"
#include "job_iio_writer.h"
#include "job_batch_alloc.h"
#include "job_imem_ctx.h"
#include "job_ai_ctx.h"

namespace job::model {

// The concept defines the requirements that a type T must satisfy
// to be considered a valid JOB memory manager.
template <typename T>
concept CJobIMem = requires(
    T &manager,
    const T &const_manager,
    JobBatchAlloc &batchAlloc,
    JobAiCtx *lctx,
    JobITensorWriter &writer,
    JobITensorReader &reader,
    std::uint32_t nUBatch,
    bool boolean_val,
    std::int32_t int32_val,
    std::uint32_t uint32_val,
    int int_val)
{
    // Required nested types.
    typename T::Ptr;
    typename T::WPtr;
    typename T::UPtr;
    typename T::LayerFilterCallback;
    typename T::LayerReuseCallback;
    typename T::LayerShareCallback;
    typename T::MemoryBreakdown;

    // Initialization and allocation.
    { manager.init(batchAlloc, nUBatch, boolean_val) } -> std::same_as<JobIMemCtx::UPtr>;

    { manager.initFull() } -> std::same_as<JobIMemCtx::UPtr>;

    { manager.initUpdate(lctx, boolean_val) } -> std::same_as<JobIMemCtx::UPtr>;

    // Instance creation.
    { const_manager.createShared() } -> std::same_as<typename T::Ptr>;

    { const_manager.createUniq() } -> std::same_as<typename T::UPtr>;

    // State.
    { const_manager.canShift() } -> std::same_as<bool>;

    { manager.clear(boolean_val) } -> std::same_as<void>;

    // Sequence management.
    { manager.seqRemove(int32_val, int32_val, int32_val) } -> std::same_as<bool>;

    { manager.seqCopy(int32_val, int32_val, int32_val, int32_val) } -> std::same_as<void>;

    { manager.seqKeep(int32_val) } -> std::same_as<void>;

    { manager.seqAdd(int32_val, int32_val, int32_val, int32_val) } -> std::same_as<void>;

    { manager.seqDivide(int32_val, int32_val, int32_val, int_val) } -> std::same_as<void>;

    { const_manager.seqPosMin(int32_val) } -> std::same_as<std::int32_t>;

    { const_manager.seqPosMax(int32_val) } -> std::same_as<std::int32_t>;

    // Diagnostics.
    { const_manager.memBreakdown() } -> std::same_as<typename T::MemoryBreakdown>;

    // Serialization.
    { const_manager.writeState(writer, int32_val, uint32_val) } -> std::same_as<void>;

    { manager.readState(reader, int32_val, uint32_val) } -> std::same_as<void>;
};

} // namespace job::model