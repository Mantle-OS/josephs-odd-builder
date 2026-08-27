#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <limits>

#include <job_file.h>

#include "../transient_test_file.h"

namespace job::io::test {

[[nodiscard]] std::filesystem::path tmpPath(std::string_view name)
{
    return std::filesystem::temp_directory_path() /
           ("job_file_" + std::to_string(::getpid()) + "_" + std::string(name));
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobFile reads an existing file", "[job_io][file][usage]")
{
    const auto path = job::io::test::tmpPath("read_existing.bin");
    TransientTestFile tmp(path.string(), std::string_view("hello from JobFile"));

    job::io::JobFile file(path);

    REQUIRE(file.openDevice());
    REQUIRE(file.isOpen());
    REQUIRE(file.access() == job::io::JobFile::Access::ReadOnly);
    REQUIRE(file.size() == 18);

    std::string contents;
    REQUIRE(file.readAll(contents) == 18);
    REQUIRE(contents == "hello from JobFile");
}

TEST_CASE("JobFile creates writes seeks and reads a file", "[job_io][file][usage]")
{
    const auto path = job::io::test::tmpPath("read_write.bin");
    TransientTestFile tmp(path.string());

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::Truncate);

    REQUIRE(file.openDevice());

    constexpr std::string_view message = "Joseph's Odd Builder";

    REQUIRE(file.write(message.data(), message.size()) == static_cast<ssize_t>(message.size()));
    REQUIRE(file.size() == message.size());
    REQUIRE(file.tell() == message.size());

    REQUIRE(file.seek(0, job::io::JobFile::Seek::Begin));

    std::string contents;
    REQUIRE(file.readAll(contents) == static_cast<ssize_t>(message.size()));
    REQUIRE(contents == message);
}

TEST_CASE("JobFile appends to an existing file", "[job_io][file][usage]")
{
    const auto path = job::io::test::tmpPath("append.bin");
    TransientTestFile tmp(path.string(), std::string_view("Hello"));

    {
        job::io::JobFile file(
            path,
            job::io::JobFile::Access::WriteOnly,
            job::io::JobFile::OpenMode::Append);

        REQUIRE(file.openDevice());

        constexpr std::string_view suffix = " world";
        REQUIRE(file.write(suffix.data(), suffix.size()) == static_cast<ssize_t>(suffix.size()));
    }

    job::io::JobFile reader(path);

    REQUIRE(reader.openDevice());

    std::string contents;
    REQUIRE(reader.readAll(contents) == 11);
    REQUIRE(contents == "Hello world");
}

TEST_CASE("JobFile typed IO reads and writes native values", "[job_io][file][usage][typed]")
{
    const auto path = job::io::test::tmpPath("typed.bin");
    TransientTestFile tmp(path.string());

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::Truncate);

    REQUIRE(file.openDevice());

    constexpr std::uint32_t expectedInteger = 0x12345678;
    constexpr float expectedFloat = 123.5F;

    REQUIRE(file.writeValue(expectedInteger));
    REQUIRE(file.writeValue(expectedFloat));

    REQUIRE(file.seek(0, job::io::JobFile::Seek::Begin));

    std::uint32_t integer{};
    float floating{};

    REQUIRE(file.readValue(integer));
    REQUIRE(file.readValue(floating));

    REQUIRE(integer == expectedInteger);
    REQUIRE(floating == expectedFloat);
}

TEST_CASE("JobFile seeks relative to beginning current position and end", "[job_io][file][usage][seek]")
{
    const auto path = job::io::test::tmpPath("seek.bin");
    TransientTestFile tmp(path.string(), std::string_view("0123456789"));

    job::io::JobFile file(path);

    REQUIRE(file.openDevice());
    REQUIRE(file.tell() == 0);

    REQUIRE(file.seek(4, job::io::JobFile::Seek::Begin));
    REQUIRE(file.tell() == 4);

    REQUIRE(file.seek(2, job::io::JobFile::Seek::Current));
    REQUIRE(file.tell() == 6);

    REQUIRE(file.seek(-2, job::io::JobFile::Seek::End));
    REQUIRE(file.tell() == 8);

    char value{};
    REQUIRE(file.read(&value, 1) == 1);
    REQUIRE(value == '8');
}

