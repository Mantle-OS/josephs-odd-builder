#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <ggml-opt.h>

#include "job_ggml_enums.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

/*
 * Native members not exposed by the public ggml-opt API:
 *
 * struct ggml_context *ctx
 *     Owned internally by ggml_opt_dataset.
 *     A JobGgmlContext wrapper can be added if upstream exposes it.
 *
 * ggml_backend_buffer_t buf
 *     Owned internally by ggml_opt_dataset.
 *     A JobGgmlBackendBuffer or borrowed buffer view can be added if
 *     upstream exposes it.
 *
 * std::vector<int64_t> permutation
 *     Managed internally by the native dataset.
 *     It controls shard order during shuffle and batch extraction but
 *     cannot currently be inspected through the public API.
*/


namespace job::ggml {
class JobGgmlOptContext;
class JOBGGML_EXPORT JobGgmlOptDataset
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptDataset>;
    using WPtr = std::weak_ptr<JobGgmlOptDataset>;
    using UPtr = std::unique_ptr<JobGgmlOptDataset>;

    explicit JobGgmlOptDataset(
        JobGgmlType dataType,
        JobGgmlType labelType,
        std::int64_t neDatapoint,
        std::int64_t neLabel,
        std::int64_t ndata,
        std::int64_t ndataShard
        );

    ~JobGgmlOptDataset();

    [[nodiscard]] static Ptr createShared(
        JobGgmlType dataType,
        JobGgmlType labelType,
        std::int64_t neDatapoint,
        std::int64_t neLabel,
        std::int64_t ndata,
        std::int64_t ndataShard
        )
    {
        return std::make_shared<JobGgmlOptDataset>(
            dataType,
            labelType,
            neDatapoint,
            neLabel,
            ndata,
            ndataShard
            );
    }

    [[nodiscard]] static UPtr createUniq(
        JobGgmlType dataType,
        JobGgmlType labelType,
        std::int64_t neDatapoint,
        std::int64_t neLabel,
        std::int64_t ndata,
        std::int64_t ndataShard
        )
    {
        return std::make_unique<JobGgmlOptDataset>(
            dataType,
            labelType,
            neDatapoint,
            neLabel,
            ndata,
            ndataShard
            );
    }

    JobGgmlOptDataset(const JobGgmlOptDataset &) = delete;
    JobGgmlOptDataset &operator=(const JobGgmlOptDataset &) = delete;
    JobGgmlOptDataset(JobGgmlOptDataset &&) = delete;
    JobGgmlOptDataset &operator=(JobGgmlOptDataset &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool hasLabels() const noexcept;

    [[nodiscard]] JobGgmlType dataType() const noexcept;
    [[nodiscard]] enum ggml_type ggmlDataType() const noexcept;

    [[nodiscard]] JobGgmlType labelType() const noexcept;
    [[nodiscard]] enum ggml_type ggmlLabelType() const noexcept;

    [[nodiscard]] std::int64_t neDatapoint() const noexcept;
    [[nodiscard]] std::int64_t neLabel() const noexcept;
    [[nodiscard]] std::int64_t ndata() const noexcept;
    [[nodiscard]] std::int64_t ndataShard() const noexcept;
    [[nodiscard]] std::int64_t shardCount() const noexcept;
    [[nodiscard]] std::size_t  nbsData() const noexcept;
    [[nodiscard]] std::size_t  nbsLabels() const noexcept;

    [[nodiscard]] JobGgmlTensor *data() noexcept;
    [[nodiscard]] const JobGgmlTensor *data() const noexcept;

    [[nodiscard]] JobGgmlTensor *labels() noexcept;
    [[nodiscard]] const JobGgmlTensor *labels() const noexcept;

    void shuffle(JobGgmlOptContext &context, std::int64_t idata = -1);

    // dataBatch shape: [ne_datapoint, ndata_batch] || labelsBatch shape: [ne_label, ndata_batch]
    void getBatch(JobGgmlTensor &dataBatch, JobGgmlTensor *labelsBatch, std::int64_t ibatch);
    void getBatchHost(void *dataBatch, std::size_t nbDataBatch, void *labelsBatch, std::int64_t ibatch);


    [[nodiscard]] ggml_opt_dataset_t dataset() noexcept;
    [[nodiscard]] const struct ggml_opt_dataset *dataset() const noexcept;

private:
    [[nodiscard]] static std::size_t shardByteCount(
        std::size_t totalByteCount,
        std::int64_t ndata,
        std::int64_t ndataShard
        );

    [[nodiscard]] static constexpr enum ggml_type toGgmlType(JobGgmlType type) noexcept
    {
        return static_cast<enum ggml_type>(type);
    }

    ggml_opt_dataset_t  m_dataset{nullptr}; // Owned

    JobGgmlTensor::UPtr m_data;
    JobGgmlTensor::UPtr m_labels;

    JobGgmlType         m_dataType{JobGgmlType::F32};
    JobGgmlType         m_labelType{JobGgmlType::F32};

    std::int64_t        m_neDatapoint{0};     // Number of elements in one datapoint.
    std::int64_t        m_neLabel{0};         // Number of elements in one label.
    std::int64_t        m_ndata{0};           // Total number of datapoints in the dataset.
    std::int64_t        m_ndataShard{0};      // Shards
    std::size_t         m_nbsData{0};         // Number of data bytes in one shard.
    std::size_t         m_nbsLabels{0};       // Number of label bytes in one shard.
};

} // namespace job::ggml


