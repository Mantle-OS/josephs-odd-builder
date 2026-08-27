#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#include <job_tmp_file.h>

namespace job::io::test {

[[nodiscard]] std::filesystem::path tmpFilePath(std::string_view name)
{
    return std::filesystem::temp_directory_path() /
           ("job_tmp_file_" + std::to_string(::getpid()) + "_" + std::string(name));
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobTmpFile creates an empty temporary file", "[job_io][tmp_file][usage]")
{
    const auto path = job::io::test::tmpFilePath("empty.bin");

    {
        job::io::JobTmpFile tmp(path);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.path() == path);
        REQUIRE(tmp.file().isOpen());
        REQUIRE(tmp.file().size() == 0);
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile creates a temporary file filled with a pattern", "[job_io][tmp_file][usage][pattern]")
{
    const auto path = job::io::test::tmpFilePath("pattern.bin");

    {
        job::io::JobTmpFile tmp(path, 4096, 'A');

        REQUIRE(tmp.exists());
        REQUIRE(tmp.file().size() == 4096);

        std::string contents;
        REQUIRE(tmp.file().readAll(contents) == 4096);
        REQUIRE(contents.size() == 4096);

        for (const char value : contents)
            REQUIRE(value == 'A');
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile creates a temporary file from string data", "[job_io][tmp_file][usage][string]")
{
    const auto path = job::io::test::tmpFilePath("string.bin");

    {
        job::io::JobTmpFile tmp(path, std::string_view("hello from JobTmpFile"));

        REQUIRE(tmp.exists());
        REQUIRE(tmp.file().size() == 21);

        std::string contents;
        REQUIRE(tmp.file().readAll(contents) == 21);
        REQUIRE(contents == "hello from JobTmpFile");
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile creates a temporary file from binary data", "[job_io][tmp_file][usage][binary]")
{
    const auto path = job::io::test::tmpFilePath("binary.bin");

    const std::vector<std::byte> data{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x7F},
        std::byte{0x80},
        std::byte{0xFF}
    };

    {
        job::io::JobTmpFile tmp(path, data);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.file().size() == data.size());

        std::vector<std::uint8_t> contents;
        REQUIRE(tmp.file().readAll(contents) == static_cast<ssize_t>(data.size()));

        REQUIRE(contents == std::vector<std::uint8_t>{0x00, 0x01, 0x7F, 0x80, 0xFF});
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile exposes its JobFile for normal file IO", "[job_io][tmp_file][usage][job_file]")
{
    const auto path = job::io::test::tmpFilePath("job_file.bin");

    {
        job::io::JobTmpFile tmp(path, std::string_view("hello"));

        REQUIRE(tmp.file().seek(0, job::io::JobFile::Seek::End));
        REQUIRE(tmp.file().write(" world", 6) == 6);

        REQUIRE(tmp.file().seek(0, job::io::JobFile::Seek::Begin));

        std::string contents;
        REQUIRE(tmp.file().readAll(contents) == 11);
        REQUIRE(contents == "hello world");
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile factories create temporary files", "[job_io][tmp_file][usage][factory]")
{
    const auto sharedPath = job::io::test::tmpFilePath("shared.bin");
    const auto uniquePath = job::io::test::tmpFilePath("unique.bin");

    {
        const auto shared = job::io::JobTmpFile::createShared(sharedPath, std::string_view("shared"));
        const auto unique = job::io::JobTmpFile::createUniq(uniquePath, std::string_view("unique"));

        REQUIRE(shared);
        REQUIRE(unique);

        REQUIRE(shared->exists());
        REQUIRE(unique->exists());

        REQUIRE(shared->file().size() == 6);
        REQUIRE(unique->file().size() == 6);
    }

    REQUIRE_FALSE(std::filesystem::exists(sharedPath));
    REQUIRE_FALSE(std::filesystem::exists(uniquePath));
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobTmpFile zero sized pattern creates an empty file", "[job_io][tmp_file][edge][zero]")
{
    const auto path = job::io::test::tmpFilePath("zero_pattern.bin");

    {
        job::io::JobTmpFile tmp(path, 0, 'X');

        REQUIRE(tmp.exists());
        REQUIRE(tmp.file().size() == 0);
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile empty string creates an empty file", "[job_io][tmp_file][edge][empty]")
{
    const auto path = job::io::test::tmpFilePath("empty_string.bin");

    {
        job::io::JobTmpFile tmp(path, std::string_view{});

        REQUIRE(tmp.exists());
        REQUIRE(tmp.file().size() == 0);
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile empty binary data creates an empty file", "[job_io][tmp_file][edge][empty]")
{
    const auto path = job::io::test::tmpFilePath("empty_binary.bin");

    const std::vector<std::byte> data;

    {
        job::io::JobTmpFile tmp(path, data);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.file().size() == 0);
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile rewinds seeded files after construction", "[job_io][tmp_file][edge][rewind]")
{
    const auto path = job::io::test::tmpFilePath("rewound.bin");

    job::io::JobTmpFile tmp(path, std::string_view("abcdef"));

    REQUIRE(tmp.file().tell() == 0);

    char value{};
    REQUIRE(tmp.file().read(&value, 1) == 1);
    REQUIRE(value == 'a');
}

TEST_CASE("JobTmpFile can be rewound explicitly", "[job_io][tmp_file][edge][rewind]")
{
    const auto path = job::io::test::tmpFilePath("explicit_rewind.bin");

    job::io::JobTmpFile tmp(path, std::string_view("abcdef"));

    REQUIRE(tmp.file().seek(4, job::io::JobFile::Seek::Begin));
    REQUIRE(tmp.file().tell() == 4);

    tmp.rewind();

    REQUIRE(tmp.file().tell() == 0);
}

TEST_CASE("JobTmpFile removes an existing destination by replacing its contents", "[job_io][tmp_file][edge][truncate]")
{
    const auto path = job::io::test::tmpFilePath("truncate_existing.bin");

    {
        job::io::JobFile file(
            path,
            job::io::JobFile::Access::ReadWrite,
            job::io::JobFile::OpenMode::Truncate);

        REQUIRE(file.openDevice());
        REQUIRE(file.write("old contents", 12) == 12);
    }

    {
        job::io::JobTmpFile tmp(path, std::string_view("new"));

        REQUIRE(tmp.file().size() == 3);

        std::string contents;
        REQUIRE(tmp.file().readAll(contents) == 3);
        REQUIRE(contents == "new");
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile can preserve the file after destruction", "[job_io][tmp_file][edge][cleanup]")
{
    const auto path = job::io::test::tmpFilePath("preserve.bin");

    {
        job::io::JobTmpFile tmp(path, std::string_view("keep me"));

        REQUIRE(tmp.removeOnDestroy());

        tmp.setRemoveOnDestroy(false);

        REQUIRE_FALSE(tmp.removeOnDestroy());
    }

    REQUIRE(std::filesystem::exists(path));

    job::io::JobFile file(path);

    REQUIRE(file.openDevice());

    std::string contents;
    REQUIRE(file.readAll(contents) == 7);
    REQUIRE(contents == "keep me");

    file.closeDevice();
    REQUIRE(std::filesystem::remove(path));
}

TEST_CASE("JobTmpFile move construction transfers cleanup ownership", "[job_io][tmp_file][edge][move]")
{
    const auto path = job::io::test::tmpFilePath("move_construct.bin");

    {
        job::io::JobTmpFile source(path, std::string_view("move me"));

        REQUIRE(source.exists());

        job::io::JobTmpFile destination(std::move(source));

        REQUIRE(destination.exists());
        REQUIRE(destination.path() == path);
        REQUIRE(destination.removeOnDestroy());

        REQUIRE(source.path().empty());
        REQUIRE_FALSE(source.removeOnDestroy());

        std::string contents;
        REQUIRE(destination.file().readAll(contents) == 7);
        REQUIRE(contents == "move me");
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpFile move assignment cleans old file and transfers cleanup ownership", "[job_io][tmp_file][edge][move]")
{
    const auto sourcePath = job::io::test::tmpFilePath("move_assign_source.bin");
    const auto destinationPath = job::io::test::tmpFilePath("move_assign_destination.bin");

    {
        job::io::JobTmpFile source(sourcePath, std::string_view("source"));
        job::io::JobTmpFile destination(destinationPath, std::string_view("destination"));

        REQUIRE(std::filesystem::exists(sourcePath));
        REQUIRE(std::filesystem::exists(destinationPath));

        destination = std::move(source);

        REQUIRE(destination.path() == sourcePath);
        REQUIRE(destination.exists());

        REQUIRE_FALSE(std::filesystem::exists(destinationPath));

        REQUIRE(source.path().empty());
        REQUIRE_FALSE(source.removeOnDestroy());

        std::string contents;
        REQUIRE(destination.file().readAll(contents) == 6);
        REQUIRE(contents == "source");
    }

    REQUIRE_FALSE(std::filesystem::exists(sourcePath));
    REQUIRE_FALSE(std::filesystem::exists(destinationPath));
}

TEST_CASE("JobTmpFile writes large patterns without requiring one giant buffer", "[job_io][tmp_file][edge][pattern]")
{
    constexpr std::size_t size = 1024 * 1024 + 123;

    const auto path = job::io::test::tmpFilePath("large_pattern.bin");

    {
        job::io::JobTmpFile tmp(path, size, std::byte{0x5A});

        REQUIRE(tmp.file().size() == size);

        std::vector<std::uint8_t> contents;
        REQUIRE(tmp.file().readAll(contents) == static_cast<ssize_t>(size));
        REQUIRE(contents.size() == size);

        REQUIRE(contents.front() == 0x5A);
        REQUIRE(contents[size / 2] == 0x5A);
        REQUIRE(contents.back() == 0x5A);
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}