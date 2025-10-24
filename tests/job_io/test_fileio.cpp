#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
#include <filesystem>

#include "file_io.h"

using namespace job::io;
namespace fs = std::filesystem;


[[nodiscard]] bool deleteFile(const std::string &ab_file_path)
{
    std::error_code ec;
    if (fs::exists(ab_file_path, ec)) {
        fs::remove(ab_file_path, ec);
        return !fs::exists(ab_file_path, ec);
    }
    return true; // already gone, treat as success
}

[[nodiscard]] bool setupFile(const std::string &txt,
                             const std::string &path = "/tmp/fileio_lifecycle_test.txt")
{
    FileIO writer(path, FileMode::RegularFile, true);
    if (!writer.openDevice())
        return false;

    const ssize_t written = writer.write(txt.c_str(), txt.size());
    writer.closeDevice();
    return (written == static_cast<ssize_t>(txt.size())) && writer.flush();
}

TEST_CASE("FileIO basic open/close lifecycle", "[fileio]") {
    const std::string path = "/tmp/fileio_lifecycle_test.txt";
    REQUIRE(deleteFile(path));

    {
        FileIO writer(path, FileMode::RegularFile, true);
        REQUIRE(writer.openDevice());
        REQUIRE(writer.isOpen());
        writer.closeDevice();
        REQUIRE(writer.flush()); // I hear that it is backwards in Austraila ?
        REQUIRE_FALSE(writer.isOpen());
    }

    // Closing twice should not crash
    FileIO f(path, FileMode::RegularFile, false);
    f.closeDevice();
    REQUIRE(f.flush()); // I hear that it is backwards in Austraila ?

    REQUIRE_NOTHROW(f.closeDevice());

    REQUIRE(deleteFile(path));
}

TEST_CASE("FileIO write and read back content", "[fileio]") {
    const std::string path = "/tmp/fileio_rw_test.txt";
    const std::string data = "Hello FileIO!\n";
    REQUIRE(deleteFile(path));

    REQUIRE(setupFile(data, path)); // <---- Move this up here

    SECTION("Write data to file") {
        FileIO writer(path, FileMode::RegularFile, true);
        REQUIRE(writer.openDevice());
        const ssize_t written = writer.write(data.c_str(), data.size());
        REQUIRE(written == static_cast<ssize_t>(data.size()));
        writer.closeDevice();
    }

    SECTION("Read back data from file") {
        FileIO reader(path, FileMode::RegularFile, false);
        REQUIRE(reader.openDevice());
        char buffer[256] = {0};
        const ssize_t readBytes = reader.read(buffer, sizeof(buffer));
        REQUIRE(readBytes > 0);
        REQUIRE(std::string(buffer, readBytes).find("Hello FileIO!") != std::string::npos);
        reader.closeDevice();
        REQUIRE(reader.flush());
    }

    REQUIRE(deleteFile(path));
}

TEST_CASE("FileIO writes to stdout and stderr", "[fileio][stdio]") {
    std::ostringstream captureOut;
    std::ostringstream captureErr;

    std::streambuf *oldCout = std::cout.rdbuf(captureOut.rdbuf());
    std::streambuf *oldCerr = std::cerr.rdbuf(captureErr.rdbuf());

    {
        FileIO outFile("", FileMode::StdOut, true);
        REQUIRE(outFile.openDevice());
        const std::string msg = "stdout test\n";
        const ssize_t written = outFile.write(msg.c_str(), msg.size());
        REQUIRE(written == static_cast<ssize_t>(msg.size()));
    }

    {
        FileIO errFile("", FileMode::StdErr, true);
        REQUIRE(errFile.openDevice());
        const std::string msg = "stderr test\n";
        const ssize_t written = errFile.write(msg.c_str(), msg.size());
        REQUIRE(written == static_cast<ssize_t>(msg.size()));
    }

    std::cout.rdbuf(oldCout);
    std::cerr.rdbuf(oldCerr);

    const std::string outText = captureOut.str();
    const std::string errText = captureErr.str();

    REQUIRE(outText.find("stdout test") != std::string::npos);
    REQUIRE(errText.find("stderr test") != std::string::npos);
}

TEST_CASE("FileIO invalid operations", "[fileio][invalid]") {
    FileIO stdinFile("", FileMode::StdIn, false);
    REQUIRE(stdinFile.openDevice());

    const char *msg = "should fail\n";
    const ssize_t invalidWrite = stdinFile.write(msg, strlen(msg));
    REQUIRE(invalidWrite == -1);

    FileIO stdoutFile("", FileMode::StdOut, true);
    REQUIRE(stdoutFile.openDevice());

    char buf[16];
    const ssize_t invalidRead = stdoutFile.read(buf, sizeof(buf));
    REQUIRE(invalidRead == -1);
}
