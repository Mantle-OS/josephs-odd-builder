#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_syslink.h>

namespace job::io::test {

namespace fs = std::filesystem;

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

        m_path = fs::temp_directory_path() /
                 ("job-syslink-test-" + std::to_string(::getpid()) + "-" + std::to_string(stamp));

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

TEST_CASE("JobSysLink represents a symbolic link path", "[job_io][syslink][usage][path]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path linkPath = temp.path() / "current";
    JobSysLink link(linkPath);

    REQUIRE(link.path() == linkPath);
    REQUIRE_FALSE(link.empty());
}

TEST_CASE("JobSysLink can change the represented path", "[job_io][syslink][usage][path]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path first = temp.path() / "first";
    const fs::path second = temp.path() / "second";

    JobSysLink link(first);
    REQUIRE(link.path() == first);

    link.setPath(second);
    REQUIRE(link.path() == second);
}

TEST_CASE("JobSysLink creates a symbolic link to a file", "[job_io][syslink][usage][create][file]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target.txt";
    const fs::path linkPath = temp.path() / "target-link";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "JOB";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.createFileLink(target));
    REQUIRE(link.exists());
    REQUIRE(link.isSysLink());
    REQUIRE_FALSE(link.isDangling());

    REQUIRE(link.target() == target);
    REQUIRE(link.canonicalTarget() == fs::canonical(target));
}

TEST_CASE("JobSysLink creates a symbolic link to a directory", "[job_io][syslink][usage][create][directory]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target-dir";
    const fs::path linkPath = temp.path() / "target-dir-link";

    REQUIRE(fs::create_directory(target));

    JobSysLink link(linkPath);

    REQUIRE(link.createDirectoryLink(target));
    REQUIRE(link.exists());
    REQUIRE(link.isSysLink());
    REQUIRE_FALSE(link.isDangling());

    REQUIRE(link.target() == target);
    REQUIRE(link.canonicalTarget() == fs::canonical(target));
}

TEST_CASE("JobSysLink generic create creates a symbolic link", "[job_io][syslink][usage][create]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";
    const fs::path linkPath = temp.path() / "link";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "generic";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.create(target));
    REQUIRE(link.exists());
    REQUIRE(link.isSysLink());
    REQUIRE(link.target() == target);
}

TEST_CASE("JobSysLink preserves a relative target exactly", "[job_io][syslink][usage][target][relative]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path targetDir = temp.path() / "versions";
    const fs::path linkDir = temp.path() / "current";
    const fs::path target = targetDir / "v2";
    const fs::path linkPath = linkDir / "app";
    const fs::path storedTarget = fs::path{"../versions/v2"};

    REQUIRE(fs::create_directories(targetDir));
    REQUIRE(fs::create_directories(linkDir));

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "v2";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.create(storedTarget));
    REQUIRE(link.target() == storedTarget);
}

TEST_CASE("JobSysLink resolves a relative target from the link parent",
          "[job_io][syslink][usage][target][resolve]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path targetDir = temp.path() / "lib";
    const fs::path linkDir = temp.path() / "current";
    const fs::path target = targetDir / "app";
    const fs::path linkPath = linkDir / "app";
    const fs::path storedTarget = fs::path{"../lib/app"};

    REQUIRE(fs::create_directories(targetDir));
    REQUIRE(fs::create_directories(linkDir));

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "app";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.create(storedTarget));

    REQUIRE(link.target() == storedTarget);
    REQUIRE(link.resolvedTarget() == target.lexically_normal());
    REQUIRE(link.canonicalTarget() == fs::canonical(target));
}

TEST_CASE("JobSysLink reports dangling symbolic links", "[job_io][syslink][usage][dangling]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path linkPath = temp.path() / "dangling";
    const fs::path target = temp.path() / "missing-target";

    JobSysLink link(linkPath);

    REQUIRE(link.create(target));

    /*
     * The directory entry itself exists even though following it reaches
     * nothing.
     */
    REQUIRE(link.exists());
    REQUIRE(link.isSysLink());
    REQUIRE(link.isDangling());

    REQUIRE(link.target() == target);
    REQUIRE(link.resolvedTarget() == target.lexically_normal());
    REQUIRE(link.canonicalTarget().empty());
}

