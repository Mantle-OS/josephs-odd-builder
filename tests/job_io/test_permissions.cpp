#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <job_permissions.h>

using namespace job::core;

TEST_CASE("IOPermissions basic usage and examples", "[core][permissions][usage]")
{
    SECTION("DefaultFile as classic 0644")
    {
        IOPermissions perms = IOPermissions::DefaultFile;
        PermissionBits mode = toMode(perms);

        // 0644 = rw-r--r--
        REQUIRE((mode & 0777u) == 0644u);

        const auto oct  = toString(perms, PermissionStringType::OctalOnly);
        const auto sym  = toString(perms, PermissionStringType::NoOctal);
        const auto both = toString(perms, PermissionStringType::Both);

        REQUIRE(oct  == "0644");
        REQUIRE(sym  == "rw-r--r--");
        REQUIRE(both == "0644 rw-r--r--");

        REQUIRE(std::string(toName(perms)) == "DefaultFile");
    }

    SECTION("DefaultDirectory as typical 0755")
    {
        IOPermissions perms = IOPermissions::DefaultDirectory;
        PermissionBits mode = toMode(perms);

        // 0755 = rwxr-xr-x
        REQUIRE((mode & 0777u) == 0755u);

        const auto oct  = toString(perms, PermissionStringType::OctalOnly);
        const auto sym  = toString(perms, PermissionStringType::NoOctal);
        const auto both = toString(perms, PermissionStringType::Both);

        REQUIRE(oct  == "0755");
        REQUIRE(sym  == "rwxr-xr-x");
        REQUIRE(both == "0755 rwxr-xr-x");

        REQUIRE(std::string(toName(perms)) == "DefaultDirectory");
    }

    SECTION("ReadWriteUser (0600) for private files")
    {
        IOPermissions perms = IOPermissions::ReadWriteUser;
        PermissionBits mode = toMode(perms);

        REQUIRE((mode & 0777u) == 0600u);

        const auto oct = toString(perms, PermissionStringType::OctalOnly);
        const auto sym = toString(perms, PermissionStringType::NoOctal);

        REQUIRE(oct == "0600");
        REQUIRE(sym == "rw-------");

        REQUIRE(std::string(toName(perms)) == "ReadWriteUser");
    }

    SECTION("BadIdeas (0777) for the what-are-you-doing mode")
    {
        IOPermissions perms = IOPermissions::BadIdeas;
        PermissionBits mode = toMode(perms);

        REQUIRE((mode & 0777u) == 0777u);

        const auto oct  = toString(perms, PermissionStringType::OctalOnly);
        const auto sym  = toString(perms, PermissionStringType::NoOctal);
        const auto both = toString(perms, PermissionStringType::Both);

        REQUIRE(oct  == "0777");
        REQUIRE(sym  == "rwxrwxrwx");
        REQUIRE(both == "0777 rwxrwxrwx");

        REQUIRE(std::string(toName(perms)) == "WhyAreYouDoingThis");
    }

    SECTION("None = 0000, no permissions")
    {
        IOPermissions perms = IOPermissions::None;
        PermissionBits mode = toMode(perms);

        REQUIRE((mode & 0777u) == 0000u);

        const auto oct  = toString(perms, PermissionStringType::OctalOnly);
        const auto sym  = toString(perms, PermissionStringType::NoOctal);
        const auto both = toString(perms, PermissionStringType::Both);

        REQUIRE(oct  == "0000");
        REQUIRE(sym  == "---------");
        REQUIRE(both == "0000 ---------");

        REQUIRE(std::string(toName(perms)) == "None");
    }
}

//
// Block 2: edge cases
//
TEST_CASE("IOPermissions::BadIdeas encodes as 0777", "[core][permissions]") {
    auto m = toMode(IOPermissions::BadIdeas);
    REQUIRE((m & 07777u) == 0777u);
    auto s = toString(IOPermissions::BadIdeas, PermissionStringType::Both);
    REQUIRE(s.starts_with("0777")); // no filesystem harmed
}

