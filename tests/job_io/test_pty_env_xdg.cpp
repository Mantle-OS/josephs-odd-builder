#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_set>

#include <pty_env_xdg.h>

using namespace job::io;

TEST_CASE("PtyEnvXdg exposes XDG environment names", "[job_io][pty][env][xdg]")
{
    using Env = PtyEnvXdg::Env;

    REQUIRE(PtyEnvXdg::name(Env::DataHome) == "XDG_DATA_HOME");
    REQUIRE(PtyEnvXdg::name(Env::ConfigHome) == "XDG_CONFIG_HOME");
    REQUIRE(PtyEnvXdg::name(Env::StateHome) == "XDG_STATE_HOME");
    REQUIRE(PtyEnvXdg::name(Env::CacheHome) == "XDG_CACHE_HOME");
    REQUIRE(PtyEnvXdg::name(Env::RuntimeDir) == "XDG_RUNTIME_DIR");
    REQUIRE(PtyEnvXdg::name(Env::DataDirs) == "XDG_DATA_DIRS");
    REQUIRE(PtyEnvXdg::name(Env::ConfigDirs) == "XDG_CONFIG_DIRS");
}

TEST_CASE("PtyEnvXdg classifies scalar paths and path lists", "[job_io][pty][env][xdg][type]")
{
    using Env = PtyEnvXdg::Env;
    using ValueType = PtyEnvXdg::ValueType;

    REQUIRE(PtyEnvXdg::valueType(Env::DataHome) == ValueType::Path);
    REQUIRE(PtyEnvXdg::valueType(Env::ConfigHome) == ValueType::Path);
    REQUIRE(PtyEnvXdg::valueType(Env::StateHome) == ValueType::Path);
    REQUIRE(PtyEnvXdg::valueType(Env::CacheHome) == ValueType::Path);
    REQUIRE(PtyEnvXdg::valueType(Env::RuntimeDir) == ValueType::Path);

    REQUIRE(PtyEnvXdg::valueType(Env::DataDirs) == ValueType::PathList);
    REQUIRE(PtyEnvXdg::valueType(Env::ConfigDirs) == ValueType::PathList);
}

TEST_CASE("PtyEnvXdg home variables use home relative defaults", "[job_io][pty][env][xdg][default]")
{
    using Env = PtyEnvXdg::Env;
    using DefaultType = PtyEnvXdg::DefaultType;

    REQUIRE(PtyEnvXdg::defaultType(Env::DataHome) == DefaultType::HomeRelative);
    REQUIRE(PtyEnvXdg::defaultValue(Env::DataHome) == ".local/share");

    REQUIRE(PtyEnvXdg::defaultType(Env::ConfigHome) == DefaultType::HomeRelative);
    REQUIRE(PtyEnvXdg::defaultValue(Env::ConfigHome) == ".config");

    REQUIRE(PtyEnvXdg::defaultType(Env::StateHome) == DefaultType::HomeRelative);
    REQUIRE(PtyEnvXdg::defaultValue(Env::StateHome) == ".local/state");

    REQUIRE(PtyEnvXdg::defaultType(Env::CacheHome) == DefaultType::HomeRelative);
    REQUIRE(PtyEnvXdg::defaultValue(Env::CacheHome) == ".cache");
}

TEST_CASE("PtyEnvXdg runtime directory has no static default", "[job_io][pty][env][xdg][runtime]")
{
    using Env = PtyEnvXdg::Env;
    using DefaultType = PtyEnvXdg::DefaultType;

    REQUIRE(PtyEnvXdg::defaultType(Env::RuntimeDir) == DefaultType::None);
    REQUIRE(PtyEnvXdg::defaultValue(Env::RuntimeDir).empty());
}