TEST_CASE("JobSysLink remove deletes the link and preserves a file target",
          "[job_io][syslink][usage][remove][file]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "important.txt";
    const fs::path linkPath = temp.path() / "important-link";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "do not delete";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.create(target));
    REQUIRE(link.exists());

    REQUIRE(link.remove());

    REQUIRE_FALSE(link.exists());
    REQUIRE_FALSE(link.isSysLink());

    REQUIRE(fs::exists(target));
    REQUIRE(fs::is_regular_file(target));

    std::ifstream stream(target);
    REQUIRE(stream.is_open());

    std::string value;
    stream >> value;

    REQUIRE(value == "do");
}

TEST_CASE("JobSysLink remove deletes the link and preserves a directory target",
          "[job_io][syslink][usage][remove][directory]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "important-dir";
    const fs::path linkPath = temp.path() / "important-dir-link";

    REQUIRE(fs::create_directory(target));

    {
        std::ofstream stream(target / "payload.txt");
        REQUIRE(stream.is_open());
        stream << "payload";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.createDirectoryLink(target));
    REQUIRE(link.remove());

    REQUIRE_FALSE(link.exists());
    REQUIRE(fs::exists(target));
    REQUIRE(fs::is_directory(target));
    REQUIRE(fs::exists(target / "payload.txt"));
}

TEST_CASE("JobSysLink removes a dangling link", "[job_io][syslink][usage][remove][dangling]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path linkPath = temp.path() / "dangling";
    const fs::path target = temp.path() / "missing";

    JobSysLink link(linkPath);

    REQUIRE(link.create(target));
    REQUIRE(link.isDangling());

    REQUIRE(link.remove());

    REQUIRE_FALSE(link.exists());
    REQUIRE_FALSE(link.isSysLink());
}

TEST_CASE("JobSysLink static helpers operate without constructing an object",
          "[job_io][syslink][usage][static]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";
    const fs::path linkPath = temp.path() / "link";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "target";
    }

    REQUIRE(JobSysLink::create(linkPath, target));

    REQUIRE(JobSysLink::exists(linkPath));
    REQUIRE(JobSysLink::isSysLink(linkPath));
    REQUIRE_FALSE(JobSysLink::isDangling(linkPath));

    REQUIRE(JobSysLink::target(linkPath) == target);
    REQUIRE(JobSysLink::resolvedTarget(linkPath) == target.lexically_normal());
    REQUIRE(JobSysLink::canonicalTarget(linkPath) == fs::canonical(target));

    REQUIRE(JobSysLink::remove(linkPath));

    REQUIRE_FALSE(JobSysLink::exists(linkPath));
    REQUIRE(fs::exists(target));
}

TEST_CASE("JobSysLink factories create symbolic link objects", "[job_io][syslink][usage][factory]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path linkPath = temp.path() / "link";

    const auto shared = JobSysLink::createShared(linkPath);
    const auto unique = JobSysLink::createUniq(linkPath);

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);

    REQUIRE(shared->path() == linkPath);
    REQUIRE(unique->path() == linkPath);
}

// =============================================================================
// Block 2: Edge cases / failure behavior
// =============================================================================

TEST_CASE("JobSysLink default construction represents no path", "[job_io][syslink][edge][empty]")
{
    JobSysLink link;

    REQUIRE(link.empty());
    REQUIRE(link.path().empty());

    REQUIRE_FALSE(link.exists());
    REQUIRE_FALSE(link.isSysLink());
    REQUIRE_FALSE(link.isDangling());

    REQUIRE(link.target().empty());
    REQUIRE(link.resolvedTarget().empty());
    REQUIRE(link.canonicalTarget().empty());
}

TEST_CASE("JobSysLink rejects creation with an empty link path", "[job_io][syslink][edge][create]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "target";
    }

    JobSysLink link;

    REQUIRE_FALSE(link.create(target));
    REQUIRE_FALSE(link.createFileLink(target));
    REQUIRE_FALSE(link.createDirectoryLink(target));

    REQUIRE_FALSE(JobSysLink::create({}, target));
    REQUIRE_FALSE(JobSysLink::createFileLink({}, target));
    REQUIRE_FALSE(JobSysLink::createDirectoryLink({}, target));
}

