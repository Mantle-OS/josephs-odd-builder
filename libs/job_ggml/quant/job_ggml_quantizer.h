#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include <ggml.h>

#include "job_ggml_enums.h"
#include "jobggml_export.h"

#include "job_ggml_quantization_params.h"
#include "job_ggml_quantization_result.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlQuantizer
{
public:
    using Ptr  = std::shared_ptr<JobGgmlQuantizer>;
    using WPtr = std::weak_ptr<JobGgmlQuantizer>;
    using UPtr = std::unique_ptr<JobGgmlQuantizer>;

    JobGgmlQuantizer() = default;
    ~JobGgmlQuantizer() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGgmlQuantizer>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGgmlQuantizer>();
    }

    JobGgmlQuantizer(const JobGgmlQuantizer &) = default;
    JobGgmlQuantizer &operator=(const JobGgmlQuantizer &) = default;
    JobGgmlQuantizer(JobGgmlQuantizer &&) noexcept = default;
    JobGgmlQuantizer &operator=(JobGgmlQuantizer &&) noexcept = default;


    static void init(JobGgmlType type); // Upstream Owned
    static void nativeFree() noexcept;  // Upstream Owned

    [[nodiscard]] static bool isQuantized(JobGgmlType type) noexcept;
    [[nodiscard]] static bool requiresImportanceMatrix(JobGgmlType type) noexcept;

    [[nodiscard]] static std::int64_t blockSize(JobGgmlType type) noexcept;
    [[nodiscard]] static std::size_t typeSize(JobGgmlType type) noexcept;
    [[nodiscard]] static std::size_t rowSize(JobGgmlType type, std::int64_t elementsPerRow) noexcept;

    [[nodiscard]] static std::size_t requiredBytes(const JobGgmlQuantizationParams &params) noexcept;

    [[nodiscard]] static bool validate(const JobGgmlQuantizationParams &params,
                                       std::span<const float> source,
                                       std::span<const std::byte> destination) noexcept;

    [[nodiscard]] static JobGgmlQuantizationResult quantizeChunk(const JobGgmlQuantizationParams &params,
                                                                 std::span<const float> source,
                                                                 std::span<std::byte> destination);
};

} // namespace job::ggml