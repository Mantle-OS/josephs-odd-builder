#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <job_permissions.h>

using namespace job::io;

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

TEST_CASE("IOPermissions formatting exhaustive validation and benchmarks",
          "[job_io][permissions][stress][benchmark]")
{
    SECTION("all permission bit patterns produce valid formatting")
    {
        for (unsigned mode = 0; mode <= 07777u; ++mode) {
            const auto perms = static_cast<IOPermissions>(mode);

            const auto octal = toStringView(perms, PermissionStringType::OctalOnly);
            const auto symbolic = toStringView(perms, PermissionStringType::NoOctal);
            const auto both = toStringView(perms, PermissionStringType::Both);

            REQUIRE(octal.size() == 4);
            REQUIRE(symbolic.size() == 9);
            REQUIRE(both.size() == 14);

            REQUIRE(both.substr(0, 4) == octal);
            REQUIRE(both[4] == ' ');
            REQUIRE(both.substr(5, 9) == symbolic);

            REQUIRE(octal[0] >= '0');
            REQUIRE(octal[0] <= '7');
            REQUIRE(octal[1] >= '0');
            REQUIRE(octal[1] <= '7');
            REQUIRE(octal[2] >= '0');
            REQUIRE(octal[2] <= '7');
            REQUIRE(octal[3] >= '0');
            REQUIRE(octal[3] <= '7');
        }
    }

    SECTION("special permission bits format correctly")
    {
        REQUIRE(toStringView(static_cast<IOPermissions>(04755), PermissionStringType::Both) == "4755 rwsr-xr-x");
        REQUIRE(toStringView(static_cast<IOPermissions>(04644), PermissionStringType::Both) == "4644 rwSr--r--");

        REQUIRE(toStringView(static_cast<IOPermissions>(02755), PermissionStringType::Both) == "2755 rwxr-sr-x");
        REQUIRE(toStringView(static_cast<IOPermissions>(02644), PermissionStringType::Both) == "2644 rw-r-Sr--");

        REQUIRE(toStringView(static_cast<IOPermissions>(01777), PermissionStringType::Both) == "1777 rwxrwxrwt");
        REQUIRE(toStringView(static_cast<IOPermissions>(01776), PermissionStringType::Both) == "1776 rwxrwxrwT");

        REQUIRE(toStringView(static_cast<IOPermissions>(07777), PermissionStringType::Both) == "7777 rwsrwsrwt");
    }

    SECTION("benchmark lookup formatting across all permission values")
    {
        BENCHMARK("toStringView Both across all 4096 permissions") {
            std::uint64_t checksum = 0;

            for (unsigned mode = 0; mode <= 07777u; ++mode) {
                const auto value = toStringView(
                    static_cast<IOPermissions>(mode),
                    PermissionStringType::Both);

                checksum += static_cast<unsigned char>(value[0]);
                checksum += static_cast<unsigned char>(value[3]);
                checksum += static_cast<unsigned char>(value[5]);
                checksum += static_cast<unsigned char>(value[9]);
                checksum += static_cast<unsigned char>(value[13]);
            }

            return checksum;
        };
    }

    SECTION("benchmark compatibility string formatting across all permission values")
    {
        BENCHMARK("toString Both across all 4096 permissions") {
            std::uint64_t checksum = 0;

            for (unsigned mode = 0; mode <= 07777u; ++mode) {
                const auto value = toString(
                    static_cast<IOPermissions>(mode),
                    PermissionStringType::Both);

                checksum += static_cast<unsigned char>(value[0]);
                checksum += static_cast<unsigned char>(value[3]);
                checksum += static_cast<unsigned char>(value[5]);
                checksum += static_cast<unsigned char>(value[9]);
                checksum += static_cast<unsigned char>(value[13]);
            }

            return checksum;
        };
    }

    SECTION("benchmark individual runtime-varying permission lookups")
    {
        std::uint32_t state = 0x12345678u;

        BENCHMARK("toStringView Both runtime-varying permission") {
            // Xorshift keeps the requested permission changing so the compiler
            // cannot reduce this to one constant table entry.
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;

            const auto perms = static_cast<IOPermissions>(state & 07777u);
            const auto value = toStringView(perms, PermissionStringType::Both);

            return static_cast<std::uint32_t>(
                static_cast<unsigned char>(value[0]) +
                static_cast<unsigned char>(value[5]) +
                static_cast<unsigned char>(value[13]));
        };
    }
}

#endif
