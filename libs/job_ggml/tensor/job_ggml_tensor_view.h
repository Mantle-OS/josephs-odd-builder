#pragma once

#include <cstddef>
#include <memory>

#include <ggml.h>

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorView
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorView>;
    using UPtr = std::unique_ptr<JobGgmlTensorView>;

    explicit JobGgmlTensorView(struct ggml_tensor *tensor);
    ~JobGgmlTensorView() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensorView>(tensor); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) {return std::make_unique<JobGgmlTensorView>(tensor);}
    JobGgmlTensorView(const JobGgmlTensorView &) = delete;
    JobGgmlTensorView &operator=(const JobGgmlTensorView &) = delete;
    JobGgmlTensorView(JobGgmlTensorView &&) = delete;
    JobGgmlTensorView &operator=(JobGgmlTensorView &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isView() const noexcept;

    [[nodiscard]] struct ggml_tensor *source() noexcept;
    [[nodiscard]] const struct ggml_tensor *source() const noexcept;

    [[nodiscard]] std::size_t offset() const noexcept;

    [[nodiscard]] struct ggml_tensor *rootSource() noexcept;
    [[nodiscard]] const struct ggml_tensor *rootSource() const noexcept;

    [[nodiscard]] std::size_t depth() const noexcept;

    [[nodiscard]] bool hasSource(const struct ggml_tensor *tensor) const noexcept;
    [[nodiscard]] struct ggml_tensor *tensor() noexcept;
    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept;

private:
    static constexpr std::size_t MaxViewDepth = 64;
    struct ggml_tensor *m_tensor{nullptr}; // Borrowed from the owning GGML context.
};

} // namespace job::ggml