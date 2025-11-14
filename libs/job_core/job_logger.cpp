#include "job_logger.h"
#include <iomanip>
#include <iostream>
#include <iosfwd>

namespace job::core {

JobLogger &JobLogger::instance() noexcept
{
    static JobLogger logger;
    return logger;
}

void JobLogger::setLevel(LogLevel level) noexcept
{
    m_level.store(level, std::memory_order_relaxed);
}

LogLevel JobLogger::level() const noexcept
{
    return m_level.load(std::memory_order_relaxed);
}

void JobLogger::log(LogLevel lvl, const std::string &msg) noexcept
{
    if (lvl > m_level.load(std::memory_order_relaxed))
        return;

    const char *lvlStr = nullptr;
    switch (lvl) {
    case LogLevel::Error:
        lvlStr = "ERROR";
        break;
    case LogLevel::Warn:
        lvlStr = "WARN ";
        break;
    case LogLevel::Info:
        lvlStr = "INFO ";
        break;
    case LogLevel::Debug:
        lvlStr = "DEBUG";
        break;
    }

    const std::string timeStr = timestamp();

    std::ostringstream line;
    line << '[' << timeStr << "] [" << lvlStr << "] " << msg << '\n';

    // Thread-safe I/O block
    {
        std::scoped_lock lock(m_mutex);
        if (lvl == LogLevel::Error || lvl == LogLevel::Warn)
            std::cerr << line.str();
        else
            std::cout << line.str();
    }
}

std::string JobLogger::timestamp() const
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace job::core
// CHECKPOINT: v1