TEST_CASE("JobFile reads binary data into a byte vector", "[job_io][file][usage][binary]")
{
    const auto path = job::io::test::tmpPath("binary.bin");

    const std::vector<std::byte> source{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x7F},
        std::byte{0x80},
        std::byte{0xFF}
    };

    TransientTestFile tmp(path.string(), source);

    job::io::JobFile file(path);

    REQUIRE(file.openDevice());

    std::vector<std::uint8_t> contents;

    REQUIRE(file.readAll(contents) == 5);
    REQUIRE(contents == std::vector<std::uint8_t>{0x00, 0x01, 0x7F, 0x80, 0xFF});
}

TEST_CASE("JobFile uses POSIX permissions through IODevice", "[job_io][file][usage][permissions]")
{
    const auto path = job::io::test::tmpPath("permissions.bin");
    TransientTestFile tmp(path.string());

    REQUIRE(::chmod(path.c_str(), 0644) == 0);

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::OpenExisting);

    REQUIRE(file.openDevice());
    REQUIRE(job::io::toMode(file.permissions()) == 0644);

    file.setPermissions(job::io::IOPermissions::ReadWriteUser);

    REQUIRE(job::io::toMode(file.permissions()) == 0600);

    struct stat info {};
    REQUIRE(::stat(path.c_str(), &info) == 0);
    REQUIRE((info.st_mode & 07777) == 0600);
}

TEST_CASE("JobFile wraps a borrowed native file descriptor", "[job_io][file][usage][fd]")
{
    const auto path = job::io::test::tmpPath("borrow_fd.bin");
    TransientTestFile tmp(path.string(), std::string_view("native fd"));

    const int fd = ::open(path.c_str(), O_RDONLY);
    REQUIRE(fd >= 0);

    {
        job::io::JobFile file(fd, false);

        REQUIRE(file.isOpen());
        REQUIRE(file.fd() == fd);
        REQUIRE(file.handle() == job::io::JobFile::Handle::FileDescriptor);
        REQUIRE_FALSE(file.owned());
        REQUIRE(file.access() == job::io::JobFile::Access::ReadOnly);

        std::string contents;
        REQUIRE(file.readAll(contents) == 9);
        REQUIRE(contents == "native fd");
    }

    // Borrowed means JobFile must leave the descriptor alive.
    REQUIRE(::fcntl(fd, F_GETFD) != -1);
    REQUIRE(::close(fd) == 0);
}

TEST_CASE("JobFile wraps a borrowed FILE pointer while using its native descriptor", "[job_io][file][usage][file_pointer]")
{
    const auto path = job::io::test::tmpPath("borrow_fp.bin");
    TransientTestFile tmp(path.string(), std::string_view("stdio bridge"));

    FILE *fp = ::fopen(path.c_str(), "rb");
    REQUIRE(fp != nullptr);

    const int fd = ::fileno(fp);
    REQUIRE(fd >= 0);

    {
        job::io::JobFile file(fp, false);

        REQUIRE(file.isOpen());
        REQUIRE(file.fp() == fp);
        REQUIRE(file.fd() == fd);
        REQUIRE(file.handle() == job::io::JobFile::Handle::FilePointer);
        REQUIRE_FALSE(file.owned());

        std::string contents;
        REQUIRE(file.readAll(contents) == 12);
        REQUIRE(contents == "stdio bridge");
    }

    REQUIRE(::fileno(fp) == fd);
    REQUIRE(::fclose(fp) == 0);
}

