#pragma once

#include <mutex>
#include <string>
#include <atomic>
#include <format>

namespace job::core {

enum class LogLevel : uint8_t {
    Error = 0,
    Warn,
    Info,
    Debug,
    Assert
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

} // namespace job::core

// FIXME (BACKLOG) we should have a bit of better messaging. example adding categories
// Also build time checking is not working. changing this would be something that is
// needed
namespace job::core::detail {
template <typename... Args>
inline std::string format_log(std::string_view fmt, Args&&... args)
{
    if constexpr (sizeof...(args) == 0) {
        return std::string(fmt);
    } else {
        auto tuple = std::make_tuple(std::forward<Args>(args)...);
        return std::apply([&](auto&... unpacked) {
            return std::vformat(fmt, std::make_format_args(unpacked...));
        }, tuple);
    }
}



// Helper macros
#define JOB_LOG_ERROR(fmt, ...) \
::job::core::JobLogger::instance().log( \
    ::job::core::LogLevel::Error, \
    ::job::core::detail::format_log(fmt, ##__VA_ARGS__) \
)

#define JOB_LOG_WARN(fmt, ...) \
::job::core::JobLogger::instance().log( \
          ::job::core::LogLevel::Warn, \
          ::job::core::detail::format_log(fmt, ##__VA_ARGS__) \
)

#define JOB_LOG_INFO(fmt, ...) \
::job::core::JobLogger::instance().log( \
          ::job::core::LogLevel::Info, \
          ::job::core::detail::format_log(fmt, ##__VA_ARGS__) \
)

#define JOB_LOG_DEBUG(fmt, ...) \
::job::core::JobLogger::instance().log( \
          ::job::core::LogLevel::Debug, \
          ::job::core::detail::format_log(fmt, ##__VA_ARGS__) \
)

#define JOB_LOG_ASSERT(fmt, ...) \
::job::core::JobLogger::instance().log( \
          ::job::core::LogLevel::Assert, \
          ::job::core::detail::format_log(fmt, ##__VA_ARGS__) \
)

} // job::core::detail

