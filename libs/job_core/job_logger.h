#pragma once

#include <mutex>
#include <string>
#include <atomic>

namespace job::core {

enum class LogLevel : uint8_t {
    Error = 0,
    Warn,
    Info,
    Debug
};

class JobLogger final {
public:
    static JobLogger &instance() noexcept;

    void setLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;

    void log(LogLevel lvl, const std::string &msg) noexcept;

private:
    JobLogger() = default;
    ~JobLogger() = default;
    JobLogger(const JobLogger &) = delete;
    JobLogger &operator=(const JobLogger &) = delete;

    [[nodiscard]] std::string timestamp() const;

private:
    std::atomic<LogLevel> m_level{LogLevel::Info};
    mutable std::mutex m_mutex;
};

// Macros
#define JOB_LOG_ERROR(msg) ::job::core::JobLogger::instance().log(::job::core::LogLevel::Error, msg)
#define JOB_LOG_WARN(msg)  ::job::core::JobLogger::instance().log(::job::core::LogLevel::Warn,  msg)
#define JOB_LOG_INFO(msg)  ::job::core::JobLogger::instance().log(::job::core::LogLevel::Info,  msg)
#define JOB_LOG_DEBUG(msg) ::job::core::JobLogger::instance().log(::job::core::LogLevel::Debug, msg)

} // namespace job::core