TEST_CASE("PtyEnvXdg directory lists expose absolute defaults", "[job_io][pty][env][xdg][default][list]")
{
    using Env = PtyEnvXdg::Env;
    using DefaultType = PtyEnvXdg::DefaultType;

    REQUIRE(PtyEnvXdg::defaultType(Env::DataDirs) == DefaultType::Absolute);
    REQUIRE(PtyEnvXdg::defaultValue(Env::DataDirs) == "/usr/local/share:/usr/share");

    REQUIRE(PtyEnvXdg::defaultType(Env::ConfigDirs) == DefaultType::Absolute);
    REQUIRE(PtyEnvXdg::defaultValue(Env::ConfigDirs) == "/etc/xdg");
}

TEST_CASE("PtyEnvXdg data search group ordering is correct", "[job_io][pty][env][xdg][search]")
{
    using Env = PtyEnvXdg::Env;
    using SearchGroup = PtyEnvXdg::SearchGroup;

    REQUIRE(PtyEnvXdg::searchGroup(Env::DataHome) == SearchGroup::Data);
    REQUIRE(PtyEnvXdg::searchPriority(Env::DataHome) == 0);

    REQUIRE(PtyEnvXdg::searchGroup(Env::DataDirs) == SearchGroup::Data);
    REQUIRE(PtyEnvXdg::searchPriority(Env::DataDirs) == 1);
}

TEST_CASE("PtyEnvXdg config search group ordering is correct", "[job_io][pty][env][xdg][search]")
{
    using Env = PtyEnvXdg::Env;
    using SearchGroup = PtyEnvXdg::SearchGroup;

    REQUIRE(PtyEnvXdg::searchGroup(Env::ConfigHome) == SearchGroup::Config);
    REQUIRE(PtyEnvXdg::searchPriority(Env::ConfigHome) == 0);

    REQUIRE(PtyEnvXdg::searchGroup(Env::ConfigDirs) == SearchGroup::Config);
    REQUIRE(PtyEnvXdg::searchPriority(Env::ConfigDirs) == 1);
}

TEST_CASE("PtyEnvXdg non-search variables have no search group", "[job_io][pty][env][xdg][search]")
{
    using Env = PtyEnvXdg::Env;
    using SearchGroup = PtyEnvXdg::SearchGroup;

    REQUIRE(PtyEnvXdg::searchGroup(Env::StateHome) == SearchGroup::None);
    REQUIRE(PtyEnvXdg::searchGroup(Env::CacheHome) == SearchGroup::None);
    REQUIRE(PtyEnvXdg::searchGroup(Env::RuntimeDir) == SearchGroup::None);

    REQUIRE(PtyEnvXdg::searchPriority(Env::StateHome) == 0);
    REQUIRE(PtyEnvXdg::searchPriority(Env::CacheHome) == 0);
    REQUIRE(PtyEnvXdg::searchPriority(Env::RuntimeDir) == 0);
}

TEST_CASE("PtyEnvXdg scalar path variables require absolute paths", "[job_io][pty][env][xdg][constraint]")
{
    using Env = PtyEnvXdg::Env;
    using Constraint = PtyEnvXdg::Constraint;

    REQUIRE(hasConstraint(PtyEnvXdg::constraints(Env::DataHome), Constraint::AbsolutePath));
    REQUIRE(hasConstraint(PtyEnvXdg::constraints(Env::ConfigHome), Constraint::AbsolutePath));
    REQUIRE(hasConstraint(PtyEnvXdg::constraints(Env::StateHome), Constraint::AbsolutePath));
    REQUIRE(hasConstraint(PtyEnvXdg::constraints(Env::CacheHome), Constraint::AbsolutePath));
}

TEST_CASE("PtyEnvXdg directory lists require absolute paths", "[job_io][pty][env][xdg][constraint][list]")
{
    using Env = PtyEnvXdg::Env;
    using Constraint = PtyEnvXdg::Constraint;

    REQUIRE(hasConstraint(PtyEnvXdg::constraints(Env::DataDirs), Constraint::AbsolutePath));
    REQUIRE(hasConstraint(PtyEnvXdg::constraints(Env::ConfigDirs), Constraint::AbsolutePath));
}

