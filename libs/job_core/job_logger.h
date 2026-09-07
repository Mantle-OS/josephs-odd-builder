#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#include "job_contract.h"
#include "jobcore_export.h"

namespace job::core {

enum class LogLevel : std::uint8_t {
    Error = 0,
    Warn,
    Info,
    Debug
};

class JOBCORE_EXPORT JobLogger final
{
public:
    static JobLogger &instance() noexcept;

    void setLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;

    void log(LogLevel level, const std::string &message) noexcept;

    static void contractViolation(const ContractViolation &violation) noexcept;
    void logContractViolation(const ContractViolation &violation) noexcept;

private:
    JobLogger() noexcept;
    ~JobLogger() = default;

    JobLogger(const JobLogger &) = delete;
    JobLogger &operator=(const JobLogger &) = delete;
    JobLogger(JobLogger &&) = delete;
    JobLogger &operator=(JobLogger &&) = delete;

    [[nodiscard]] std::string timestamp() const;

    std::atomic<LogLevel> m_level{LogLevel::Info};
    mutable std::mutex    m_mutex;
};

namespace detail {

template <typename... Args>
[[nodiscard]] inline std::string formatLog(std::format_string<Args...> fmt, Args&&... args)
{
    return std::format(fmt, std::forward<Args>(args)...);
}

} // namespace detail

} // namespace job::core

#define JOB_LOG_ERROR(fmt, ...) \
::job::core::JobLogger::instance().log( \
                                        ::job::core::LogLevel::Error, \
                                        ::job::core::detail::formatLog(fmt __VA_OPT__(,) __VA_ARGS__))

#define JOB_LOG_WARN(fmt, ...) \
    ::job::core::JobLogger::instance().log( \
              ::job::core::LogLevel::Warn, \
              ::job::core::detail::formatLog(fmt __VA_OPT__(,) __VA_ARGS__))

#define JOB_LOG_INFO(fmt, ...) \
    ::job::core::JobLogger::instance().log( \
              ::job::core::LogLevel::Info, \
              ::job::core::detail::formatLog(fmt __VA_OPT__(,) __VA_ARGS__))

#define JOB_LOG_DEBUG(fmt, ...) \
    ::job::core::JobLogger::instance().log( \
              ::job::core::LogLevel::Debug, \
              ::job::core::detail::formatLog(fmt __VA_OPT__(,) __VA_ARGS__))


