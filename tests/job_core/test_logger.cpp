#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <contracts>

#include <job_contract.h>
#include <job_logger.h>

using namespace job::core;

TEST_CASE("JobLogger singleton is stable", "[logger]")
{
    JobLogger &first = JobLogger::instance();
    JobLogger &second = JobLogger::instance();

    REQUIRE(&first == &second);
}

TEST_CASE("JobLogger stores log levels", "[logger]")
{
    JobLogger &logger = JobLogger::instance();

    logger.setLevel(LogLevel::Error);
    REQUIRE(logger.level() == LogLevel::Error);

    logger.setLevel(LogLevel::Warn);
    REQUIRE(logger.level() == LogLevel::Warn);

    logger.setLevel(LogLevel::Info);
    REQUIRE(logger.level() == LogLevel::Info);

    logger.setLevel(LogLevel::Debug);
    REQUIRE(logger.level() == LogLevel::Debug);
}

TEST_CASE("JobLogger respects log levels", "[logger]")
{
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Warn);

    std::ostringstream outCapture;
    std::ostringstream errCapture;

    std::streambuf *oldOut = std::cout.rdbuf(outCapture.rdbuf());
    std::streambuf *oldErr = std::cerr.rdbuf(errCapture.rdbuf());

    JOB_LOG_ERROR("error message");
    JOB_LOG_WARN("warn message");
    JOB_LOG_INFO("info message");
    JOB_LOG_DEBUG("debug message");

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    const std::string output = outCapture.str() + errCapture.str();

    REQUIRE(output.find("ERROR") != std::string::npos);
    REQUIRE(output.find("WARN") != std::string::npos);
    REQUIRE(output.find("INFO") == std::string::npos);
    REQUIRE(output.find("DEBUG") == std::string::npos);
}

TEST_CASE("JobLogger routes error and warning to stderr", "[logger]")
{
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Debug);

    std::ostringstream outCapture;
    std::ostringstream errCapture;

    std::streambuf *oldOut = std::cout.rdbuf(outCapture.rdbuf());
    std::streambuf *oldErr = std::cerr.rdbuf(errCapture.rdbuf());

    JOB_LOG_ERROR("error destination");
    JOB_LOG_WARN("warn destination");

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    REQUIRE(outCapture.str().empty());
    REQUIRE(errCapture.str().find("error destination") != std::string::npos);
    REQUIRE(errCapture.str().find("warn destination") != std::string::npos);
}

TEST_CASE("JobLogger routes info and debug to stdout", "[logger]")
{
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Debug);

    std::ostringstream outCapture;
    std::ostringstream errCapture;

    std::streambuf *oldOut = std::cout.rdbuf(outCapture.rdbuf());
    std::streambuf *oldErr = std::cerr.rdbuf(errCapture.rdbuf());

    JOB_LOG_INFO("info destination");
    JOB_LOG_DEBUG("debug destination");

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    REQUIRE(outCapture.str().find("info destination") != std::string::npos);
    REQUIRE(outCapture.str().find("debug destination") != std::string::npos);
    REQUIRE(errCapture.str().empty());
}

TEST_CASE("JobLogger formats arguments", "[logger]")
{
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Debug);

    std::ostringstream capture;
    std::streambuf *old = std::cout.rdbuf(capture.rdbuf());

    JOB_LOG_INFO("value={} name={}", 42, "JOB");

    std::cout.rdbuf(old);

    REQUIRE(capture.str().find("value=42 name=JOB") != std::string::npos);
}

TEST_CASE("JobLogger accepts plain messages", "[logger]")
{
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Debug);

    std::ostringstream capture;
    std::streambuf *old = std::cout.rdbuf(capture.rdbuf());

    JOB_LOG_INFO("plain message");

    std::cout.rdbuf(old);

    REQUIRE(capture.str().find("plain message") != std::string::npos);
}

TEST_CASE("JobLogger prints timestamps and levels", "[logger]")
{
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Debug);

    std::ostringstream capture;
    std::streambuf *old = std::cout.rdbuf(capture.rdbuf());

    JOB_LOG_DEBUG("test message");

    std::cout.rdbuf(old);

    const std::string output = capture.str();

    REQUIRE(output.starts_with("["));
    REQUIRE(output.find("] [DEBUG]") != std::string::npos);
    REQUIRE(output.find("test message") != std::string::npos);
}

TEST_CASE("JobLogger registers the contract callback", "[logger][contract]")
{
    JobLogger &logger = JobLogger::instance();
    (void)logger;

    REQUIRE(contractCallback() == &JobLogger::contractViolation);
}

TEST_CASE("JobLogger contract callback is independent of log level", "[logger][contract]")
{
    JobLogger &logger = JobLogger::instance();

    logger.setLevel(LogLevel::Error);
    REQUIRE(contractCallback() == &JobLogger::contractViolation);

    logger.setLevel(LogLevel::Warn);
    REQUIRE(contractCallback() == &JobLogger::contractViolation);

    logger.setLevel(LogLevel::Info);
    REQUIRE(contractCallback() == &JobLogger::contractViolation);

    logger.setLevel(LogLevel::Debug);
    REQUIRE(contractCallback() == &JobLogger::contractViolation);
}