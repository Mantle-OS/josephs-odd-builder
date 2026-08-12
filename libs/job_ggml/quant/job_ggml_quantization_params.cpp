#include "job_ggml_quantization_params.h"

namespace job::ggml {

JobGgmlQuantizationParams::JobGgmlQuantizationParams(JobGgmlType type, std::int64_t start, std::int64_t rows, std::int64_t elementsPerRow) noexcept :
    m_type{type},
    m_start{start},
    m_rows{rows},
    m_elementsPerRow{elementsPerRow}
{

}

JobGgmlType JobGgmlQuantizationParams::type() const noexcept
{
    return m_type;
}

void JobGgmlQuantizationParams::setType(JobGgmlType type) noexcept
{
    m_type = type;
}

std::int64_t JobGgmlQuantizationParams::start() const noexcept
{
    return m_start;
}

void JobGgmlQuantizationParams::setStart(std::int64_t start) noexcept
{
    m_start = start;
}

std::int64_t JobGgmlQuantizationParams::rows() const noexcept
{
    return m_rows;
}

void JobGgmlQuantizationParams::setRows(std::int64_t rows) noexcept
{
    m_rows = rows;
}

std::int64_t JobGgmlQuantizationParams::elementsPerRow() const noexcept
{
    return m_elementsPerRow;
}

void JobGgmlQuantizationParams::setElementsPerRow(std::int64_t elementsPerRow) noexcept
{
    m_elementsPerRow = elementsPerRow;
}

std::span<const float> JobGgmlQuantizationParams::importanceMatrix() const noexcept
{
    return m_importanceMatrix;
}

void JobGgmlQuantizationParams::setImportanceMatrix(std::span<const float> importanceMatrix) noexcept
{
    m_importanceMatrix = importanceMatrix;
}

void JobGgmlQuantizationParams::clearImportanceMatrix() noexcept
{
    m_importanceMatrix = {};
}

bool JobGgmlQuantizationParams::hasImportanceMatrix() const noexcept
{
    return !m_importanceMatrix.empty();
}

} // namespace job::ggml