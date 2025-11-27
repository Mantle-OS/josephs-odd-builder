#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <job_logger.h>

using namespace job::core;

TEST_CASE("JobLogger respects log levels", "[logger]") {
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Warn);

    std::ostringstream outCapture;
    std::ostringstream errCapture;

    std::streambuf *oldOut = std::cout.rdbuf(outCapture.rdbuf());
    std::streambuf *oldErr = std::cerr.rdbuf(errCapture.rdbuf());

    JOB_LOG_INFO("This should not appear");
    JOB_LOG_WARN("This should appear");

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    // Combine both streams for analysis
    std::string output = outCapture.str() + errCapture.str();

    REQUIRE(output.find("WARN") != std::string::npos);
    REQUIRE(output.find("INFO") == std::string::npos);
}


TEST_CASE("JobLogger prints timestamps and levels", "[logger]") {
    JobLogger &logger = JobLogger::instance();
    logger.setLevel(LogLevel::Debug);

    std::ostringstream capture;
    std::streambuf *old = std::cout.rdbuf(capture.rdbuf());

    JOB_LOG_DEBUG("test message");

    std::cout.rdbuf(old);

    std::string output = capture.str();
    REQUIRE(output.find("DEBUG") != std::string::npos);
    REQUIRE(output.find("test message") != std::string::npos);
    REQUIRE(output.find('[') != std::string::npos);  // timestamp brackets
}
// CHECKPOINT: v1.0