TEST_CASE("IOPermissions edge cases for setuid/setgid/sticky and custom combos",
          "[core][permissions][edge]")
{
    SECTION("setuid affects user exec position with s/S")
    {
        // user r + x + setuid => r-s------
        {
            PermissionBits m = S_IRUSR | S_IXUSR | S_ISUID;
            auto p   = static_cast<IOPermissions>(m);
            auto sym = toString(p, PermissionStringType::NoOctal);
            REQUIRE(sym == "r-s------");
        }

        // user r + setuid (no exec) => r-S------
        {
            PermissionBits m = S_IRUSR | S_ISUID;
            auto p   = static_cast<IOPermissions>(m);
            auto sym = toString(p, PermissionStringType::NoOctal);
            REQUIRE(sym == "r-S------");
        }
    }

    SECTION("setgid affects group exec position with s/S")
    {
        // group r + x + setgid
        {
            PermissionBits m = S_IRGRP | S_IXGRP | S_ISGID;
            auto p   = static_cast<IOPermissions>(m);
            auto sym = toString(p, PermissionStringType::NoOctal);
            REQUIRE(sym == "---r-s---");

        }

        // group r + setgid
        {
            PermissionBits m = S_IRGRP | S_ISGID;
            auto p   = static_cast<IOPermissions>(m);
            auto sym = toString(p, PermissionStringType::NoOctal);
            REQUIRE(sym == "---r-S---");
        }
    }

    SECTION("sticky bit affects other exec position with t/T")
    {
        // sticky + other exec => --------t
        {
            PermissionBits m = S_IXOTH | S_ISVTX;
            auto p   = static_cast<IOPermissions>(m);
            auto sym = toString(p, PermissionStringType::NoOctal);
            REQUIRE(sym == "--------t");
        }

        // sticky only => --------T
        {
            PermissionBits m = S_ISVTX;
            auto p   = static_cast<IOPermissions>(m);
            auto sym = toString(p, PermissionStringType::NoOctal);
            REQUIRE(sym == "--------T");
        }
    }

    SECTION("Custom permission combinations map to name=Custom but still stringify correctly")
    {
        PermissionBits m = S_IRUSR | S_IWOTH; // owner read, other write
        auto p = static_cast<IOPermissions>(m);

        // Not one of the named presets:
        REQUIRE(std::string(toName(p)) == "Custom");

        auto sym = toString(p, PermissionStringType::NoOctal);
        REQUIRE(sym.size() == 9);

        // sanity: we at least see 'r' and 'w' somewhere in there
        REQUIRE(sym.find('r') != std::string::npos);
        REQUIRE(sym.find('w') != std::string::npos);
    }

    SECTION("OctalOnly, NoOctal, Both produce structurally consistent strings")
    {
        IOPermissions perms = IOPermissions::DefaultFile;

        const auto octOnly = toString(perms, PermissionStringType::OctalOnly);
        const auto noOct   = toString(perms, PermissionStringType::NoOctal);
        const auto both    = toString(perms, PermissionStringType::Both);

        // Octal-only: 4 chars, no spaces
        REQUIRE(octOnly.size() == 4);
        REQUIRE(octOnly.find(' ') == std::string::npos);

        // No-octal: 9 chars, no spaces
        REQUIRE(noOct.size() == 9);
        REQUIRE(noOct.find(' ') == std::string::npos);

        // Both: "0XYZ rwxr-xr-x"
        REQUIRE(both.size() >= 4 + 1 + 9);
        auto pos = both.find(' ');
        REQUIRE(pos == 4);
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("IOPermissions toString formatting stress and micro-benchmark",
          "[core][permissions][stress][benchmark]")
{
    std::vector<IOPermissions> perms;
    perms.reserve(32);

    perms.push_back(IOPermissions::None);
    perms.push_back(IOPermissions::DefaultFile);
    perms.push_back(IOPermissions::DefaultDirectory);
    perms.push_back(IOPermissions::BadIdeas);
    perms.push_back(IOPermissions::ReadUser);
    perms.push_back(IOPermissions::ReadWriteUser);
    perms.push_back(IOPermissions::ReadWriteAll);

    // A few raw octal-style combinations
    for (unsigned m = 0; m <= 0777u; m += 0177u)
        perms.push_back(static_cast<IOPermissions>(m));

    SECTION("structural sanity over many iterations")
    {
        for (int iter = 0; iter < 5000; ++iter) {
            for (auto p : perms) {
                auto s1 = toString(p, PermissionStringType::OctalOnly);
                auto s2 = toString(p, PermissionStringType::NoOctal);
                auto s3 = toString(p, PermissionStringType::Both);

                REQUIRE_FALSE(s1.empty());
                REQUIRE_FALSE(s2.empty());
                REQUIRE_FALSE(s3.empty());

                // "Both" must be "0XYZ rwxr-xr-x" layout
                auto pos = s3.find(' ');
                REQUIRE(pos == 4);
                REQUIRE(s3.size() >= 4 + 1 + 9);
            }
        }
    }

    SECTION("benchmark toString(PermissionStringType::Both)")
    {
        BENCHMARK("toString Both on mixed permissions")
        {
            std::size_t total = 0;
            for (auto p : perms) {
                auto s = toString(p, PermissionStringType::Both);
                total += s.size();
            }
            // return something so the compiler can't just nuke the loop
            return total;
        };
    }
}
#endif
