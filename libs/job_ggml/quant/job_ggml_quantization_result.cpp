#include "job_ggml_quantization_result.h"

namespace job::ggml {

JobGgmlQuantizationResult::JobGgmlQuantizationResult(JobGgmlType type, std::size_t bytesWritten, std::int64_t rowsProcessed ) noexcept :
    m_type{type},
    m_bytesWritten{bytesWritten},
    m_rowsProcessed{rowsProcessed}
{

}

JobGgmlType JobGgmlQuantizationResult::type() const noexcept
{
    return m_type;
}

void JobGgmlQuantizationResult::setType(JobGgmlType type) noexcept
{
    m_type = type;
}

std::size_t JobGgmlQuantizationResult::bytesWritten() const noexcept
{
    return m_bytesWritten;
}

void JobGgmlQuantizationResult::setBytesWritten(std::size_t bytesWritten) noexcept
{
    m_bytesWritten = bytesWritten;
}

std::int64_t JobGgmlQuantizationResult::rowsProcessed() const noexcept
{
    return m_rowsProcessed;
}

void JobGgmlQuantizationResult::setRowsProcessed(std::int64_t rowsProcessed) noexcept
{
    m_rowsProcessed = rowsProcessed;
}

} // namespace job::ggml