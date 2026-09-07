#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>
#include <unordered_set>

#include <pty_env_posix.h>

using namespace job::io;

TEST_CASE("PtyEnvPosix exposes core POSIX environment names", "[job_io][pty][env][posix]")
{
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Home) == "HOME");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Path) == "PATH");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Lang) == "LANG");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::LcAll) == "LC_ALL");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Pwd) == "PWD");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Oldpwd) == "OLDPWD");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Shell) == "SHELL");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Term) == "TERM");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Tmpdir) == "TMPDIR");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Tz) == "TZ");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::User) == "USER");
    REQUIRE(PtyEnvPosix::name(PtyEnvPosix::Env::Logname) == "LOGNAME");
}

TEST_CASE("PtyEnvPosix exposes locale environment metadata", "[job_io][pty][env][posix][locale]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;
    using Requirement = PtyEnvPosix::Requirement;

    REQUIRE(PtyEnvPosix::valueType(Env::Lang) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcAll) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcCollate) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcCtype) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcMessages) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcMonetary) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcNumeric) == ValueType::Locale);
    REQUIRE(PtyEnvPosix::valueType(Env::LcTime) == ValueType::Locale);

    REQUIRE(PtyEnvPosix::requirement(Env::Lang) == Requirement::Optional);
    REQUIRE(PtyEnvPosix::requirement(Env::LcAll) == Requirement::Optional);
}

TEST_CASE("PtyEnvPosix exposes login initialized environment metadata", "[job_io][pty][env][posix]")
{
    using Env = PtyEnvPosix::Env;
    using Requirement = PtyEnvPosix::Requirement;

    REQUIRE(PtyEnvPosix::requirement(Env::Home) == Requirement::LoginInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Logname) == Requirement::LoginInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Shell) == Requirement::LoginInitialized);
}

TEST_CASE("PtyEnvPosix exposes shell initialized environment metadata", "[job_io][pty][env][posix]")
{
    using Env = PtyEnvPosix::Env;
    using Requirement = PtyEnvPosix::Requirement;

    REQUIRE(PtyEnvPosix::requirement(Env::Oldpwd) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Optarg) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Optind) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Ppid) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Ps1) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Ps2) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Ps4) == Requirement::ShellInitialized);
    REQUIRE(PtyEnvPosix::requirement(Env::Pwd) == Requirement::ShellInitialized);
}

TEST_CASE("PtyEnvPosix exposes utility defined environment metadata", "[job_io][pty][env][posix]")
{
    using Env = PtyEnvPosix::Env;
    using Requirement = PtyEnvPosix::Requirement;

    REQUIRE(PtyEnvPosix::requirement(Env::Arflags) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Cc) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Cflags) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Fc) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Fflags) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Ldflags) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Lex) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Lflags) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Makeflags) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Yacc) == Requirement::UtilityDefined);
    REQUIRE(PtyEnvPosix::requirement(Env::Yflags) == Requirement::UtilityDefined);
}

TEST_CASE("PtyEnvPosix classifies path values", "[job_io][pty][env][posix][path]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;

    REQUIRE(PtyEnvPosix::valueType(Env::Home) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Datemsk) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Histfile) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Mail) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Pwd) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Oldpwd) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Shell) == ValueType::Path);
    REQUIRE(PtyEnvPosix::valueType(Env::Tmpdir) == ValueType::Path);
}

TEST_CASE("PtyEnvPosix classifies path lists", "[job_io][pty][env][posix][list]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;

    REQUIRE(PtyEnvPosix::valueType(Env::Path) == ValueType::PathList);
    REQUIRE(PtyEnvPosix::valueType(Env::Cdpath) == ValueType::PathList);
    REQUIRE(PtyEnvPosix::valueType(Env::Mailpath) == ValueType::PathList);
    REQUIRE(PtyEnvPosix::valueType(Env::Manpath) == ValueType::PathList);

    REQUIRE(PtyEnvPosix::isList(Env::Path));
    REQUIRE(PtyEnvPosix::isList(Env::Cdpath));
    REQUIRE(PtyEnvPosix::isList(Env::Mailpath));
    REQUIRE(PtyEnvPosix::isList(Env::Manpath));

    REQUIRE(PtyEnvPosix::separator(Env::Path) == ':');
    REQUIRE(PtyEnvPosix::separator(Env::Cdpath) == ':');
    REQUIRE(PtyEnvPosix::separator(Env::Mailpath) == ':');
    REQUIRE(PtyEnvPosix::separator(Env::Manpath) == ':');
}

TEST_CASE("PtyEnvPosix classifies NLSPATH as a message catalog list", "[job_io][pty][env][posix][list]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;

    REQUIRE(PtyEnvPosix::name(Env::Nlspath) == "NLSPATH");
    REQUIRE(PtyEnvPosix::valueType(Env::Nlspath) == ValueType::MessageCatalogPath);
    REQUIRE(PtyEnvPosix::isList(Env::Nlspath));
    REQUIRE(PtyEnvPosix::separator(Env::Nlspath) == ':');
}