TEST_CASE("JobFile move construction transfers an open file resource", "[job_io][file][usage][move]")
{
    const auto path = job::io::test::tmpPath("move.bin");
    TransientTestFile tmp(path.string(), std::string_view("move me"));

    job::io::JobFile source(path);

    REQUIRE(source.openDevice());

    const int fd = source.fd();

    job::io::JobFile destination(std::move(source));

    REQUIRE(destination.isOpen());
    REQUIRE(destination.fd() == fd);
    REQUIRE(destination.owned());

    REQUIRE_FALSE(source.isOpen());
    REQUIRE(source.fd() == -1);
    REQUIRE(source.handle() == job::io::JobFile::Handle::None);

    std::string contents;
    REQUIRE(destination.readAll(contents) == 7);
    REQUIRE(contents == "move me");
}

TEST_CASE("JobFile factories create shared and unique file resources", "[job_io][file][usage][factory]")
{
    const auto sharedPath = job::io::test::tmpPath("shared_factory.bin");
    const auto uniquePath = job::io::test::tmpPath("unique_factory.bin");

    TransientTestFile sharedTmp(sharedPath.string());
    TransientTestFile uniqueTmp(uniquePath.string());

    const auto shared = job::io::JobFile::createShared(sharedPath);
    const auto unique = job::io::JobFile::createUniq(uniquePath);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->openDevice());
    REQUIRE(unique->openDevice());

    REQUIRE(shared->path() == sharedPath);
    REQUIRE(unique->path() == uniquePath);
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobFile default object has no open resource", "[job_io][file][edge]")
{
    job::io::JobFile file;

    REQUIRE_FALSE(file.isOpen());
    REQUIRE(file.fd() == -1);
    REQUIRE(file.handle() == job::io::JobFile::Handle::None);
    REQUIRE_FALSE(file.hasPath());
    REQUIRE_FALSE(file.owned());

    REQUIRE_FALSE(file.openDevice());
    REQUIRE(file.flush());
}

TEST_CASE("JobFile OpenExisting fails for a missing path", "[job_io][file][edge]")
{
    const auto path = job::io::test::tmpPath("missing.bin");
    std::filesystem::remove(path);

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadOnly,
        job::io::JobFile::OpenMode::OpenExisting);

    REQUIRE_FALSE(file.openDevice());
    REQUIRE_FALSE(file.isOpen());
    REQUIRE(file.fd() == -1);
}

TEST_CASE("JobFile Create preserves existing contents", "[job_io][file][edge][open_mode]")
{
    const auto path = job::io::test::tmpPath("create_preserve.bin");
    TransientTestFile tmp(path.string(), std::string_view("do not truncate"));

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::Create);

    REQUIRE(file.openDevice());
    REQUIRE(file.size() == 15);

    std::string contents;
    REQUIRE(file.readAll(contents) == 15);
    REQUIRE(contents == "do not truncate");
}

TEST_CASE("JobFile Truncate removes existing contents", "[job_io][file][edge][open_mode]")
{
    const auto path = job::io::test::tmpPath("truncate.bin");
    TransientTestFile tmp(path.string(), std::string_view("goodbye"));

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::Truncate);

    REQUIRE(file.openDevice());
    REQUIRE(file.size() == 0);
}

TEST_CASE("JobFile rejects incompatible read only open modes", "[job_io][file][edge][open_mode]")
{
    const auto truncatePath = job::io::test::tmpPath("readonly_truncate.bin");
    const auto appendPath = job::io::test::tmpPath("readonly_append.bin");

    TransientTestFile truncateTmp(truncatePath.string(), std::string_view("data"));
    TransientTestFile appendTmp(appendPath.string(), std::string_view("data"));

    job::io::JobFile truncate(
        truncatePath,
        job::io::JobFile::Access::ReadOnly,
        job::io::JobFile::OpenMode::Truncate);

    job::io::JobFile append(
        appendPath,
        job::io::JobFile::Access::ReadOnly,
        job::io::JobFile::OpenMode::Append);

    REQUIRE_FALSE(truncate.openDevice());
    REQUIRE_FALSE(append.openDevice());

    REQUIRE(std::filesystem::file_size(truncatePath) == 4);
    REQUIRE(std::filesystem::file_size(appendPath) == 4);
}

