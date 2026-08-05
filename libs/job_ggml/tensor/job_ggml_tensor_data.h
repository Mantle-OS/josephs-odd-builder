#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>

#include <ggml.h>
#include <ggml-backend.h>
#include <ggml-cpu.h>

#include "job_ggml_enums.h"
#include "job_ggml_type_traits.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorData
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorData>;
    using UPtr = std::unique_ptr<JobGgmlTensorData>;

    explicit JobGgmlTensorData(struct ggml_tensor *tensor);
    ~JobGgmlTensorData() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensorData>(tensor); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensorData>(tensor); }

    JobGgmlTensorData(const JobGgmlTensorData &) = delete;
    JobGgmlTensorData &operator=(const JobGgmlTensorData &) = delete;
    JobGgmlTensorData(JobGgmlTensorData &&) = delete;
    JobGgmlTensorData &operator=(JobGgmlTensorData &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    // Type information
    [[nodiscard]] JobGgmlType type() const noexcept;
    [[nodiscard]] enum ggml_type ggmlType() const noexcept;

    [[nodiscard]] const char *typeName() const noexcept;

    [[nodiscard]] std::int64_t blockSize() const noexcept;
    [[nodiscard]] std::size_t typeSize() const noexcept;
    [[nodiscard]] std::size_t rowSize() const noexcept;

    [[nodiscard]] bool isQuantized() const noexcept;

    [[nodiscard]] JobGgmlTypeTraits::UPtr typeTraits() const;

    // Native storage association
    [[nodiscard]] ggml_backend_buffer_t buffer() const noexcept;
    [[nodiscard]] ggml_backend_buffer_type_t bufferType() const noexcept;

    [[nodiscard]] bool hasBuffer() const noexcept;
    [[nodiscard]] bool bufferIsHost() const noexcept;

    // Tensor data pointer
    [[nodiscard]] void *data() noexcept;
    [[nodiscard]] const void *data() const noexcept;

    [[nodiscard]] float *dataF32() noexcept;
    [[nodiscard]] const float *dataF32() const noexcept;

    [[nodiscard]] bool hasData() const noexcept;

    // Tensor flags
    [[nodiscard]] std::int32_t flags() const noexcept;
    void setFlags(std::int32_t flags) noexcept;

    [[nodiscard]] bool hasFlag(JobGgmlTensorFlag flag) const noexcept;
    void addFlag(JobGgmlTensorFlag flag) noexcept;
    void removeFlag(JobGgmlTensorFlag flag) noexcept;

    [[nodiscard]] bool isInput() const noexcept;
    [[nodiscard]] bool isOutput() const noexcept;
    [[nodiscard]] bool isParameter() const noexcept;
    [[nodiscard]] bool isLoss() const noexcept;
    [[nodiscard]] bool isCompute() const noexcept;

    // Backend-specific extension storage
    [[nodiscard]] void *extra() noexcept;
    [[nodiscard]] const void *extra() const noexcept;
    [[nodiscard]] bool hasExtra() const noexcept;

    /*
     * CPU/host tensor access.
     *
     * These methods use helpers exported by ggml-cpu.h and require tensor
     * storage that is directly accessible from the host. Use JobGgmlBackend
     * transfer operations for CUDA, Vulkan, or other device-managed storage.
     */
    [[nodiscard]] bool isHostAccessible() const noexcept;
    // s(igneed)32 int access
    [[nodiscard]] std::int32_t valueI32(std::int64_t index) const;
    void setValueI32(std::int64_t index, std::int32_t value);
    void fillI32(std::int32_t value);
    [[nodiscard]] std::int32_t valueI32(std::int64_t i0, std::int64_t i1, std::int64_t i2, std::int64_t i3) const;
    void setValueI32(std::int64_t i0, std::int64_t i1, std::int64_t i2, std::int64_t i3, std::int32_t value);
    // 32 float
    [[nodiscard]] float valueF32(std::int64_t index) const;
    void setValueF32(std::int64_t index, float value);
    void fillF32(float value);
    [[nodiscard]] float valueF32(std::int64_t i0, std::int64_t i1, std::int64_t i2, std::int64_t i3) const;
    void setValueF32(std::int64_t i0, std::int64_t i1, std::int64_t i2, std::int64_t i3, float value);
    // end ggml-cpu.h


    [[nodiscard]] struct ggml_tensor *tensor() noexcept;
    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept;

    [[nodiscard]] static constexpr JobGgmlType fromGgmlType(enum ggml_type type) noexcept { return static_cast<JobGgmlType>(type); }

    [[nodiscard]] static constexpr enum ggml_type toGgmlType(JobGgmlType type) noexcept { return static_cast<enum ggml_type>(type); }

private:
    [[nodiscard]] bool validElementIndex(std::int64_t index) const noexcept;
    [[nodiscard]] bool validElementCoordinates(std::int64_t i0, std::int64_t i1, std::int64_t i2, std::int64_t i3) const noexcept; // helper for the ggml-cpu calls
    struct ggml_tensor *m_tensor{nullptr}; // Borrowed from the owning GGML context.
};

} // namespace job::ggml