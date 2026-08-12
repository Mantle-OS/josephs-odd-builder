#include "job_ggml_quantizer.h"

#include <limits>
#include <stdexcept>

#include <ggml.h>

namespace job::ggml {

void JobGgmlQuantizer::init(JobGgmlType type)
{
    ggml_quantize_init(static_cast<ggml_type>(type));
}

void JobGgmlQuantizer::nativeFree() noexcept
{
    ggml_quantize_free();
}

bool JobGgmlQuantizer::isQuantized(JobGgmlType type) noexcept
{
    return ggml_is_quantized(static_cast<ggml_type>(type));
}

bool JobGgmlQuantizer::requiresImportanceMatrix(JobGgmlType type) noexcept
{
    return ggml_quantize_requires_imatrix(static_cast<ggml_type>(type));
}

std::int64_t JobGgmlQuantizer::blockSize(JobGgmlType type) noexcept
{
    return ggml_blck_size(static_cast<ggml_type>(type));
}

std::size_t JobGgmlQuantizer::typeSize(JobGgmlType type) noexcept
{
    return ggml_type_size(static_cast<ggml_type>(type));
}

std::size_t JobGgmlQuantizer::rowSize(JobGgmlType type, std::int64_t elementsPerRow) noexcept
{
    if (elementsPerRow <= 0)
        return 0;

    const std::int64_t nativeBlockSize = blockSize(type);

    if (nativeBlockSize <= 0)
        return 0;

    if ((elementsPerRow % nativeBlockSize) != 0)
        return 0;

    return ggml_row_size(static_cast<ggml_type>(type), elementsPerRow);
}

std::size_t JobGgmlQuantizer::requiredBytes(const JobGgmlQuantizationParams &params) noexcept
{
    if (params.rows() <= 0 || params.elementsPerRow() <= 0)
        return 0;

    const std::size_t nativeRowSize = rowSize(params.type(), params.elementsPerRow());

    if (nativeRowSize == 0)
        return 0;

    const std::size_t rows = static_cast<std::size_t>(params.rows());

    if (rows > (std::numeric_limits<std::size_t>::max() / nativeRowSize))
        return 0;

    return rows * nativeRowSize;
}

bool JobGgmlQuantizer::validate(const JobGgmlQuantizationParams &params,
                                std::span<const float> source,
                                std::span<const std::byte> destination) noexcept
{
    if (params.start() < 0 || params.rows() <= 0 || params.elementsPerRow() <= 0)
        return false;

    const std::int64_t nativeBlockSize = blockSize(params.type());

    if (nativeBlockSize <= 0)
        return false;

    if ((params.start() % nativeBlockSize) != 0)
        return false;

    if ((params.start() % params.elementsPerRow()) != 0)
        return false;

    if ((params.elementsPerRow() % nativeBlockSize) != 0)
        return false;

    if (requiresImportanceMatrix(params.type()) && !params.hasImportanceMatrix())
        return false;

    const std::uint64_t start = static_cast<std::uint64_t>(params.start());
    const std::uint64_t rows = static_cast<std::uint64_t>(params.rows());
    const std::uint64_t elementsPerRow = static_cast<std::uint64_t>(params.elementsPerRow());

    if (rows > ((std::numeric_limits<std::uint64_t>::max() - start) / elementsPerRow))
        return false;

    const std::uint64_t requiredSourceElements = start + (rows * elementsPerRow);

    if (requiredSourceElements > source.size())
        return false;

    const std::size_t nativeRowSize = rowSize(params.type(), params.elementsPerRow());

    if (nativeRowSize == 0)
        return false;

    const std::uint64_t startRow = start / elementsPerRow;

    if (startRow > (std::numeric_limits<std::uint64_t>::max() / nativeRowSize))
        return false;

    const std::uint64_t destinationOffset = startRow * nativeRowSize;
    if (rows > ((std::numeric_limits<std::uint64_t>::max() - destinationOffset) / nativeRowSize))
        return false;

    const std::uint64_t requiredDestinationBytes = destinationOffset + (rows * nativeRowSize);

    if (requiredDestinationBytes > destination.size())
        return false;

    return true;
}

JobGgmlQuantizationResult JobGgmlQuantizer::quantizeChunk(const JobGgmlQuantizationParams &params,
                                                          std::span<const float> source,
                                                          std::span<std::byte> destination )
{
    if (!validate(params, source, destination))
        throw std::invalid_argument{ "Invalid GGML quantization parameters or buffers" };

    const std::span<const float> importanceMatrix = params.importanceMatrix();
    const float *nativeImportanceMatrix = importanceMatrix.empty() ? nullptr : importanceMatrix.data();
    const std::size_t bytesWritten = ggml_quantize_chunk(static_cast<ggml_type>(params.type()),
                                                         source.data(),
                                                         destination.data(),
                                                         params.start(),
                                                         params.rows(),
                                                         params.elementsPerRow(),
                                                         nativeImportanceMatrix);

    return JobGgmlQuantizationResult{params.type(), bytesWritten, params.rows()};
}

} // namespace job::ggml