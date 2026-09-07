#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_dir.h>

namespace job::io::test {

namespace fs = std::filesystem;

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

        m_path = fs::temp_directory_path() /
                 ("job-dir-test-" + std::to_string(::getpid()) + "-" + std::to_string(stamp));

        std::error_code ec;
        fs::create_directories(m_path, ec);

        if (ec)
            m_path.clear();
    }

    ~TemporaryDirectory()
    {
        if (m_path.empty())
            return;

        std::error_code ec;
        fs::remove_all(m_path, ec);
    }

    TemporaryDirectory(const TemporaryDirectory &) = delete;
    TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;
    TemporaryDirectory(TemporaryDirectory &&) = delete;
    TemporaryDirectory &operator=(TemporaryDirectory &&) = delete;

    [[nodiscard]] const fs::path &path() const noexcept
    {
        return m_path;
    }

private:
    fs::path m_path;
};

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobDir represents a filesystem directory", "[job_io][dir][usage][path]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path());

    REQUIRE(dir.path() == temp.path());
    REQUIRE_FALSE(dir.empty());
    REQUIRE(dir.exists());
    REQUIRE(dir.isDirectory());
    REQUIRE(dir.isAbsolute());
    REQUIRE_FALSE(dir.isRelative());
}

TEST_CASE("JobDir can change the represented path", "[job_io][dir][usage][path]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path first = temp.path() / "first";
    const fs::path second = temp.path() / "second";

    JobDir dir(first);
    REQUIRE(dir.path() == first);

    dir.setPath(second);
    REQUIRE(dir.path() == second);
}

TEST_CASE("JobDir creates a directory", "[job_io][dir][usage][create]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "example");

    REQUIRE_FALSE(dir.exists());
    REQUIRE(dir.create());
    REQUIRE(dir.exists());
    REQUIRE(dir.isDirectory());
}