TEST_CASE("PtyEnvXdg runtime directory exposes all required constraints", "[job_io][pty][env][xdg][runtime][constraint]")
{
    using Env = PtyEnvXdg::Env;
    using Constraint = PtyEnvXdg::Constraint;

    const Constraint constraints = PtyEnvXdg::constraints(Env::RuntimeDir);

    REQUIRE(hasConstraint(constraints, Constraint::AbsolutePath));
    REQUIRE(hasConstraint(constraints, Constraint::UserOwned));
    REQUIRE(hasConstraint(constraints, Constraint::PrivateMode));
    REQUIRE(hasConstraint(constraints, Constraint::LocalFilesystem));
    REQUIRE(hasConstraint(constraints, Constraint::SessionLifetime));
    REQUIRE(hasConstraint(constraints, Constraint::WarnOnFallback));
}

TEST_CASE("PtyEnvXdg runtime directory uses private mode 0700", "[job_io][pty][env][xdg][runtime]")
{
    REQUIRE(PtyEnvXdg::privateDirectoryMode == 0700);
}

TEST_CASE("PtyEnvXdg constraint operators combine flags", "[job_io][pty][env][xdg][constraint]")
{
    using Constraint = PtyEnvXdg::Constraint;

    Constraint constraints = Constraint::AbsolutePath | Constraint::UserOwned;

    REQUIRE(hasConstraint(constraints, Constraint::AbsolutePath));
    REQUIRE(hasConstraint(constraints, Constraint::UserOwned));
    REQUIRE_FALSE(hasConstraint(constraints, Constraint::PrivateMode));

    constraints |= Constraint::PrivateMode;

    REQUIRE(hasConstraint(constraints, Constraint::PrivateMode));
    REQUIRE((constraints & Constraint::UserOwned) == Constraint::UserOwned);
}

TEST_CASE("PtyEnvXdg directory lists use colon separators", "[job_io][pty][env][xdg][list]")
{
    using Env = PtyEnvXdg::Env;

    REQUIRE(PtyEnvXdg::isList(Env::DataDirs));
    REQUIRE(PtyEnvXdg::isList(Env::ConfigDirs));

    REQUIRE(PtyEnvXdg::separator(Env::DataDirs) == ':');
    REQUIRE(PtyEnvXdg::separator(Env::ConfigDirs) == ':');
}

TEST_CASE("PtyEnvXdg scalar variables are not lists", "[job_io][pty][env][xdg][list]")
{
    using Env = PtyEnvXdg::Env;

    REQUIRE_FALSE(PtyEnvXdg::isList(Env::DataHome));
    REQUIRE_FALSE(PtyEnvXdg::isList(Env::ConfigHome));
    REQUIRE_FALSE(PtyEnvXdg::isList(Env::StateHome));
    REQUIRE_FALSE(PtyEnvXdg::isList(Env::CacheHome));
    REQUIRE_FALSE(PtyEnvXdg::isList(Env::RuntimeDir));

    REQUIRE(PtyEnvXdg::separator(Env::DataHome) == '\0');
    REQUIRE(PtyEnvXdg::separator(Env::ConfigHome) == '\0');
    REQUIRE(PtyEnvXdg::separator(Env::StateHome) == '\0');
    REQUIRE(PtyEnvXdg::separator(Env::CacheHome) == '\0');
    REQUIRE(PtyEnvXdg::separator(Env::RuntimeDir) == '\0');
}

TEST_CASE("PtyEnvXdg exposes base directory paths", "[job_io][pty][env][xdg][base]")
{
    using BaseDir = PtyEnvXdg::BaseDir;

    REQUIRE(PtyEnvXdg::basePath(BaseDir::Data) == ".local/share");
    REQUIRE(PtyEnvXdg::basePath(BaseDir::Config) == ".config");
    REQUIRE(PtyEnvXdg::basePath(BaseDir::State) == ".local/state");
    REQUIRE(PtyEnvXdg::basePath(BaseDir::Cache) == ".cache");
    REQUIRE(PtyEnvXdg::basePath(BaseDir::Runtime).empty());
    REQUIRE(PtyEnvXdg::basePath(BaseDir::Executable) == ".local/bin");
}

