#pragma once

#include <cstddef>
#include <memory>
#include <string_view>

#include "jobusb_export.h"

namespace job::usb {

class JOBUSB_EXPORT JobUsbIdCursor
{
public:
    using Ptr  = std::shared_ptr<JobUsbIdCursor>;
    using WPtr = std::weak_ptr<JobUsbIdCursor>;
    using UPtr = std::unique_ptr<JobUsbIdCursor>;

    struct Line
    {
        std::string_view    raw{};
        std::string_view    text{};
        std::size_t         number      = 0;
        std::size_t         indentation = 0;
    };

    JobUsbIdCursor() = delete;
    explicit JobUsbIdCursor(std::string_view source) noexcept;
    ~JobUsbIdCursor() = default;

    JobUsbIdCursor(const JobUsbIdCursor &) = default;
    JobUsbIdCursor &operator=(const JobUsbIdCursor &) = default;
    JobUsbIdCursor(JobUsbIdCursor &&) noexcept = default;
    JobUsbIdCursor &operator=(JobUsbIdCursor &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(std::string_view source);
    [[nodiscard]] static UPtr createUniq(std::string_view source);

    [[nodiscard]] bool atEnd() const noexcept;
    [[nodiscard]] const Line &line() const noexcept pre(!atEnd());

    [[nodiscard]]  bool advance() noexcept;

private:
    void readCurrentLine() noexcept;

    std::string_view    m_source;
    Line                m_line;
    std::size_t         m_offset    = 0;
};

} // namespace job::usb