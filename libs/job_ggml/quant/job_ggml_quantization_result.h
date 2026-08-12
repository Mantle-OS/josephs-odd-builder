#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlQuantizationResult
{
public:
    using Ptr  = std::shared_ptr<JobGgmlQuantizationResult>;
    using WPtr = std::weak_ptr<JobGgmlQuantizationResult>;
    using UPtr = std::unique_ptr<JobGgmlQuantizationResult>;

    JobGgmlQuantizationResult() = default;
    explicit JobGgmlQuantizationResult(JobGgmlType type, std::size_t bytesWritten = 0, std::int64_t rowsProcessed = 0) noexcept;
    ~JobGgmlQuantizationResult() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlType type = {}, std::size_t bytesWritten = 0, std::int64_t rowsProcessed = 0)
    {
        return std::make_shared<JobGgmlQuantizationResult>(type, bytesWritten, rowsProcessed);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlType type = {}, std::size_t bytesWritten = 0, std::int64_t rowsProcessed = 0)
    {
        return std::make_unique<JobGgmlQuantizationResult>(type, bytesWritten, rowsProcessed);
    }

    JobGgmlQuantizationResult(const JobGgmlQuantizationResult &) = default;
    JobGgmlQuantizationResult &operator=(const JobGgmlQuantizationResult &) = default;
    JobGgmlQuantizationResult(JobGgmlQuantizationResult &&) noexcept = default;
    JobGgmlQuantizationResult &operator=(JobGgmlQuantizationResult &&) noexcept = default;

    [[nodiscard]] JobGgmlType type() const noexcept;
    void setType(JobGgmlType type) noexcept;

    [[nodiscard]] std::size_t bytesWritten() const noexcept;
    void setBytesWritten(std::size_t bytesWritten) noexcept;

    [[nodiscard]] std::int64_t rowsProcessed() const noexcept;
    void setRowsProcessed(std::int64_t rowsProcessed) noexcept;

private:
    JobGgmlType     m_type{};
    std::size_t     m_bytesWritten{0};
    std::int64_t    m_rowsProcessed{0};
};

} // namespace job::ggml