TEST_CASE("JobSysLink rejects creation with an empty target", "[job_io][syslink][edge][create]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path linkPath = temp.path() / "link";
    const fs::path emptyTarget;

    JobSysLink link(linkPath);

    REQUIRE_FALSE(link.create(emptyTarget));
    REQUIRE_FALSE(link.createFileLink(emptyTarget));
    REQUIRE_FALSE(link.createDirectoryLink(emptyTarget));

    REQUIRE_FALSE(JobSysLink::create(linkPath, emptyTarget));
    REQUIRE_FALSE(JobSysLink::createFileLink(linkPath, emptyTarget));
    REQUIRE_FALSE(JobSysLink::createDirectoryLink(linkPath, emptyTarget));
}

TEST_CASE("JobSysLink does not replace an existing regular file",
          "[job_io][syslink][edge][create][existing]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";
    const fs::path occupied = temp.path() / "occupied";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "target";
    }

    {
        std::ofstream stream(occupied);
        REQUIRE(stream.is_open());
        stream << "existing";
    }

    JobSysLink link(occupied);

    REQUIRE_FALSE(link.create(target));
    REQUIRE_FALSE(link.isSysLink());
    REQUIRE(fs::is_regular_file(occupied));
}

TEST_CASE("JobSysLink does not replace an existing directory",
          "[job_io][syslink][edge][create][existing]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";
    const fs::path occupied = temp.path() / "occupied";

    REQUIRE(fs::create_directory(target));
    REQUIRE(fs::create_directory(occupied));

    JobSysLink link(occupied);

    REQUIRE_FALSE(link.createDirectoryLink(target));
    REQUIRE_FALSE(link.isSysLink());
    REQUIRE(fs::is_directory(occupied));
}

TEST_CASE("JobSysLink does not replace an existing dangling symlink",
          "[job_io][syslink][edge][create][dangling]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path occupied = temp.path() / "occupied";
    const fs::path firstTarget = temp.path() / "missing-one";
    const fs::path secondTarget = temp.path() / "missing-two";

    REQUIRE(JobSysLink::create(occupied, firstTarget));
    REQUIRE(JobSysLink::exists(occupied));
    REQUIRE(JobSysLink::isDangling(occupied));

    REQUIRE_FALSE(JobSysLink::create(occupied, secondTarget));

    REQUIRE(JobSysLink::target(occupied) == firstTarget);
}

TEST_CASE("JobSysLink remove rejects an empty path", "[job_io][syslink][edge][remove]")
{
    JobSysLink link;

    REQUIRE_FALSE(link.remove());
    REQUIRE_FALSE(JobSysLink::remove({}));
}

TEST_CASE("JobSysLink remove rejects a regular file", "[job_io][syslink][edge][remove][file]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path file = temp.path() / "file";

    {
        std::ofstream stream(file);
        REQUIRE(stream.is_open());
        stream << "keep me";
    }

    JobSysLink link(file);

    REQUIRE_FALSE(link.remove());
    REQUIRE(fs::exists(file));
    REQUIRE(fs::is_regular_file(file));
}

TEST_CASE("JobSysLink remove rejects a real directory", "[job_io][syslink][edge][remove][directory]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path dir = temp.path() / "directory";

    REQUIRE(fs::create_directory(dir));

    JobSysLink link(dir);

    REQUIRE_FALSE(link.remove());
    REQUIRE(fs::exists(dir));
    REQUIRE(fs::is_directory(dir));
}

TEST_CASE("JobSysLink target helpers reject non-links", "[job_io][syslink][edge][target]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path file = temp.path() / "file";

    {
        std::ofstream stream(file);
        REQUIRE(stream.is_open());
        stream << "data";
    }

    JobSysLink link(file);

    REQUIRE(link.target().empty());
    REQUIRE(link.resolvedTarget().empty());
    REQUIRE(link.canonicalTarget().empty());
}