TEST_CASE("PtyEnvXdg contains metadata for every environment entry", "[job_io][pty][env][xdg][table]")
{
    using Env = PtyEnvXdg::Env;

    const auto count = static_cast<std::size_t>(Env::Count);

    REQUIRE(count == 7);

    for (std::size_t index = 0; index < count; ++index) {
        const auto env = static_cast<Env>(index);

        INFO("XDG environment index: " << index);

        REQUIRE_FALSE(PtyEnvXdg::name(env).empty());
    }
}

TEST_CASE("PtyEnvXdg environment names are unique", "[job_io][pty][env][xdg][table]")
{
    using Env = PtyEnvXdg::Env;

    std::unordered_set<std::string_view> names;
    const auto count = static_cast<std::size_t>(Env::Count);

    names.reserve(count);

    for (std::size_t index = 0; index < count; ++index) {
        const auto env = static_cast<Env>(index);
        const std::string_view name = PtyEnvXdg::name(env);

        INFO("XDG environment index: " << index << ", name: " << name);

        REQUIRE_FALSE(name.empty());
        REQUIRE(names.insert(name).second);
    }

    REQUIRE(names.size() == count);
}

TEST_CASE("PtyEnvXdg contains metadata for every base directory", "[job_io][pty][env][xdg][base][table]")
{
    using BaseDir = PtyEnvXdg::BaseDir;

    const auto count = static_cast<std::size_t>(BaseDir::Count);

    REQUIRE(count == 6);

    for (std::size_t index = 0; index < count; ++index) {
        const auto dir = static_cast<BaseDir>(index);

        INFO("XDG base directory index: " << index);

        if (dir == BaseDir::Runtime)
            REQUIRE(PtyEnvXdg::basePath(dir).empty());
        else
            REQUIRE_FALSE(PtyEnvXdg::basePath(dir).empty());
    }
}

TEST_CASE("PtyEnvXdg invalid environment entry returns safe defaults", "[job_io][pty][env][xdg][edge]")
{
    using Env = PtyEnvXdg::Env;
    using ValueType = PtyEnvXdg::ValueType;
    using DefaultType = PtyEnvXdg::DefaultType;
    using SearchGroup = PtyEnvXdg::SearchGroup;
    using Constraint = PtyEnvXdg::Constraint;

    const auto invalid = static_cast<Env>(static_cast<std::uint8_t>(Env::Count));

    REQUIRE(PtyEnvXdg::name(invalid).empty());
    REQUIRE(PtyEnvXdg::valueType(invalid) == ValueType::Path);
    REQUIRE(PtyEnvXdg::defaultType(invalid) == DefaultType::None);
    REQUIRE(PtyEnvXdg::defaultValue(invalid).empty());
    REQUIRE(PtyEnvXdg::searchGroup(invalid) == SearchGroup::None);
    REQUIRE(PtyEnvXdg::searchPriority(invalid) == 0);
    REQUIRE(PtyEnvXdg::constraints(invalid) == Constraint::None);
    REQUIRE_FALSE(PtyEnvXdg::isList(invalid));
    REQUIRE(PtyEnvXdg::separator(invalid) == '\0');
}

TEST_CASE("PtyEnvXdg invalid base directory returns an empty path", "[job_io][pty][env][xdg][base][edge]")
{
    using BaseDir = PtyEnvXdg::BaseDir;

    const auto invalid = static_cast<BaseDir>(static_cast<std::uint8_t>(BaseDir::Count));

    REQUIRE(PtyEnvXdg::basePath(invalid).empty());
}