TEST_CASE("JobFile enforces descriptor access mode", "[job_io][file][edge][access]")
{
    const auto path = job::io::test::tmpPath("access.bin");
    TransientTestFile tmp(path.string(), std::string_view("hello"));

    SECTION("read only rejects writes")
    {
        job::io::JobFile file(path);

        REQUIRE(file.openDevice());

        char value{};
        REQUIRE(file.read(&value, 1) == 1);
        REQUIRE(file.write("x", 1) == -1);
    }

    SECTION("write only rejects reads")
    {
        job::io::JobFile file(
            path,
            job::io::JobFile::Access::WriteOnly,
            job::io::JobFile::OpenMode::OpenExisting);

        REQUIRE(file.openDevice());

        char value{};
        REQUIRE(file.read(&value, 1) == -1);
        REQUIRE(file.write("x", 1) == 1);
    }
}

TEST_CASE("JobFile handles zero length reads and writes", "[job_io][file][edge][zero]")
{
    const auto path = job::io::test::tmpPath("zero.bin");
    TransientTestFile tmp(path.string());

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::OpenExisting);

    REQUIRE(file.openDevice());

    char value{};

    REQUIRE(file.read(&value, 0) == 0);
    REQUIRE(file.write(&value, 0) == 0);
    REQUIRE(file.size() == 0);
}

TEST_CASE("JobFile rejects IO without an open resource", "[job_io][file][edge][closed]")
{
    job::io::JobFile file;

    char value{};

    REQUIRE(file.read(&value, 1) == -1);
    REQUIRE(file.write(&value, 1) == -1);
    REQUIRE(file.size() == 0);
    REQUIRE(file.tell() == std::numeric_limits<std::size_t>::max());
    REQUIRE_FALSE(file.seek(0, job::io::JobFile::Seek::Begin));
}

TEST_CASE("JobFile closeDevice is safe to call repeatedly", "[job_io][file][edge][close]")
{
    const auto path = job::io::test::tmpPath("close_twice.bin");
    TransientTestFile tmp(path.string());

    job::io::JobFile file(path);

    REQUIRE(file.openDevice());

    file.closeDevice();
    file.closeDevice();

    REQUIRE_FALSE(file.isOpen());
    REQUIRE(file.fd() == -1);
    REQUIRE(file.handle() == job::io::JobFile::Handle::None);

    // Closing the resource does not throw away the path identity.
    REQUIRE(file.path() == path);

    REQUIRE(file.openDevice());
}

TEST_CASE("JobFile does not close a borrowed native descriptor", "[job_io][file][edge][ownership]")
{
    const auto path = job::io::test::tmpPath("borrowed_fd.bin");
    TransientTestFile tmp(path.string());

    const int fd = ::open(path.c_str(), O_RDONLY);
    REQUIRE(fd >= 0);

    {
        job::io::JobFile file(fd, false);
        REQUIRE(file.isOpen());
    }

    REQUIRE(::fcntl(fd, F_GETFD) != -1);
    REQUIRE(::close(fd) == 0);
}

TEST_CASE("JobFile closes an owned native descriptor", "[job_io][file][edge][ownership]")
{
    const auto path = job::io::test::tmpPath("owned_fd.bin");
    TransientTestFile tmp(path.string());

    const int fd = ::open(path.c_str(), O_RDONLY);
    REQUIRE(fd >= 0);

    {
        job::io::JobFile file(fd, true);
        REQUIRE(file.isOpen());
        REQUIRE(file.owned());
    }

    errno = 0;

    REQUIRE(::fcntl(fd, F_GETFD) == -1);
    REQUIRE(errno == EBADF);
}