TEST_CASE("JobSysLink resolved target does not require the target to exist",
          "[job_io][syslink][edge][target][dangling]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path linkDir = temp.path() / "links";
    const fs::path linkPath = linkDir / "current";
    const fs::path storedTarget = fs::path{"../missing/version"};
    const fs::path expected = (linkDir / storedTarget).lexically_normal();

    REQUIRE(fs::create_directory(linkDir));
    REQUIRE(JobSysLink::create(linkPath, storedTarget));

    REQUIRE(JobSysLink::isDangling(linkPath));
    REQUIRE(JobSysLink::target(linkPath) == storedTarget);
    REQUIRE(JobSysLink::resolvedTarget(linkPath) == expected);
    REQUIRE(JobSysLink::canonicalTarget(linkPath).empty());
}

TEST_CASE("JobSysLink follows a chain when requesting the canonical target",
          "[job_io][syslink][edge][target][chain]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";
    const fs::path middle = temp.path() / "middle";
    const fs::path first = temp.path() / "first";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "target";
    }

    REQUIRE(JobSysLink::create(middle, target));
    REQUIRE(JobSysLink::create(first, middle));

    REQUIRE(JobSysLink::isSysLink(first));
    REQUIRE(JobSysLink::target(first) == middle);
    REQUIRE(JobSysLink::canonicalTarget(first) == fs::canonical(target));
}

TEST_CASE("JobSysLink becomes dangling when its target is removed",
          "[job_io][syslink][edge][dangling][lifecycle]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";
    const fs::path linkPath = temp.path() / "link";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "target";
    }

    JobSysLink link(linkPath);

    REQUIRE(link.create(target));
    REQUIRE_FALSE(link.isDangling());

    REQUIRE(fs::remove(target));

    REQUIRE(link.exists());
    REQUIRE(link.isSysLink());
    REQUIRE(link.isDangling());
}

// =============================================================================
// Block 3: Benchmarks / stress
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

#ifndef JOB_CI_BUILD

TEST_CASE("JobSysLink survives repeated create and remove cycles",
          "[job_io][syslink][stress][lifecycle]")
{
    constexpr std::size_t ITERATIONS = 1000;

    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "target";
    }

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobSysLink lifecycle iteration: " << iteration);

        const fs::path linkPath = temp.path() / ("link-" + std::to_string(iteration));
        JobSysLink link(linkPath);

        REQUIRE(link.create(target));
        REQUIRE(link.exists());
        REQUIRE(link.isSysLink());
        REQUIRE_FALSE(link.isDangling());

        REQUIRE(link.remove());
        REQUIRE_FALSE(link.exists());

        REQUIRE(fs::exists(target));
    }
}

TEST_CASE("JobSysLink survives repeated dangling-link cycles",
          "[job_io][syslink][stress][dangling]")
{
    constexpr std::size_t ITERATIONS = 1000;

    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobSysLink dangling iteration: " << iteration);

        const fs::path linkPath = temp.path() / ("dangling-" + std::to_string(iteration));
        const fs::path target = temp.path() / ("missing-" + std::to_string(iteration));

        JobSysLink link(linkPath);

        REQUIRE(link.create(target));
        REQUIRE(link.exists());
        REQUIRE(link.isSysLink());
        REQUIRE(link.isDangling());

        REQUIRE(link.remove());
        REQUIRE_FALSE(link.exists());
    }
}

#endif

TEST_CASE("JobSysLink benchmarks", "[job_io][syslink][benchmark]")
{
    TemporaryDirectory temp;
    REQUIRE_FALSE(temp.path().empty());

    const fs::path target = temp.path() / "target";

    {
        std::ofstream stream(target);
        REQUIRE(stream.is_open());
        stream << "benchmark";
    }

    std::size_t iteration = 0;

    BENCHMARK("JobSysLink create and remove")
    {
        const fs::path linkPath = temp.path() / ("benchmark-" + std::to_string(iteration++));
        JobSysLink link(linkPath);

        const bool created = link.create(target);

        if (created)
            (void)link.remove();

        return created;
    };
}

#endif

} // namespace job::io::test
