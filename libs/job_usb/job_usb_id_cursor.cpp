#include "job_usb_id_cursor.h"
#include "job_usb_id_line_kernel.h"

namespace job::usb {

JobUsbIdCursor::JobUsbIdCursor(std::string_view source) noexcept :
    m_source(source)
{
    if (!m_source.empty())
        readCurrentLine();
}

JobUsbIdCursor::Ptr JobUsbIdCursor::createShared(std::string_view source)
{
    return std::make_shared<JobUsbIdCursor>(source);
}

JobUsbIdCursor::UPtr JobUsbIdCursor::createUniq(std::string_view source)
{
    return std::make_unique<JobUsbIdCursor>(source);
}

bool JobUsbIdCursor::atEnd() const noexcept
{
    return m_offset >= m_source.size();
}

const JobUsbIdCursor::Line &JobUsbIdCursor::line() const noexcept
{
    return m_line;
}

bool JobUsbIdCursor::advance() noexcept
{
    if (atEnd())
        return false;

    const auto newline = m_source.find('\n', m_offset);

    if (newline == std::string_view::npos) {
        m_offset = m_source.size();
        m_line = {};
        return false;
    }

    m_offset = newline + 1;
    if (atEnd()) {
        m_line = {};
        return false;
    }

    readCurrentLine();
    return true;
}

void JobUsbIdCursor::readCurrentLine() noexcept
{
    contract_assert(m_offset < m_source.size());
    const auto newline = m_source.find('\n', m_offset);
    const auto end = newline == std::string_view::npos ? m_source.size() : newline;

    auto raw = m_source.subview(m_offset, end - m_offset);
    if (!raw.empty() && raw.back() == '\r')
        raw.remove_suffix(1);

    m_line.raw = raw;
    m_line.text = JobUsbIdLineKernel::removeIndentation(raw);
    ++m_line.number;
    m_line.indentation = JobUsbIdLineKernel::indentation(raw);
}

} // namespace job::usb