TEST_CASE("JobFile detects access from a supplied native descriptor", "[job_io][file][edge][fd]")
{
    const auto path = job::io::test::tmpPath("detect_access.bin");
    TransientTestFile tmp(path.string());

    const int readFd = ::open(path.c_str(), O_RDONLY);
    const int writeFd = ::open(path.c_str(), O_WRONLY);
    const int readWriteFd = ::open(path.c_str(), O_RDWR);

    REQUIRE(readFd >= 0);
    REQUIRE(writeFd >= 0);
    REQUIRE(readWriteFd >= 0);

    {
        job::io::JobFile readFile(readFd, false);
        job::io::JobFile writeFile(writeFd, false);
        job::io::JobFile readWriteFile(readWriteFd, false);

        REQUIRE(readFile.access() == job::io::JobFile::Access::ReadOnly);
        REQUIRE(writeFile.access() == job::io::JobFile::Access::WriteOnly);
        REQUIRE(readWriteFile.access() == job::io::JobFile::Access::ReadWrite);
    }

    REQUIRE(::close(readFd) == 0);
    REQUIRE(::close(writeFd) == 0);
    REQUIRE(::close(readWriteFd) == 0);
}

TEST_CASE("JobFile applies non blocking mode to its native descriptor", "[job_io][file][edge][nonblocking]")
{
    const auto path = job::io::test::tmpPath("nonblocking.bin");
    TransientTestFile tmp(path.string());

    job::io::JobFile file(
        path,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::OpenExisting);

    file.setNonBlocking(true);

    REQUIRE(file.openDevice());

    int flags = ::fcntl(file.fd(), F_GETFL, 0);
    REQUIRE(flags >= 0);
    REQUIRE((flags & O_NONBLOCK) != 0);

    file.setNonBlocking(false);

    flags = ::fcntl(file.fd(), F_GETFL, 0);
    REQUIRE(flags >= 0);
    REQUIRE((flags & O_NONBLOCK) == 0);
}

TEST_CASE("JobFile read callback receives successfully read bytes", "[job_io][file][edge][callback]")
{
    const auto path = job::io::test::tmpPath("callback.bin");
    TransientTestFile tmp(path.string(), std::string_view("callback"));

    job::io::JobFile file(path);

    REQUIRE(file.openDevice());

    std::string callbackData;

    file.setReadCallback([&callbackData](const char *data, std::size_t size) {
        callbackData.append(data, size);
    });

    char buffer[4]{};

    REQUIRE(file.read(buffer, sizeof(buffer)) == 4);
    REQUIRE(callbackData == "call");

    REQUIRE(file.read(buffer, sizeof(buffer)) == 4);
    REQUIRE(callbackData == "callback");
}

TEST_CASE("JobFile move assignment releases the old owned resource", "[job_io][file][edge][move]")
{
    const auto firstPath = job::io::test::tmpPath("move_assign_first.bin");
    const auto secondPath = job::io::test::tmpPath("move_assign_second.bin");

    TransientTestFile firstTmp(firstPath.string());
    TransientTestFile secondTmp(secondPath.string());

    job::io::JobFile source(
        firstPath,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::OpenExisting);

    job::io::JobFile destination(
        secondPath,
        job::io::JobFile::Access::ReadWrite,
        job::io::JobFile::OpenMode::OpenExisting);

    REQUIRE(source.openDevice());
    REQUIRE(destination.openDevice());

    const int sourceFd = source.fd();
    const int destinationFd = destination.fd();

    destination = std::move(source);

    REQUIRE(destination.fd() == sourceFd);
    REQUIRE(destination.isOpen());

    REQUIRE_FALSE(source.isOpen());
    REQUIRE(source.fd() == -1);

    errno = 0;
    REQUIRE(::fcntl(destinationFd, F_GETFD) == -1);
    REQUIRE(errno == EBADF);
}