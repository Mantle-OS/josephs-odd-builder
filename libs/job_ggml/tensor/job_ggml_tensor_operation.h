#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

#include <ggml.h>

#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorOperation
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorOperation>;
    using UPtr = std::unique_ptr<JobGgmlTensorOperation>;

    static constexpr std::size_t MaxSources     = GGML_MAX_SRC;
    static constexpr std::size_t ParameterBytes = GGML_MAX_OP_PARAMS;
    static constexpr std::size_t ParameterCount = GGML_MAX_OP_PARAMS / sizeof(std::int32_t);

    explicit JobGgmlTensorOperation(struct ggml_tensor *tensor);
    ~JobGgmlTensorOperation() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensorOperation>(tensor); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensorOperation>(tensor); }

    JobGgmlTensorOperation(const JobGgmlTensorOperation &) = delete;
    JobGgmlTensorOperation &operator=(const JobGgmlTensorOperation &) = delete;
    JobGgmlTensorOperation(JobGgmlTensorOperation &&) = delete;
    JobGgmlTensorOperation &operator=(JobGgmlTensorOperation &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] JobGgmlOp operation() const noexcept;
    [[nodiscard]] enum ggml_op ggmlOperation() const noexcept;

    [[nodiscard]] bool hasOperation() const noexcept;
    [[nodiscard]] bool isOperation(JobGgmlOp operation) const noexcept;

    [[nodiscard]] const char *operationName() const noexcept;
    [[nodiscard]] const char *operationSymbol() const noexcept;

    [[nodiscard]] std::int32_t flags() const noexcept;

    [[nodiscard]] std::size_t sourceCount() const noexcept;
    [[nodiscard]] bool hasSources() const noexcept;

    [[nodiscard]] struct ggml_tensor *source(std::size_t index) noexcept;
    [[nodiscard]] const struct ggml_tensor *source(std::size_t index) const noexcept;
    [[nodiscard]] bool hasSource(const struct ggml_tensor *tensor) const noexcept;

    [[nodiscard]] std::array<struct ggml_tensor *, MaxSources> sources() noexcept;
    [[nodiscard]] std::array<const struct ggml_tensor *, MaxSources> sources() const noexcept;

    [[nodiscard]] std::int32_t parameter(std::size_t index) const noexcept;
    [[nodiscard]] std::array<std::int32_t, ParameterCount> parameters() const noexcept;
    [[nodiscard]] const std::int32_t *parameterData() const noexcept;
    [[nodiscard]] std::size_t parameterCount() const noexcept;
    [[nodiscard]] std::size_t parameterByteSize() const noexcept;

    template<typename T>
    [[nodiscard]] bool readParameter(std::size_t byteOffset, T &value) const noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "GGML operation parameters must be trivially copyable");

        if (!m_tensor || byteOffset > ParameterBytes ||
            sizeof(T) > ParameterBytes - byteOffset) {
            return false;
        }

        std::memcpy(
            &value,
            reinterpret_cast<const std::byte *>(m_tensor->op_params) + byteOffset,
            sizeof(T)
        );

        return true;
    }

    [[nodiscard]] JobGgmlUnaryOp unaryOperation() const noexcept;
    [[nodiscard]] JobGgmlGluOp gluOperation() const noexcept;

    [[nodiscard]] bool isUnaryOperation() const noexcept;
    [[nodiscard]] bool isGluOperation() const noexcept;

    [[nodiscard]] struct ggml_tensor *tensor() noexcept;
    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept;

private:
    [[nodiscard]] bool validSourceIndex(std::size_t index) const noexcept;

    struct ggml_tensor *m_tensor{nullptr}; // Borrowed from the owning GGML context.
};

} // namespace job::ggml