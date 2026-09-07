#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>

#include <job_tmp_dir.h>
namespace job::io::test {

[[nodiscard]] std::filesystem::path tmpDirPath(std::string_view name)
{
    return std::filesystem::temp_directory_path() /
           ("job_tmp_dir_" + std::to_string(::getpid()) + "_" + std::string(name));
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobTmpDir creates an empty temporary directory",
          "[job_io][tmp_dir][usage]")
{
    const auto path = job::io::test::tmpDirPath("empty");

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.path() == path);

        REQUIRE(tmp.dir().exists());
        REQUIRE(tmp.dir().isDirectory());
        REQUIRE(tmp.dir().isEmpty());

        REQUIRE(tmp.dir().hasPermissions(
            job::io::IOPermissions::PrivateDirectory));
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir creates missing parent directories",
          "[job_io][tmp_dir][usage][parents]")
{
    const auto root = job::io::test::tmpDirPath("parents");
    const auto path = root / "one" / "two" / "three";

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.path() == path);

        REQUIRE(std::filesystem::is_directory(root));
        REQUIRE(std::filesystem::is_directory(root / "one"));
        REQUIRE(std::filesystem::is_directory(root / "one/two"));
        REQUIRE(std::filesystem::is_directory(path));
    }

    REQUIRE_FALSE(std::filesystem::exists(path));

    /*
     * JobTmpDir owns the directory represented by its complete path.
     * Recursive cleanup therefore removes the entire created subtree rooted
     * at that path, but parents above that owned path remain.
     */
    REQUIRE(std::filesystem::exists(root));

    std::filesystem::remove_all(root);
}

TEST_CASE("JobTmpDir accepts explicit directory permissions",
          "[job_io][tmp_dir][usage][permissions]")
{
    const auto path = job::io::test::tmpDirPath("permissions");

    {
        job::io::JobTmpDir tmp(
            path,
            job::io::IOPermissions::DefaultDirectory);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.dir().hasPermissions(
            job::io::IOPermissions::DefaultDirectory));
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir exposes its JobDir for normal directory operations",
          "[job_io][tmp_dir][usage][job_dir]")
{
    const auto path = job::io::test::tmpDirPath("job_dir");

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.dir().exists());
        REQUIRE(tmp.dir().isDirectory());
        REQUIRE(tmp.dir().isEmpty());

        const auto child = tmp.path() / "child";

        job::io::JobDir childDir(child);

        REQUIRE(childDir.create());
        REQUIRE(childDir.exists());
        REQUIRE(childDir.isDirectory());

        REQUIRE_FALSE(tmp.dir().isEmpty());

        const auto directories = tmp.dir().directories();

        REQUIRE(directories.size() == 1);
        REQUIRE(directories.front() == child);
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir pathString returns its directory path",
          "[job_io][tmp_dir][usage][path]")
{
    const auto path = job::io::test::tmpDirPath("path_string");

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.path() == path);
        REQUIRE(tmp.pathString() == path.string());
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir factories create temporary directories",
          "[job_io][tmp_dir][usage][factory]")
{
    const auto sharedPath =
        job::io::test::tmpDirPath("shared");

    const auto uniquePath =
        job::io::test::tmpDirPath("unique");

    {
        const auto shared =
            job::io::JobTmpDir::createShared(sharedPath);

        const auto unique =
            job::io::JobTmpDir::createUniq(uniquePath);

        REQUIRE(shared);
        REQUIRE(unique);

        REQUIRE(shared->exists());
        REQUIRE(unique->exists());

        REQUIRE(shared->path() == sharedPath);
        REQUIRE(unique->path() == uniquePath);

        REQUIRE(shared->dir().isDirectory());
        REQUIRE(unique->dir().isDirectory());
    }

    REQUIRE_FALSE(std::filesystem::exists(sharedPath));
    REQUIRE_FALSE(std::filesystem::exists(uniquePath));
}

TEST_CASE("JobTmpDir recursively removes directory contents on destruction",
          "[job_io][tmp_dir][usage][cleanup]")
{
    const auto path =
        job::io::test::tmpDirPath("recursive_cleanup");

    {
        job::io::JobTmpDir tmp(path);

        const auto one = tmp.path() / "one";
        const auto two = one / "two";
        const auto three = two / "three";

        REQUIRE(job::io::JobDir::createParents(three));

        REQUIRE(std::filesystem::exists(one));
        REQUIRE(std::filesystem::exists(two));
        REQUIRE(std::filesystem::exists(three));
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobTmpDir can preserve the directory after destruction",
          "[job_io][tmp_dir][edge][cleanup]")
{
    const auto path =
        job::io::test::tmpDirPath("preserve");

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.removeOnDestroy());

        tmp.setRemoveOnDestroy(false);

        REQUIRE_FALSE(tmp.removeOnDestroy());
    }

    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::is_directory(path));

    REQUIRE(job::io::JobDir::removeRecursive(path));
}

