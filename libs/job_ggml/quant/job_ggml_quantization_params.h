#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlQuantizationParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlQuantizationParams>;
    using WPtr = std::weak_ptr<JobGgmlQuantizationParams>;
    using UPtr = std::unique_ptr<JobGgmlQuantizationParams>;

    JobGgmlQuantizationParams() = default;

    explicit JobGgmlQuantizationParams(JobGgmlType type, std::int64_t start = 0, std::int64_t rows = 0, std::int64_t elementsPerRow = 0) noexcept;
    ~JobGgmlQuantizationParams() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlType type = {}, std::int64_t start = 0, std::int64_t rows = 0, std::int64_t elementsPerRow = 0)
    {
        return std::make_shared<JobGgmlQuantizationParams>(type, start, rows, elementsPerRow);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlType type = {}, std::int64_t start = 0, std::int64_t rows = 0, std::int64_t elementsPerRow = 0)
    {
        return std::make_unique<JobGgmlQuantizationParams>(type, start, rows, elementsPerRow);
    }

    JobGgmlQuantizationParams(const JobGgmlQuantizationParams &) = default;
    JobGgmlQuantizationParams &operator=(const JobGgmlQuantizationParams &) = default;
    JobGgmlQuantizationParams(JobGgmlQuantizationParams &&) noexcept = default;
    JobGgmlQuantizationParams &operator=(JobGgmlQuantizationParams &&) noexcept = default;

    [[nodiscard]] JobGgmlType type() const noexcept;
    void setType(JobGgmlType type) noexcept;

    [[nodiscard]] std::int64_t start() const noexcept;
    void setStart(std::int64_t start) noexcept;

    [[nodiscard]] std::int64_t rows() const noexcept;
    void setRows(std::int64_t rows) noexcept;

    [[nodiscard]] std::int64_t elementsPerRow() const noexcept;
    void setElementsPerRow(std::int64_t elementsPerRow) noexcept;

    [[nodiscard]] std::span<const float> importanceMatrix() const noexcept;
    void setImportanceMatrix(std::span<const float> importanceMatrix) noexcept;
    void clearImportanceMatrix() noexcept;

    [[nodiscard]] bool hasImportanceMatrix() const noexcept;

private:
    JobGgmlType             m_type{};
    std::int64_t            m_start{0};
    std::int64_t            m_rows{0};
    std::int64_t            m_elementsPerRow{0};
    std::span<const float>  m_importanceMatrix{};
};

} // namespace job::ggml