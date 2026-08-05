#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <gguf.h>

#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgufTensorInfo
{
public:
    using Ptr  = std::shared_ptr<JobGgufTensorInfo>;
    using WPtr = std::weak_ptr<JobGgufTensorInfo>;
    using UPtr = std::unique_ptr<JobGgufTensorInfo>;

    explicit JobGgufTensorInfo(const JobGgmlTensor &tensor, std::uint64_t offset);

    ~JobGgufTensorInfo() = default;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] std::string name() const;
    void setName(const std::string &name);

    [[nodiscard]] JobGgmlType type() const noexcept;
    [[nodiscard]] enum ggml_type ggmlType() const noexcept;
    void setType(JobGgmlType type);
    void setGgmlType(enum ggml_type type);

    [[nodiscard]] int rank() const noexcept;

    [[nodiscard]] std::int64_t extent(std::size_t dimension) const noexcept;

    [[nodiscard]] std::size_t stride(std::size_t dimension) const noexcept;

    [[nodiscard]] std::int64_t elementCount() const noexcept;
    [[nodiscard]] std::size_t byteCount() const noexcept;
    [[nodiscard]] std::size_t paddedByteCount(std::size_t alignment) const noexcept;

    [[nodiscard]] bool isQuantized() const noexcept;

    [[nodiscard]] std::uint64_t offset() const noexcept;
    void setOffset(std::uint64_t offset) noexcept;

    [[nodiscard]] bool isAligned(std::size_t alignment) const noexcept;

    void setTensor(const JobGgmlTensor &tensor);
    void reset() noexcept;

private:
    [[nodiscard]] bool validDimension(std::size_t dimension) const noexcept;

    struct ggml_tensor m_tensor{};
    std::uint64_t      m_offset{0};
};

} // namespace job::ggml