TEST_CASE("PtyEnvPosix classifies TZ as timezone metadata", "[job_io][pty][env][posix][timezone]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;
    using Requirement = PtyEnvPosix::Requirement;

    REQUIRE(PtyEnvPosix::name(Env::Tz) == "TZ");
    REQUIRE(PtyEnvPosix::valueType(Env::Tz) == ValueType::Timezone);
    REQUIRE(PtyEnvPosix::requirement(Env::Tz) == Requirement::Optional);
    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Tz));
    REQUIRE(PtyEnvPosix::separator(Env::Tz) == '\0');
}

TEST_CASE("PtyEnvPosix classifies ENV as a shell code path", "[job_io][pty][env][posix]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;

    REQUIRE(PtyEnvPosix::name(Env::Env) == "ENV");
    REQUIRE(PtyEnvPosix::valueType(Env::Env) == ValueType::ShellCodePath);
    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Env));
}

TEST_CASE("PtyEnvPosix classifies integer environment values", "[job_io][pty][env][posix][integer]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;

    REQUIRE(PtyEnvPosix::valueType(Env::Columns) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Histsize) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Lineno) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Lines) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Mailcheck) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Nproc) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Optind) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Ppid) == ValueType::PositiveInteger);
    REQUIRE(PtyEnvPosix::valueType(Env::Seconds) == ValueType::PositiveInteger);

    REQUIRE(PtyEnvPosix::valueType(Env::Opterr) == ValueType::Integer);
    REQUIRE(PtyEnvPosix::valueType(Env::Random) == ValueType::Integer);
}

TEST_CASE("PtyEnvPosix classifies compiler and utility flags", "[job_io][pty][env][posix][flags]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;

    REQUIRE(PtyEnvPosix::valueType(Env::Arflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Cflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Fflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Gflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Ldflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Lflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Makeflags) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::More) == ValueType::Flags);
    REQUIRE(PtyEnvPosix::valueType(Env::Yflags) == ValueType::Flags);
}

TEST_CASE("PtyEnvPosix scalar values are not lists", "[job_io][pty][env][posix][list]")
{
    using Env = PtyEnvPosix::Env;

    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Home));
    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Lang));
    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Shell));
    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Term));
    REQUIRE_FALSE(PtyEnvPosix::isList(Env::Tz));

    REQUIRE(PtyEnvPosix::separator(Env::Home) == '\0');
    REQUIRE(PtyEnvPosix::separator(Env::Lang) == '\0');
    REQUIRE(PtyEnvPosix::separator(Env::Shell) == '\0');
    REQUIRE(PtyEnvPosix::separator(Env::Term) == '\0');
    REQUIRE(PtyEnvPosix::separator(Env::Tz) == '\0');
}

TEST_CASE("PtyEnvPosix contains metadata for every enumerated environment entry", "[job_io][pty][env][posix][table]")
{
    using Env = PtyEnvPosix::Env;

    const auto count = static_cast<std::size_t>(Env::Count);

    REQUIRE(count > 0);

    for (std::size_t index = 0; index < count; ++index) {
        const auto env = static_cast<Env>(index);

        INFO("Environment index: " << index);

        REQUIRE_FALSE(PtyEnvPosix::name(env).empty());
    }
}

TEST_CASE("PtyEnvPosix environment names are unique", "[job_io][pty][env][posix][table]")
{
    using Env = PtyEnvPosix::Env;

    std::unordered_set<std::string_view> names;
    const auto count = static_cast<std::size_t>(Env::Count);

    names.reserve(count);

    for (std::size_t index = 0; index < count; ++index) {
        const auto env = static_cast<Env>(index);
        const std::string_view name = PtyEnvPosix::name(env);

        INFO("Environment index: " << index << ", name: " << name);

        REQUIRE_FALSE(name.empty());
        REQUIRE(names.insert(name).second);
    }

    REQUIRE(names.size() == count);
}

TEST_CASE("PtyEnvPosix list metadata always has a separator", "[job_io][pty][env][posix][table]")
{
    using Env = PtyEnvPosix::Env;

    const auto count = static_cast<std::size_t>(Env::Count);

    for (std::size_t index = 0; index < count; ++index) {
        const auto env = static_cast<Env>(index);
        const bool list = PtyEnvPosix::isList(env);
        const char separator = PtyEnvPosix::separator(env);

        INFO("Environment: " << PtyEnvPosix::name(env));

        if (list)
            REQUIRE(separator != '\0');
        else
            REQUIRE(separator == '\0');
    }
}

TEST_CASE("PtyEnvPosix invalid environment entry returns safe defaults", "[job_io][pty][env][posix][edge]")
{
    using Env = PtyEnvPosix::Env;
    using ValueType = PtyEnvPosix::ValueType;
    using Requirement = PtyEnvPosix::Requirement;

    const auto invalid = static_cast<Env>(static_cast<std::uint8_t>(Env::Count));

    REQUIRE(PtyEnvPosix::name(invalid).empty());
    REQUIRE(PtyEnvPosix::valueType(invalid) == ValueType::Scalar);
    REQUIRE(PtyEnvPosix::requirement(invalid) == Requirement::Optional);
    REQUIRE_FALSE(PtyEnvPosix::isList(invalid));
    REQUIRE(PtyEnvPosix::separator(invalid) == '\0');
}