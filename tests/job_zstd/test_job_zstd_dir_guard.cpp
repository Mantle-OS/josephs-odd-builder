#include <catch2/catch_test_macros.hpp>

#include "job_zstd_dir_guard.h"

#include <filesystem>
#include <vector>
#include <stdexcept>
#include <type_traits>

namespace job::zstd {

namespace {

[[nodiscard]] std::filesystem::path makePath(const std::string &name)
{
    return std::filesystem::path("/fake") / name;
}

// A little helper to exercise "guard goes out of scope via early return" --
// the whole point of RAII is that it doesn't care *why* the scope ended.
void pushThenReturnEarly(std::vector<std::filesystem::path> &stack, const std::filesystem::path &dir, bool shouldReturnEarly)
{
    JobZstdDirGuard const guard(stack, dir);

    if (shouldReturnEarly)
        return;
}

// Same idea, but the scope ends via an exception instead of a return.
void pushThenThrow(std::vector<std::filesystem::path> &stack, const std::filesystem::path &dir)
{
    JobZstdDirGuard const guard(stack, dir);
    throw std::runtime_error("simulated failure mid-traversal");
}

} // namespace

TEST_CASE("JobZstdDirGuard pushes a path on construction", "[job_zstd][dirguard][usage]")
{
    std::vector<std::filesystem::path> stack;
    std::filesystem::path const dir = makePath("a");

    {
        JobZstdDirGuard const guard(stack, dir);
        REQUIRE(stack.size() == 1);
        REQUIRE(stack.back() == dir);
    }
}

TEST_CASE("JobZstdDirGuard pops the path when it goes out of scope normally", "[job_zstd][dirguard][usage]")
{
    std::vector<std::filesystem::path> stack;
    std::filesystem::path const dir = makePath("a");

    {
        JobZstdDirGuard const guard(stack, dir);
        REQUIRE(stack.size() == 1);
    }

    REQUIRE(stack.empty());
}

TEST_CASE("JobZstdDirGuard nests in strict last-in-first-out order", "[job_zstd][dirguard][usage]")
{
    // This is the whole reason it exists: mirroring a DFS call stack so
    // "am I about to walk back into my own ancestor" is a simple linear scan, not something that needs its own bookkeeping.
    std::vector<std::filesystem::path> stack;
    std::filesystem::path const dirA = makePath("a");
    std::filesystem::path const dirB = makePath("a/b");
    std::filesystem::path const dirC = makePath("a/b/c");

    JobZstdDirGuard const guardA(stack, dirA);
    REQUIRE(stack == std::vector<std::filesystem::path>{dirA});

    {
        JobZstdDirGuard const guardB(stack, dirB);
        REQUIRE(stack == std::vector<std::filesystem::path>{dirA, dirB});

        {
            JobZstdDirGuard const guardC(stack, dirC);
            REQUIRE(stack == std::vector<std::filesystem::path>{dirA, dirB, dirC});
        }

        REQUIRE(stack == std::vector<std::filesystem::path>{dirA, dirB});
    }

    REQUIRE(stack == std::vector<std::filesystem::path>{dirA});
}

// 2
TEST_CASE("JobZstdDirGuard pops correctly even when the scope exits via an early return", "[job_zstd][dirguard][edge]")
{
    std::vector<std::filesystem::path> stack;

    pushThenReturnEarly(stack, makePath("a"), true);
    REQUIRE(stack.empty());
}

TEST_CASE("JobZstdDirGuard pops correctly even when the scope exits via an exception", "[job_zstd][dirguard][edge]")
{
    // This is the case that actually justifies using RAII
    std::vector<std::filesystem::path> stack;

    REQUIRE_THROWS_AS(pushThenThrow(stack, makePath("a")), std::runtime_error);
    REQUIRE(stack.empty());
}

TEST_CASE("JobZstdDirGuard leaves earlier entries untouched when a later one unwinds via exception", "[job_zstd][dirguard][edge]")
{
    std::vector<std::filesystem::path> stack;
    std::filesystem::path const dirA = makePath("a");

    JobZstdDirGuard const guardA(stack, dirA);
    REQUIRE(stack.size() == 1);

    REQUIRE_THROWS_AS(pushThenThrow(stack, makePath("a/b")), std::runtime_error);
    REQUIRE(stack == std::vector<std::filesystem::path>{dirA});
}

TEST_CASE("JobZstdDirGuard works correctly starting from an empty stack", "[job_zstd][dirguard][edge]")
{
    std::vector<std::filesystem::path> stack;
    REQUIRE(stack.empty());

    {
        JobZstdDirGuard const guard(stack, makePath("only"));
        REQUIRE(stack.size() == 1);
    }

    REQUIRE(stack.empty());
}

TEST_CASE("JobZstdDirGuard accepts duplicate paths without complaint", "[job_zstd][dirguard][edge]")
{
    std::vector<std::filesystem::path> stack;
    std::filesystem::path const dir = makePath("a");

    JobZstdDirGuard const outer(stack, dir);
    JobZstdDirGuard const inner(stack, dir);

    REQUIRE(stack == std::vector<std::filesystem::path>{dir, dir});
}

TEST_CASE("JobZstdDirGuard is not copyable", "[job_zstd][dirguard][edge]")
{
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<JobZstdDirGuard>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<JobZstdDirGuard>);
}

TEST_CASE("JobZstdDirGuard is not movable", "[job_zstd][dirguard][edge]")
{
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<JobZstdDirGuard>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<JobZstdDirGuard>);
}

} // namespace job::zstd