TEST_CASE("JobTmpDir preserved directory keeps its contents",
          "[job_io][tmp_dir][edge][cleanup]")
{
    const auto path =
        job::io::test::tmpDirPath("preserve_contents");

    const auto child = path / "child";

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(job::io::JobDir::create(child));

        tmp.setRemoveOnDestroy(false);
    }

    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::exists(child));

    REQUIRE(job::io::JobDir::removeRecursive(path));
}

TEST_CASE("JobTmpDir move construction transfers cleanup ownership",
          "[job_io][tmp_dir][edge][move]")
{
    const auto path =
        job::io::test::tmpDirPath("move_construct");

    {
        job::io::JobTmpDir source(path);

        REQUIRE(source.exists());

        const auto child = source.path() / "child";

        REQUIRE(job::io::JobDir::create(child));

        job::io::JobTmpDir destination(std::move(source));

        REQUIRE(destination.exists());
        REQUIRE(destination.path() == path);
        REQUIRE(destination.removeOnDestroy());

        REQUIRE(destination.dir().exists());
        REQUIRE(std::filesystem::exists(child));

        REQUIRE(source.path().empty());
        REQUIRE(source.dir().empty());
        REQUIRE_FALSE(source.exists());
        REQUIRE_FALSE(source.removeOnDestroy());
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir move assignment cleans old directory and transfers cleanup ownership",
          "[job_io][tmp_dir][edge][move]")
{
    const auto sourcePath =
        job::io::test::tmpDirPath("move_assign_source");

    const auto destinationPath =
        job::io::test::tmpDirPath("move_assign_destination");

    {
        job::io::JobTmpDir source(sourcePath);
        job::io::JobTmpDir destination(destinationPath);

        REQUIRE(std::filesystem::exists(sourcePath));
        REQUIRE(std::filesystem::exists(destinationPath));

        const auto sourceChild = sourcePath / "source_child";
        const auto destinationChild = destinationPath / "destination_child";

        REQUIRE(job::io::JobDir::create(sourceChild));
        REQUIRE(job::io::JobDir::create(destinationChild));

        destination = std::move(source);

        REQUIRE(destination.path() == sourcePath);
        REQUIRE(destination.exists());
        REQUIRE(destination.removeOnDestroy());

        /*
         * The directory previously owned by destination must be cleaned during
         * move assignment before it assumes ownership of sourcePath.
         */
        REQUIRE_FALSE(std::filesystem::exists(destinationPath));

        REQUIRE(std::filesystem::exists(sourceChild));

        REQUIRE(source.path().empty());
        REQUIRE(source.dir().empty());
        REQUIRE_FALSE(source.exists());
        REQUIRE_FALSE(source.removeOnDestroy());
    }

    REQUIRE_FALSE(std::filesystem::exists(sourcePath));
    REQUIRE_FALSE(std::filesystem::exists(destinationPath));
}

TEST_CASE("JobTmpDir cleanup handles an already removed directory",
          "[job_io][tmp_dir][edge][cleanup]")
{
    const auto path =
        job::io::test::tmpDirPath("already_removed");

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.dir().removeRecursive());

        REQUIRE_FALSE(tmp.exists());

        /*
         * Destruction must remain harmless even though the resource has
         * already been removed manually.
         */
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir constructor accepts an existing directory",
          "[job_io][tmp_dir][edge][existing]")
{
    const auto path =
        job::io::test::tmpDirPath("existing");

    REQUIRE(job::io::JobDir::create(
        path,
        job::io::IOPermissions::DefaultDirectory));

    {
        job::io::JobTmpDir tmp(path);

        REQUIRE(tmp.exists());
        REQUIRE(tmp.dir().isDirectory());

        /*
         * JobDir::createParents(perms) leaves an already-existing directory
         * untouched. JobTmpDir therefore assumes cleanup ownership without
         * changing its existing permissions.
         */
        REQUIRE(tmp.dir().hasPermissions(
            job::io::IOPermissions::DefaultDirectory));
    }

    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("JobTmpDir throws when its path names an existing file",
          "[job_io][tmp_dir][edge][failure]")
{
    const auto path =
        job::io::test::tmpDirPath("existing_file");

    {
        std::FILE *file = std::fopen(path.c_str(), "wb");

        REQUIRE(file != nullptr);

        std::fclose(file);
    }

    REQUIRE(std::filesystem::is_regular_file(path));

    REQUIRE_THROWS_AS(
        job::io::JobTmpDir(path),
        std::runtime_error);

    REQUIRE(std::filesystem::is_regular_file(path));
    REQUIRE(std::filesystem::remove(path));
}

TEST_CASE("JobTmpDir throws for an empty path",
          "[job_io][tmp_dir][edge][empty]")
{
    REQUIRE_THROWS_AS(
        job::io::JobTmpDir(std::filesystem::path{}),
        std::runtime_error);
}