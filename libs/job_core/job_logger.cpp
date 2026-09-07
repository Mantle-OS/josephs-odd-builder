
#include "job_logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace job::core {

JobLogger &JobLogger::instance() noexcept
{
    static JobLogger logger;
    return logger;
}

JobLogger::JobLogger() noexcept
{
    setContractCallback(&JobLogger::contractViolation);
}

void JobLogger::setLevel(LogLevel level) noexcept
{
    m_level.store(level, std::memory_order_relaxed);
}

LogLevel JobLogger::level() const noexcept
{
    return m_level.load(std::memory_order_relaxed);
}

void JobLogger::log(LogLevel level, const std::string &message) noexcept
{
    if (level > m_level.load(std::memory_order_relaxed))
        return;

    const char *levelString = nullptr;

    switch (level) {
    case LogLevel::Error:
        levelString = "ERROR";
        break;
    case LogLevel::Warn:
        levelString = "WARN ";
        break;
    case LogLevel::Info:
        levelString = "INFO ";
        break;
    case LogLevel::Debug:
        levelString = "DEBUG";
        break;
    }

    const std::string timeString = timestamp();

    std::ostringstream line;
    line << '[' << timeString << "] [" << levelString << "] " << message << '\n';

    std::scoped_lock lock(m_mutex);

    if (level == LogLevel::Error || level == LogLevel::Warn)
        std::cerr << line.str();
    else
        std::cout << line.str();
}

void JobLogger::contractViolation(const ContractViolation &violation) noexcept
{
    JobLogger::instance().logContractViolation(violation);
}

void JobLogger::logContractViolation(const ContractViolation &violation) noexcept
{
    const auto location = violation.location();

    const char *kindString = "UNKNOWN";
    switch (violation.kind()) {
    case std::contracts::assertion_kind::pre:
        kindString = "PRE";
        break;
    case std::contracts::assertion_kind::post:
        kindString = "POST";
        break;
    case std::contracts::assertion_kind::assert:
        kindString = "ASSERT";
        break;
    }

    const char *semanticString = "UNKNOWN";
    switch (violation.semantic()) {
    case std::contracts::evaluation_semantic::ignore:
        semanticString = "IGNORE";
        break;
    case std::contracts::evaluation_semantic::observe:
        semanticString = "OBSERVE";
        break;
    case std::contracts::evaluation_semantic::enforce:
        semanticString = "ENFORCE";
        break;
    case std::contracts::evaluation_semantic::quick_enforce:
        semanticString = "QUICK_ENFORCE";
        break;
    }

    const char *modeString = "UNKNOWN";
    switch (violation.mode()) {
    case std::contracts::detection_mode::predicate_false:
        modeString = "PREDICATE_FALSE";
        break;
    case std::contracts::detection_mode::evaluation_exception:
        modeString = "EVALUATION_EXCEPTION";
        break;
    }

    const std::string timeString = timestamp();

    std::ostringstream line;
    line << '[' << timeString << "] [CONTRACT]"
         << " [" << kindString << ']'
         << " [" << semanticString << ']'
         << " [" << modeString << ']';

    if (violation.is_terminating())
        line << " [TERMINATING]";

    line << '\n';

    if (violation.comment() && violation.comment()[0] != '\0')
        line << "  " << violation.comment() << '\n';

    if (location.file_name() && location.file_name()[0] != '\0')
        line << "  " << location.file_name() << ':' << location.line() << ':' << location.column() << '\n';

    if (location.function_name() && location.function_name()[0] != '\0')
        line << "  " << location.function_name() << '\n';

    std::scoped_lock lock(m_mutex);
    std::cerr << line.str();
}

std::string JobLogger::timestamp() const
{
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto time = system_clock::to_time_t(now);
    const auto milliseconds = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

    return stream.str();
}

} // namespace job::core