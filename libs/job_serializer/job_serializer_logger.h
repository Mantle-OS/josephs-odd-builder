#pragma once

#include <mutex>
#include <string>
#include <atomic>
#include <format>

namespace job::serializer {

enum class LogLevel : uint8_t {
    Error = 0,
    Warn,
    Info,
    Debug,
    Assert
};

class JobSerLogger final {
public:
    static JobSerLogger &instance() noexcept;

    void setLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;

    void log(LogLevel lvl, const std::string &msg) noexcept;

private:
    JobSerLogger() = default;
    ~JobSerLogger() = default;
    JobSerLogger(const JobSerLogger &) = delete;
    JobSerLogger &operator=(const JobSerLogger &) = delete;

    [[nodiscard]] std::string timestamp() const;

private:
    std::atomic<LogLevel> m_level{LogLevel::Info};
    mutable std::mutex m_mutex;
};

} // namespace job::serializer


namespace job::serializer::detail {
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
#define JOB_SER_ERROR(fmt, ...) \
::job::serializer::JobSerLogger::instance().log( \
              ::job::serializer::LogLevel::Error, \
              ::job::serializer::detail::format_log(fmt, ##__VA_ARGS__) \
            )

#define JOB_SER_WARN(fmt, ...) \
    ::job::serializer::JobSerLogger::instance().log( \
              ::job::serializer::LogLevel::Warn, \
              ::job::serializer::detail::format_log(fmt, ##__VA_ARGS__) \
            )

#define JOB_SER_INFO(fmt, ...) \
    ::job::serializer::JobSerLogger::instance().log( \
              ::job::serializer::LogLevel::Info, \
              ::job::serializer::detail::format_log(fmt, ##__VA_ARGS__) \
            )

#define JOB_SER_DEBUG(fmt, ...) \
    ::job::serializer::JobSerLogger::instance().log( \
              ::job::serializer::LogLevel::Debug, \
              ::job::serializer::detail::format_log(fmt, ##__VA_ARGS__) \
            )

#define JOB_SER_ASSERT(fmt, ...) \
    ::job::serializer::JobSerLogger::instance().log( \
              ::job::serializer::LogLevel::Assert, \
              ::job::serializer::detail::format_log(fmt, ##__VA_ARGS__) \
            )

} // job::serializer::detail