TEST_CASE("JobDir creates a directory with explicit permissions", "[job_io][dir][usage][create][permissions]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "private");

    REQUIRE(dir.create(IOPermissions::PrivateDirectory));
    REQUIRE(dir.exists());
    REQUIRE(dir.isDirectory());
    REQUIRE(dir.hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE(dir.permissions() == IOPermissions::PrivateDirectory);
}

TEST_CASE("JobDir creates missing parent directories", "[job_io][dir][usage][create][parents]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path path = temp.path() / "one" / "two" / "three";
    JobDir dir(path);

    REQUIRE_FALSE(dir.exists());
    REQUIRE(dir.createParents());
    REQUIRE(dir.exists());
    REQUIRE(dir.isDirectory());
    REQUIRE(JobDir::isDirectory(temp.path() / "one"));
    REQUIRE(JobDir::isDirectory(temp.path() / "one" / "two"));
}

TEST_CASE("JobDir creates missing parent directories with explicit permissions",
          "[job_io][dir][usage][create][parents][permissions]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path first = temp.path() / "private-a";
    const fs::path second = first / "private-b";
    const fs::path third = second / "private-c";

    REQUIRE(JobDir::createParents(third, IOPermissions::PrivateDirectory));
    REQUIRE(JobDir(first).hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE(JobDir(second).hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE(JobDir(third).hasPermissions(IOPermissions::PrivateDirectory));
}

TEST_CASE("JobDir permission creation does not modify an existing directory",
          "[job_io][dir][usage][create][permissions][existing]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "existing");

    REQUIRE(dir.create(IOPermissions::DefaultDirectory));
    REQUIRE(dir.hasPermissions(IOPermissions::DefaultDirectory));

    REQUIRE(dir.create(IOPermissions::PrivateDirectory));
    REQUIRE(dir.hasPermissions(IOPermissions::DefaultDirectory));
}

TEST_CASE("JobDir can inspect and change directory permissions", "[job_io][dir][usage][permissions]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "permissions");

    REQUIRE(dir.create(IOPermissions::PrivateDirectory));
    REQUIRE(dir.permissions() == IOPermissions::PrivateDirectory);

    REQUIRE(dir.setPermissions(IOPermissions::DefaultDirectory));
    REQUIRE(dir.permissions() == IOPermissions::DefaultDirectory);
    REQUIRE(dir.hasPermissions(IOPermissions::DefaultDirectory));
    REQUIRE_FALSE(dir.hasPermissions(IOPermissions::PrivateDirectory));
}

TEST_CASE("JobDir reports basic directory accessibility", "[job_io][dir][usage][access]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "accessible");

    REQUIRE(dir.create(IOPermissions::PrivateDirectory));
    REQUIRE(dir.isReadable());
    REQUIRE(dir.isWritable());
    REQUIRE(dir.isExecutable());
}

TEST_CASE("JobDir enumerates directory entries", "[job_io][dir][usage][entries]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path root = temp.path() / "entries";
    REQUIRE(JobDir::create(root));

    {
        std::ofstream stream(root / "file.txt");
        REQUIRE(stream.is_open());
        stream << "JOB";
    }

    REQUIRE(JobDir::create(root / "subdir"));

    const auto entries = JobDir(root).entries();
    REQUIRE(entries.size() == 2);
}

TEST_CASE("JobDir separates files from directories", "[job_io][dir][usage][entries]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path root = temp.path() / "contents";
    REQUIRE(JobDir::create(root));

    {
        std::ofstream first(root / "one.txt");
        std::ofstream second(root / "two.txt");
        REQUIRE(first.is_open());
        REQUIRE(second.is_open());
        first << "one";
        second << "two";
    }

    REQUIRE(JobDir::create(root / "first"));
    REQUIRE(JobDir::create(root / "second"));

    JobDir dir(root);
    REQUIRE(dir.files().size() == 2);
    REQUIRE(dir.directories().size() == 2);
}

TEST_CASE("JobDir reports whether a directory is empty", "[job_io][dir][usage][empty]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "empty");

    REQUIRE(dir.create());
    REQUIRE(dir.isEmpty());

    {
        std::ofstream stream(dir.path() / "data.txt");
        REQUIRE(stream.is_open());
        stream << "data";
    }

    REQUIRE_FALSE(dir.isEmpty());
}

TEST_CASE("JobDir removes an empty directory", "[job_io][dir][usage][remove]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "remove-me");

    REQUIRE(dir.create());
    REQUIRE(dir.remove());
    REQUIRE_FALSE(dir.exists());
}

TEST_CASE("JobDir recursively removes a directory tree", "[job_io][dir][usage][remove][recursive]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path root = temp.path() / "tree";
    const fs::path leaf = root / "one" / "two" / "three";

    REQUIRE(JobDir::createParents(leaf));

    {
        std::ofstream stream(leaf / "payload.txt");
        REQUIRE(stream.is_open());
        stream << "payload";
    }

    JobDir dir(root);
    REQUIRE(dir.removeRecursive());
    REQUIRE_FALSE(dir.exists());
}

TEST_CASE("JobDir static helpers operate without constructing an object", "[job_io][dir][usage][static]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path path = temp.path() / "static";

    REQUIRE_FALSE(JobDir::exists(path));
    REQUIRE(JobDir::create(path));
    REQUIRE(JobDir::exists(path));
    REQUIRE(JobDir::isDirectory(path));
    REQUIRE(JobDir::isEmpty(path));
    REQUIRE(JobDir::remove(path));
    REQUIRE_FALSE(JobDir::exists(path));
}

TEST_CASE("JobDir factories create directory objects", "[job_io][dir][usage][factory]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const auto shared = JobDir::createShared(temp.path());
    const auto unique = JobDir::createUniq(temp.path());

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);
    REQUIRE(shared->path() == temp.path());
    REQUIRE(unique->path() == temp.path());
}

// =============================================================================
// Block 2: Edge cases / failure behavior
// =============================================================================

TEST_CASE("JobDir default construction represents no path", "[job_io][dir][edge][empty]")
{
    JobDir dir;

    REQUIRE(dir.empty());
    REQUIRE(dir.path().empty());
    REQUIRE_FALSE(dir.exists());
    REQUIRE_FALSE(dir.isDirectory());
    REQUIRE_FALSE(dir.isEmpty());
    REQUIRE_FALSE(dir.isReadable());
    REQUIRE_FALSE(dir.isWritable());
    REQUIRE_FALSE(dir.isExecutable());
    REQUIRE_FALSE(dir.isAbsolute());
    REQUIRE_FALSE(dir.isRelative());
    REQUIRE(dir.permissions() == IOPermissions::None);
}

TEST_CASE("JobDir rejects directory creation with an empty path", "[job_io][dir][edge][create]")
{
    const JobDir::Path emptyPath;
    JobDir dir;

    REQUIRE_FALSE(dir.create());
    REQUIRE_FALSE(dir.create(IOPermissions::PrivateDirectory));
    REQUIRE_FALSE(dir.createParents());
    REQUIRE_FALSE(dir.createParents(IOPermissions::PrivateDirectory));
    REQUIRE_FALSE(JobDir::create(emptyPath));
    REQUIRE_FALSE(JobDir::create(emptyPath, IOPermissions::PrivateDirectory));
    REQUIRE_FALSE(JobDir::createParents(emptyPath));
    REQUIRE_FALSE(JobDir::createParents(emptyPath, IOPermissions::PrivateDirectory));
}

TEST_CASE("JobDir rejects removal with an empty path", "[job_io][dir][edge][remove]")
{
    const JobDir::Path emptyPath;
    JobDir dir;

    REQUIRE_FALSE(dir.remove());
    REQUIRE_FALSE(dir.removeRecursive());
    REQUIRE_FALSE(JobDir::remove(emptyPath));
    REQUIRE_FALSE(JobDir::removeRecursive(emptyPath));
}

TEST_CASE("JobDir does not treat a regular file as a directory", "[job_io][dir][edge][file]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path file = temp.path() / "regular-file";

    {
        std::ofstream stream(file);
        REQUIRE(stream.is_open());
        stream << "not a directory";
    }

    JobDir dir(file);

    REQUIRE(dir.exists());
    REQUIRE_FALSE(dir.isDirectory());
    REQUIRE_FALSE(dir.isEmpty());
    REQUIRE_FALSE(JobDir::create(file));
    REQUIRE_FALSE(JobDir::create(file, IOPermissions::PrivateDirectory));
    REQUIRE_FALSE(JobDir::createParents(file));
    REQUIRE_FALSE(JobDir::createParents(file, IOPermissions::PrivateDirectory));
}

TEST_CASE("JobDir cannot remove a non-empty directory non-recursively", "[job_io][dir][edge][remove]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "non-empty");
    REQUIRE(dir.create());

    {
        std::ofstream stream(dir.path() / "file.txt");
        REQUIRE(stream.is_open());
        stream << "data";
    }

    REQUIRE_FALSE(dir.remove());
    REQUIRE(dir.exists());
}

TEST_CASE("JobDir remove reports false for a missing directory", "[job_io][dir][edge][remove]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path missing = temp.path() / "missing";

    REQUIRE_FALSE(JobDir::remove(missing));
    REQUIRE_FALSE(JobDir::removeRecursive(missing));
}

TEST_CASE("JobDir entry enumeration returns empty for invalid paths", "[job_io][dir][edge][entries]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir missing(temp.path() / "missing");
    REQUIRE(missing.entries().empty());
    REQUIRE(missing.files().empty());
    REQUIRE(missing.directories().empty());

    const fs::path file = temp.path() / "file";
    {
        std::ofstream stream(file);
        REQUIRE(stream.is_open());
        stream << "data";
    }

    JobDir regularFile(file);
    REQUIRE(regularFile.entries().empty());
    REQUIRE(regularFile.files().empty());
    REQUIRE(regularFile.directories().empty());
}

TEST_CASE("JobDir permission operations reject a missing directory", "[job_io][dir][edge][permissions]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    JobDir dir(temp.path() / "missing");

    REQUIRE(dir.permissions() == IOPermissions::None);
    REQUIRE_FALSE(dir.hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE_FALSE(dir.setPermissions(IOPermissions::PrivateDirectory));
}

TEST_CASE("JobDir distinguishes absolute and relative paths", "[job_io][dir][edge][path]")
{
    JobDir absolute(fs::temp_directory_path());
    JobDir relative("some/relative/path");

    REQUIRE(absolute.isAbsolute());
    REQUIRE_FALSE(absolute.isRelative());
    REQUIRE(relative.isRelative());
    REQUIRE_FALSE(relative.isAbsolute());
}

TEST_CASE("JobDir createParents preserves existing parent permissions",
          "[job_io][dir][edge][create][parents][permissions]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path existing = temp.path() / "existing";
    const fs::path child = existing / "child";
    const fs::path grandchild = child / "grandchild";

    JobDir existingDir(existing);

    REQUIRE(existingDir.create(IOPermissions::DefaultDirectory));
    REQUIRE(existingDir.hasPermissions(IOPermissions::DefaultDirectory));
    REQUIRE(JobDir::createParents(grandchild, IOPermissions::PrivateDirectory));

    REQUIRE(existingDir.hasPermissions(IOPermissions::DefaultDirectory));
    REQUIRE(JobDir(child).hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE(JobDir(grandchild).hasPermissions(IOPermissions::PrivateDirectory));
}

// =============================================================================
// Block 3: Benchmarks / stress
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

#ifndef JOB_CI_BUILD

TEST_CASE("JobDir survives repeated create and remove cycles", "[job_io][dir][stress][lifecycle]")
{
    constexpr std::size_t ITERATIONS = 500;

    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobDir lifecycle iteration: " << iteration);

        JobDir dir(temp.path() / ("dir-" + std::to_string(iteration)));

        REQUIRE(dir.create(IOPermissions::PrivateDirectory));
        REQUIRE(dir.exists());
        REQUIRE(dir.hasPermissions(IOPermissions::PrivateDirectory));
        REQUIRE(dir.remove());
        REQUIRE_FALSE(dir.exists());
    }
}

TEST_CASE("JobDir survives repeated recursive directory trees", "[job_io][dir][stress][recursive]")
{
    constexpr std::size_t ITERATIONS = 100;

    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobDir recursive iteration: " << iteration);

        const fs::path root = temp.path() / ("tree-" + std::to_string(iteration));
        const fs::path leaf = root / "a" / "b" / "c" / "d";

        REQUIRE(JobDir::createParents(leaf, IOPermissions::PrivateDirectory));
        REQUIRE(JobDir::isDirectory(leaf));
        REQUIRE(JobDir::removeRecursive(root));
        REQUIRE_FALSE(JobDir::exists(root));
    }
}

#endif

TEST_CASE("JobDir benchmarks", "[job_io][dir][benchmark]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    std::size_t iteration = 0;

    BENCHMARK("JobDir create and remove")
    {
        JobDir dir(temp.path() / ("benchmark-" + std::to_string(iteration++)));

        const bool created = dir.create(IOPermissions::PrivateDirectory);

        if (created)
            static_cast<void>(dir.remove());

        return created;
    };
}

#endif

} // namespace job::io::test
