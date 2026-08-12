#include "job_ggml_opt_dataset.h"

#include <limits>
#include <stdexcept>
#include <utility>

#include "job_ggml_opt_context.h"
#include "job_ggml_enums.h"

namespace job::ggml {

JobGgmlOptDataset::JobGgmlOptDataset(JobGgmlType dataType,
                                     JobGgmlType labelType,
                                     std::int64_t neDatapoint,
                                     std::int64_t neLabel,
                                     std::int64_t ndata,
                                     std::int64_t ndataShard) :
    m_dataType{dataType},
    m_labelType{labelType},
    m_neDatapoint{neDatapoint},
    m_neLabel{neLabel},
    m_ndata{ndata},
    m_ndataShard{ndataShard}
{
    const enum ggml_type nativeDataType  = toGgmlType(m_dataType);
    const enum ggml_type nativeLabelType = toGgmlType(m_labelType);

    if (!isValidGgmlType(nativeDataType))
        throw std::invalid_argument{ "JobGgmlOptDataset received an invalid data type" };

    if (m_neLabel > 0 && !isValidGgmlType(nativeLabelType))
        throw std::invalid_argument{ "JobGgmlOptDataset received an invalid label type" };

    if (m_neDatapoint <= 0)
        throw std::invalid_argument{ "JobGgmlOptDataset neDatapoint must be greater than zero" };

    if (m_neLabel < 0)
        throw std::invalid_argument{ "JobGgmlOptDataset neLabel must be greater than or equal to zero" };

    if (m_ndata <= 0)
        throw std::invalid_argument{ "JobGgmlOptDataset ndata must be greater than zero" };

    if (m_ndataShard <= 0)
        throw std::invalid_argument{ "JobGgmlOptDataset ndataShard must be greater than zero" };

    if (m_ndataShard > m_ndata)
        throw std::invalid_argument{ "JobGgmlOptDataset ndataShard cannot exceed ndata" };

    if (m_ndata % m_ndataShard != 0) {
        throw std::invalid_argument{
            "JobGgmlOptDataset ndata must be divisible by ndataShard"
        };
    }

    const std::int64_t dataBlockSize = ggml_blck_size(nativeDataType);
    if (dataBlockSize <= 0 || m_neDatapoint % dataBlockSize != 0)
        throw std::invalid_argument{ "JobGgmlOptDataset neDatapoint is not divisible by the data type block size" };

    if (m_neLabel > 0) {
        const std::int64_t labelBlockSize = ggml_blck_size(nativeLabelType);

        if (labelBlockSize <= 0 || m_neLabel % labelBlockSize != 0)
            throw std::invalid_argument{ "JobGgmlOptDataset neLabel is not divisible by the label type block size" };
    }

    ggml_opt_dataset_t nativeDataset = ggml_opt_dataset_init(nativeDataType,
                                                             nativeLabelType,
                                                             m_neDatapoint,
                                                             m_neLabel,
                                                             m_ndata,
                                                             m_ndataShard);

    if (!nativeDataset)
        throw std::runtime_error{ "Failed to initialize the GGML optimization dataset" };

    try {
        struct ggml_tensor *nativeData = ggml_opt_dataset_data(nativeDataset);

        if (!nativeData)
            throw std::runtime_error{ "GGML optimization dataset did not create its data tensor" };

        auto data = JobGgmlTensor::createUniq(nativeData);
        JobGgmlTensor::UPtr labels;

        if (m_neLabel > 0) {
            struct ggml_tensor *nativeLabels = ggml_opt_dataset_labels(nativeDataset);
            if (!nativeLabels)
                throw std::runtime_error{ "GGML optimization dataset did not create its labels tensor" };

            labels = JobGgmlTensor::createUniq(nativeLabels);
        }

        const std::size_t nbsData = shardByteCount(data->byteCount(),
                                                   m_ndata,
                                                   m_ndataShard);

        std::size_t nbsLabels = 0;

        if (labels) {
            nbsLabels = shardByteCount(labels->byteCount(),
                                       m_ndata,
                                       m_ndataShard);
        }

        m_dataset   = nativeDataset;
        m_data      = std::move(data);
        m_labels    = std::move(labels);
        m_nbsData   = nbsData;
        m_nbsLabels = nbsLabels;
    } catch (...) {
        ggml_opt_dataset_free(nativeDataset);
        throw;
    }
}

JobGgmlOptDataset::~JobGgmlOptDataset()
{
    /*
     * The wrappers borrow native tensors owned by m_dataset. Destroy the
     * wrappers before freeing the native dataset.
     */
    m_labels.reset();
    m_data.reset();

    if (m_dataset) {
        ggml_opt_dataset_free(m_dataset);
        m_dataset = nullptr;
    }
}

bool JobGgmlOptDataset::isValid() const noexcept
{
    if (!m_dataset ||
        !m_data ||
        !m_data->isValid()) {
        return false;
    }

    if (m_neDatapoint <= 0 ||
        m_neLabel < 0 ||
        m_ndata <= 0 ||
        m_ndataShard <= 0 ||
        m_ndataShard > m_ndata ||
        m_ndata % m_ndataShard != 0) {
        return false;
    }

    if (ggml_opt_dataset_ndata(m_dataset) != m_ndata)
        return false;

    if (ggml_opt_dataset_data(m_dataset) != m_data->tensor())
        return false;

    if (m_neLabel == 0)
        return !m_labels && ggml_opt_dataset_labels(m_dataset) == nullptr && m_nbsLabels == 0;

    return m_labels && m_labels->isValid() && ggml_opt_dataset_labels(m_dataset) == m_labels->tensor() && m_nbsLabels > 0;
}

bool JobGgmlOptDataset::hasLabels() const noexcept
{
    return m_dataset &&
           m_labels &&
           m_labels->isValid() &&
           ggml_opt_dataset_labels(m_dataset) != nullptr;
}

JobGgmlType JobGgmlOptDataset::dataType() const noexcept
{
    return m_dataType;
}

enum ggml_type JobGgmlOptDataset::ggmlDataType() const noexcept
{
    return toGgmlType(m_dataType);
}

JobGgmlType JobGgmlOptDataset::labelType() const noexcept
{
    return m_labelType;
}

enum ggml_type JobGgmlOptDataset::ggmlLabelType() const noexcept
{
    return toGgmlType(m_labelType);
}

std::int64_t JobGgmlOptDataset::neDatapoint() const noexcept
{
    return m_neDatapoint;
}

std::int64_t JobGgmlOptDataset::neLabel() const noexcept
{
    return m_neLabel;
}

std::int64_t JobGgmlOptDataset::ndata() const noexcept
{
    return m_dataset ? ggml_opt_dataset_ndata(m_dataset) : 0;
}

std::int64_t JobGgmlOptDataset::ndataShard() const noexcept
{
    return m_ndataShard;
}

std::int64_t JobGgmlOptDataset::shardCount() const noexcept
{
    if (m_ndata <= 0 || m_ndataShard <= 0)
        return 0;

    return m_ndata / m_ndataShard;
}

std::size_t JobGgmlOptDataset::nbsData() const noexcept
{
    return m_nbsData;
}

std::size_t JobGgmlOptDataset::nbsLabels() const noexcept
{
    return m_nbsLabels;
}

JobGgmlTensor *JobGgmlOptDataset::data() noexcept
{
    return m_data.get();
}

const JobGgmlTensor *JobGgmlOptDataset::data() const noexcept
{
    return m_data.get();
}

JobGgmlTensor *JobGgmlOptDataset::labels() noexcept
{
    return m_labels.get();
}

const JobGgmlTensor *JobGgmlOptDataset::labels() const noexcept
{
    return m_labels.get();
}

void JobGgmlOptDataset::shuffle(JobGgmlOptContext &context, std::int64_t idata)
{
    if (!isValid())
        throw std::runtime_error { "Cannot shuffle an invalid GGML optimization dataset" };

    // even with the include file here ... incomplete types
    if (!context.isValid())
        throw std::invalid_argument{ "JobGgmlOptDataset::shuffle requires a valid optimization context" };

    if (idata > m_ndata)
        throw std::out_of_range{ "JobGgmlOptDataset shuffle range exceeds ndata" };

    if (idata >= 0 && idata % m_ndataShard != 0)
        throw std::invalid_argument{ "JobGgmlOptDataset shuffle range must be divisible by ndataShard" };

    // even with the include file here ... incomplete types
    ggml_opt_dataset_shuffle(context.context(), m_dataset, idata);
}

void JobGgmlOptDataset::getBatch(JobGgmlTensor &dataBatch,
                                 JobGgmlTensor *labelsBatch,
                                 std::int64_t ibatch)
{
    if (!isValid())
        throw std::runtime_error{ "Cannot read a batch from an invalid GGML optimization dataset" };

    if (!dataBatch.isValid())
        throw std::invalid_argument{ "JobGgmlOptDataset::getBatch requires a valid data batch tensor" };

    if (!dataBatch.isContiguous())
        throw std::invalid_argument{ "JobGgmlOptDataset data batch tensor must be contiguous" };

    if (toGgmlType(dataBatch.type()) != ggmlDataType())
        throw std::invalid_argument{ "JobGgmlOptDataset data batch type does not match the dataset" };

    if (dataBatch.extent(0) != m_neDatapoint)
        throw std::invalid_argument{ "JobGgmlOptDataset data batch extent does not match neDatapoint" };

    if (hasLabels() != (labelsBatch != nullptr))
        throw std::invalid_argument{ "JobGgmlOptDataset labels batch presence does not match the dataset" };

    if (labelsBatch) {
        if (!labelsBatch->isValid())
            throw std::invalid_argument{ "JobGgmlOptDataset received an invalid labels batch tensor" };

        if (!labelsBatch->isContiguous())
            throw std::invalid_argument{ "JobGgmlOptDataset labels batch tensor must be contiguous" };

        if (toGgmlType(labelsBatch->type()) != ggmlLabelType())
            throw std::invalid_argument{ "JobGgmlOptDataset labels batch type does not match the dataset" };

        if (labelsBatch->extent(0) != m_neLabel)
            throw std::invalid_argument{ "JobGgmlOptDataset labels batch extent does not match neLabel" };
    }

    if (m_nbsData == 0 || dataBatch.byteCount() % m_nbsData != 0)
        throw std::invalid_argument{ "JobGgmlOptDataset data batch size is not divisible by nbsData" };

    const std::size_t shardsPerBatch = dataBatch.byteCount() / m_nbsData;
    if (shardsPerBatch == 0)
        throw std::invalid_argument{ "JobGgmlOptDataset data batch must contain at least one shard" };

    if (labelsBatch) {
        if (m_nbsLabels == 0)
            throw std::runtime_error{ "JobGgmlOptDataset has invalid label shard metadata" };

        if (shardsPerBatch > std::numeric_limits<std::size_t>::max() / m_nbsLabels)
            throw std::overflow_error{ "JobGgmlOptDataset label batch byte count overflowed" };

        const std::size_t expectedLabelBytes = shardsPerBatch * m_nbsLabels;
        if (labelsBatch->byteCount() != expectedLabelBytes)
            throw std::invalid_argument{ "JobGgmlOptDataset labels batch size does not match the data batch" };
    }

    if (ibatch < 0)
        throw std::out_of_range{ "JobGgmlOptDataset batch index cannot be negative" };

    const std::size_t batchIndex = static_cast<std::size_t>(ibatch);
    const std::size_t totalShards = static_cast<std::size_t>(shardCount());
    if (batchIndex > std::numeric_limits<std::size_t>::max() / shardsPerBatch || (batchIndex + 1) * shardsPerBatch > totalShards)
        throw std::out_of_range{ "JobGgmlOptDataset batch extends beyond the dataset" };

    ggml_opt_dataset_get_batch(m_dataset,
                               dataBatch.tensor(),
                               labelsBatch ? labelsBatch->tensor() : nullptr,
                               ibatch);
}

void JobGgmlOptDataset::getBatchHost(void *dataBatch,
                                     std::size_t nbDataBatch,
                                     void *labelsBatch,
                                     std::int64_t ibatch)
{
    if (!isValid())
        throw std::runtime_error{ "Cannot read a host batch from an invalid GGML optimization dataset" };

    if (!dataBatch)
        throw std::invalid_argument{ "JobGgmlOptDataset::getBatchHost requires a data destination" };

    if (nbDataBatch == 0)
        throw std::invalid_argument{ "JobGgmlOptDataset host data batch size must be greater than zero" };

    if (hasLabels() != (labelsBatch != nullptr))
        throw std::invalid_argument{ "JobGgmlOptDataset host labels presence does not match the dataset" };

    if (m_nbsData == 0 || nbDataBatch % m_nbsData != 0)
        throw std::invalid_argument{ "JobGgmlOptDataset host data batch size is not divisible by nbsData" };

    const std::size_t shardsPerBatch = nbDataBatch / m_nbsData;
    if (shardsPerBatch == 0)
        throw std::invalid_argument{ "JobGgmlOptDataset host batch must contain at least one shard" };

    if (ibatch < 0)
        throw std::out_of_range{ "JobGgmlOptDataset host batch index cannot be negative" };

    const std::size_t batchIndex = static_cast<std::size_t>(ibatch);
    const std::size_t totalShards = static_cast<std::size_t>(shardCount());
    if (batchIndex > std::numeric_limits<std::size_t>::max() / shardsPerBatch || (batchIndex + 1) * shardsPerBatch > totalShards)
        throw std::out_of_range{ "JobGgmlOptDataset host batch extends beyond the dataset" };

    ggml_opt_dataset_get_batch_host(m_dataset,
                                    dataBatch,
                                    nbDataBatch,
                                    labelsBatch,
                                    ibatch);
}

ggml_opt_dataset_t JobGgmlOptDataset::dataset() noexcept
{
    return m_dataset;
}

const struct ggml_opt_dataset *JobGgmlOptDataset::dataset() const noexcept
{
    return m_dataset;
}

std::size_t JobGgmlOptDataset::shardByteCount(std::size_t totalByteCount,
                                              std::int64_t ndata,
                                              std::int64_t ndataShard)
{
    if (ndata <= 0 || ndataShard <= 0)
        throw std::invalid_argument{ "Dataset dimensions must be greater than zero" };

    const auto dataCount = static_cast<std::size_t>(ndata);
    const auto shardDataCount = static_cast<std::size_t>(ndataShard);
    if (totalByteCount % dataCount != 0)
        throw std::runtime_error{ "Dataset tensor byte count is not divisible by ndata" };

    const std::size_t bytesPerDatapoint = totalByteCount / dataCount;
    if (bytesPerDatapoint > std::numeric_limits<std::size_t>::max() / shardDataCount)
        throw std::overflow_error{ "Dataset shard byte count exceeds size_t" };

    return bytesPerDatapoint *
           shardDataCount;
}

} // namespace job::ggml