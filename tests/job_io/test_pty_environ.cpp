#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <pty_environ.h>

using namespace job::io;

class TestEnvironmentGuard
{
public:
    explicit TestEnvironmentGuard(std::string name) :
        m_name(std::move(name))
    {
        if (const char *value = ::getenv(m_name.c_str()))
            m_original = value;
    }

    ~TestEnvironmentGuard()
    {
        if (m_original)
            ::setenv(m_name.c_str(), m_original->c_str(), 1);
        else
            ::unsetenv(m_name.c_str());
    }

    TestEnvironmentGuard(const TestEnvironmentGuard &) = delete;
    TestEnvironmentGuard &operator=(const TestEnvironmentGuard &) = delete;
    TestEnvironmentGuard(TestEnvironmentGuard &&) = delete;
    TestEnvironmentGuard &operator=(TestEnvironmentGuard &&) = delete;

private:
    std::string m_name;
    std::optional<std::string> m_original;
};

class TestEnvironmentFile
{
public:
    TestEnvironmentFile(std::string name, std::string contents)
    {
        m_path = std::filesystem::temp_directory_path() / std::move(name);

        std::ofstream file(m_path, std::ios::binary | std::ios::trunc);
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    }

    ~TestEnvironmentFile()
    {
        std::error_code error;
        std::filesystem::remove(m_path, error);
    }

    TestEnvironmentFile(const TestEnvironmentFile &) = delete;
    TestEnvironmentFile &operator=(const TestEnvironmentFile &) = delete;
    TestEnvironmentFile(TestEnvironmentFile &&) = delete;
    TestEnvironmentFile &operator=(TestEnvironmentFile &&) = delete;

    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

TEST_CASE("PtyEnviron builds its initial environment from process environ", "[job_io][pty][environ]")
{
    PtyEnviron environment;

    REQUIRE(environment.buildInitList());
    REQUIRE_FALSE(environment.initial().empty());
    REQUIRE_FALSE(environment.resolved().empty());
    REQUIRE(environment.initial().size() == environment.resolved().size());
}

TEST_CASE("PtyEnviron captures a known process environment variable", "[job_io][pty][environ]")
{
    constexpr const char *name = "JOB_PTY_ENVIRON_CAPTURE_TEST";

    TestEnvironmentGuard guard(name);

    REQUIRE(::setenv(name, "captured-value", 1) == 0);

    PtyEnviron environment;

    REQUIRE(environment.buildInitList());
    REQUIRE(environment.containsInitial(name));
    REQUIRE(environment.contains(name));

    const PtyEnv *initial = environment.initialEnv(name);
    const PtyEnv *resolved = environment.env(name);

    REQUIRE(initial != nullptr);
    REQUIRE(resolved != nullptr);

    REQUIRE(initial->value() == "captured-value");
    REQUIRE(resolved->value() == "captured-value");
    REQUIRE(initial->enabled());
    REQUIRE(resolved->enabled());
}

TEST_CASE("PtyEnviron rebuilds its initial environment from current process state", "[job_io][pty][environ]")
{
    constexpr const char *name = "JOB_PTY_ENVIRON_REBUILD_TEST";

    TestEnvironmentGuard guard(name);

    REQUIRE(::setenv(name, "first", 1) == 0);

    PtyEnviron environment;

    REQUIRE(environment.buildInitList());
    REQUIRE(environment.value(name) == "first");

    REQUIRE(::setenv(name, "second", 1) == 0);

    REQUIRE(environment.buildInitList());
    REQUIRE(environment.value(name) == "second");

    const PtyEnv *initial = environment.initialEnv(name);

    REQUIRE(initial != nullptr);
    REQUIRE(initial->value() == "second");
}

TEST_CASE("PtyEnviron initial lookup reports missing entries", "[job_io][pty][environ][initial]")
{
    PtyEnviron environment;

    REQUIRE(environment.buildInitList());

    REQUIRE_FALSE(environment.containsInitial("JOB_ENVIRONMENT_VARIABLE_THAT_DOES_NOT_EXIST"));
    REQUIRE(environment.initialIndexOf("JOB_ENVIRONMENT_VARIABLE_THAT_DOES_NOT_EXIST") == PtyEnviron::npos);
    REQUIRE(environment.initialEnv("JOB_ENVIRONMENT_VARIABLE_THAT_DOES_NOT_EXIST") == nullptr);
}

TEST_CASE("PtyEnviron resolved lookup reports missing entries", "[job_io][pty][environ]")
{
    PtyEnviron environment;

    REQUIRE(environment.buildInitList());

    REQUIRE_FALSE(environment.contains("JOB_ENVIRONMENT_VARIABLE_THAT_DOES_NOT_EXIST"));
    REQUIRE(environment.indexOf("JOB_ENVIRONMENT_VARIABLE_THAT_DOES_NOT_EXIST") == PtyEnviron::npos);
    REQUIRE(environment.env("JOB_ENVIRONMENT_VARIABLE_THAT_DOES_NOT_EXIST") == nullptr);
}

TEST_CASE("PtyEnviron can replace its resolved environment", "[job_io][pty][environ]")
{
    PtyEnviron environment;

    PtyEnviron::EnvList resolved;
    resolved.emplace_back("JOB_ALPHA", "one", true, PtyEnv::EnvType::Custom);
    resolved.emplace_back("JOB_BETA", "two", true, PtyEnv::EnvType::Custom);

    environment.setResolved(std::move(resolved));

    REQUIRE(environment.size() == 2);
    REQUIRE(environment.contains("JOB_ALPHA"));
    REQUIRE(environment.contains("JOB_BETA"));
    REQUIRE(environment.value("JOB_ALPHA") == "one");
    REQUIRE(environment.value("JOB_BETA") == "two");
}

TEST_CASE("PtyEnviron resetResolved restores initial environment", "[job_io][pty][environ]")
{
    constexpr const char *name = "JOB_PTY_ENVIRON_RESET_TEST";

    TestEnvironmentGuard guard(name);

    REQUIRE(::setenv(name, "initial", 1) == 0);

    PtyEnviron environment;

    REQUIRE(environment.buildInitList());
    REQUIRE(environment.value(name) == "initial");

    REQUIRE(environment.set(name, "changed"));
    REQUIRE(environment.value(name) == "changed");

    environment.resetResolved();

    REQUIRE(environment.value(name) == "initial");
}

TEST_CASE("PtyEnviron adds a new environment entry", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.add(PtyEnv("JOB_TEST", "value", true, PtyEnv::EnvType::Custom)));

    REQUIRE(environment.size() == 1);
    REQUIRE(environment.contains("JOB_TEST"));
    REQUIRE(environment.value("JOB_TEST") == "value");
}

TEST_CASE("PtyEnviron rejects empty and duplicate environment entries", "[job_io][pty][environ][mutation][edge]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE_FALSE(environment.add(PtyEnv("", "value", true, PtyEnv::EnvType::Custom)));

    REQUIRE(environment.add(PtyEnv("JOB_TEST", "one", true, PtyEnv::EnvType::Custom)));
    REQUIRE_FALSE(environment.add(PtyEnv("JOB_TEST", "two", true, PtyEnv::EnvType::Custom)));

    REQUIRE(environment.size() == 1);
    REQUIRE(environment.value("JOB_TEST") == "one");
}

TEST_CASE("PtyEnviron set creates and replaces entries", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("JOB_TEST", "one", true, PtyEnv::EnvType::Custom));

    REQUIRE(environment.contains("JOB_TEST"));
    REQUIRE(environment.value("JOB_TEST") == "one");

    REQUIRE(environment.set("JOB_TEST", "two", false, PtyEnv::EnvType::Posix));

    PtyEnv *entry = environment.env("JOB_TEST");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->value() == "two");
    REQUIRE_FALSE(entry->enabled());
    REQUIRE(entry->envType() == PtyEnv::EnvType::Posix);
}

TEST_CASE("PtyEnviron set rejects an empty name", "[job_io][pty][environ][mutation][edge]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE_FALSE(environment.set("", "value"));
    REQUIRE(environment.isEmpty());
}

TEST_CASE("PtyEnviron setDefault preserves an existing value", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("JOB_TEST", "explicit"));
    REQUIRE(environment.setDefault("JOB_TEST", "default"));

    REQUIRE(environment.value("JOB_TEST") == "explicit");
}

TEST_CASE("PtyEnviron setDefault creates a missing value", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.setDefault("JOB_TEST", "default"));

    REQUIRE(environment.contains("JOB_TEST"));
    REQUIRE(environment.value("JOB_TEST") == "default");
}

TEST_CASE("PtyEnviron append adds data to an existing value", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("PATH", "/bin"));
    REQUIRE(environment.append("PATH", "/usr/bin", ":"));

    REQUIRE(environment.value("PATH") == "/bin:/usr/bin");
}

TEST_CASE("PtyEnviron append creates a missing value", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.append("PATH", "/usr/bin", ":"));

    REQUIRE(environment.value("PATH") == "/usr/bin");
}

TEST_CASE("PtyEnviron append does not add separator for empty values", "[job_io][pty][environ][mutation][edge]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("PATH", ""));
    REQUIRE(environment.append("PATH", "/usr/bin", ":"));
    REQUIRE(environment.value("PATH") == "/usr/bin");

    REQUIRE(environment.append("PATH", "", ":"));
    REQUIRE(environment.value("PATH") == "/usr/bin");
}

TEST_CASE("PtyEnviron prepend adds data before an existing value", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("PATH", "/usr/bin"));
    REQUIRE(environment.prepend("PATH", "/bin", ":"));

    REQUIRE(environment.value("PATH") == "/bin:/usr/bin");
}

TEST_CASE("PtyEnviron prepend creates a missing value", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.prepend("PATH", "/bin", ":"));

    REQUIRE(environment.value("PATH") == "/bin");
}

TEST_CASE("PtyEnviron prepend ignores an empty value", "[job_io][pty][environ][mutation][edge]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("PATH", "/usr/bin"));
    REQUIRE(environment.prepend("PATH", "", ":"));

    REQUIRE(environment.value("PATH") == "/usr/bin");
}

TEST_CASE("PtyEnviron removes environment entries", "[job_io][pty][environ][mutation]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("JOB_ONE", "one"));
    REQUIRE(environment.set("JOB_TWO", "two"));

    REQUIRE(environment.size() == 2);

    REQUIRE(environment.remove("JOB_ONE"));

    REQUIRE(environment.size() == 1);
    REQUIRE_FALSE(environment.contains("JOB_ONE"));
    REQUIRE(environment.contains("JOB_TWO"));

    REQUIRE_FALSE(environment.remove("JOB_ONE"));
}

TEST_CASE("PtyEnviron value returns default for missing or disabled entries", "[job_io][pty][environ]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.value("MISSING", "fallback") == "fallback");

    REQUIRE(environment.set("JOB_DISABLED", "secret", false));
    REQUIRE(environment.value("JOB_DISABLED", "fallback") == "fallback");

    REQUIRE(environment.set("JOB_ENABLED", "visible", true));
    REQUIRE(environment.value("JOB_ENABLED", "fallback") == "visible");
}

TEST_CASE("PtyEnviron clearResolved removes only resolved environment", "[job_io][pty][environ]")
{
    PtyEnviron environment;

    REQUIRE(environment.buildInitList());

    const std::size_t initialSize = environment.initial().size();

    REQUIRE(initialSize > 0);

    environment.clearResolved();

    REQUIRE(environment.isEmpty());
    REQUIRE(environment.size() == 0);
    REQUIRE(environment.initial().size() == initialSize);
}

TEST_CASE("PtyEnviron exports enabled entries as name value strings", "[job_io][pty][environ][strings]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("JOB_ONE", "alpha"));
    REQUIRE(environment.set("JOB_TWO", "beta"));
    REQUIRE(environment.set("JOB_DISABLED", "hidden", false));

    const auto strings = environment.toStrings();

    REQUIRE(strings.size() == 2);
    REQUIRE(strings[0] == "JOB_ONE=alpha");
    REQUIRE(strings[1] == "JOB_TWO=beta");
}

TEST_CASE("PtyEnviron exports empty enabled values", "[job_io][pty][environ][strings]")
{
    PtyEnviron environment;
    environment.clearResolved();

    REQUIRE(environment.set("JOB_EMPTY", ""));

    const auto strings = environment.toStrings();

    REQUIRE(strings.size() == 1);
    REQUIRE(strings[0] == "JOB_EMPTY=");
}

TEST_CASE("PtyEnviron manages dot environment sources", "[job_io][pty][environ][dot]")
{
    PtyEnviron environment;

    REQUIRE(environment.addDotFile(
        PtyDot("/etc/profile", PtyDot::Role::SystemProfile, PtyDot::Format::Shell)));

    REQUIRE(environment.addDotFile(
        PtyDot("/home/job/.profile", PtyDot::Role::UserProfile, PtyDot::Format::Shell)));

    REQUIRE(environment.dotFiles().size() == 2);

    REQUIRE(environment.containsDotFile("/etc/profile"));
    REQUIRE(environment.containsDotFile("/home/job/.profile"));

    REQUIRE(environment.dotFileIndexOf("/etc/profile") != PtyEnviron::npos);
    REQUIRE(environment.dotFileIndexOf("/home/job/.profile") != PtyEnviron::npos);

    REQUIRE(environment.dotFile("/etc/profile") != nullptr);
    REQUIRE(environment.dotFile("/home/job/.profile") != nullptr);
}

TEST_CASE("PtyEnviron rejects empty and duplicate dot sources", "[job_io][pty][environ][dot][edge]")
{
    PtyEnviron environment;

    REQUIRE_FALSE(environment.addDotFile(
        PtyDot("", PtyDot::Role::Custom, PtyDot::Format::Custom)));

    REQUIRE(environment.addDotFile(
        PtyDot("/tmp/job.env", PtyDot::Role::Project, PtyDot::Format::DotEnv)));

    REQUIRE_FALSE(environment.addDotFile(
        PtyDot("/tmp/job.env", PtyDot::Role::Custom, PtyDot::Format::Custom)));

    REQUIRE(environment.dotFiles().size() == 1);
}

TEST_CASE("PtyEnviron removes dot environment sources", "[job_io][pty][environ][dot]")
{
    PtyEnviron environment;

    REQUIRE(environment.addDotFile(
        PtyDot("/tmp/job.env", PtyDot::Role::Project, PtyDot::Format::DotEnv)));

    REQUIRE(environment.containsDotFile("/tmp/job.env"));
    REQUIRE(environment.removeDotFile("/tmp/job.env"));

    REQUIRE_FALSE(environment.containsDotFile("/tmp/job.env"));
    REQUIRE(environment.dotFile("/tmp/job.env") == nullptr);
    REQUIRE_FALSE(environment.removeDotFile("/tmp/job.env"));
}

TEST_CASE("PtyEnviron hashes enabled dot files", "[job_io][pty][environ][dot][hash]")
{
    TestEnvironmentFile file("job_pty_environ_hash_test.env", "JOB_TEST=value\n");

    PtyEnviron environment;

    REQUIRE(environment.addDotFile(
        PtyDot(file.path().string(), PtyDot::Role::Project, PtyDot::Format::DotEnv)));

    REQUIRE(environment.hashDotFiles());

    const PtyDot *dot = environment.dotFile(file.path().string());

    REQUIRE(dot != nullptr);
    REQUIRE(dot->hasHash());
    REQUIRE_FALSE(dot->hash().empty());
}

TEST_CASE("PtyEnviron skips disabled dot files when hashing", "[job_io][pty][environ][dot][hash]")
{
    PtyEnviron environment;

    REQUIRE(environment.addDotFile(
        PtyDot("/this/file/does/not/exist",
               PtyDot::Role::Custom,
               PtyDot::Format::Custom,
               false)));

    REQUIRE(environment.hashDotFiles());

    const PtyDot *dot = environment.dotFile("/this/file/does/not/exist");

    REQUIRE(dot != nullptr);
    REQUIRE_FALSE(dot->hasHash());
}

TEST_CASE("PtyEnviron reports dot hash failures and continues hashing", "[job_io][pty][environ][dot][hash][edge]")
{
    TestEnvironmentFile file("job_pty_environ_partial_hash_test.env", "JOB_VALID=1\n");

    PtyEnviron environment;

    REQUIRE(environment.addDotFile(
        PtyDot(file.path().string(), PtyDot::Role::Project, PtyDot::Format::DotEnv)));

    REQUIRE(environment.addDotFile(
        PtyDot("/this/file/does/not/exist",
               PtyDot::Role::Custom,
               PtyDot::Format::Custom)));

    REQUIRE_FALSE(environment.hashDotFiles());

    const PtyDot *valid = environment.dotFile(file.path().string());
    const PtyDot *invalid = environment.dotFile("/this/file/does/not/exist");

    REQUIRE(valid != nullptr);
    REQUIRE(invalid != nullptr);

    REQUIRE(valid->hasHash());
    REQUIRE_FALSE(valid->hash().empty());

    REQUIRE_FALSE(invalid->hasHash());
    REQUIRE(invalid->hash().empty());
}

TEST_CASE("PtyEnviron clearDotHashes removes all stored hashes", "[job_io][pty][environ][dot][hash]")
{
    PtyEnviron environment;

    REQUIRE(environment.addDotFile(
        PtyDot("/tmp/one", PtyDot::Role::Custom, PtyDot::Format::Custom)));

    REQUIRE(environment.addDotFile(
        PtyDot("/tmp/two", PtyDot::Role::Custom, PtyDot::Format::Custom)));

    environment.dotFiles()[0].setHash({0x01, 0x02});
    environment.dotFiles()[1].setHash({0x03, 0x04});

    REQUIRE(environment.dotFiles()[0].hasHash());
    REQUIRE(environment.dotFiles()[1].hasHash());

    environment.clearDotHashes();

    REQUIRE_FALSE(environment.dotFiles()[0].hasHash());
    REQUIRE_FALSE(environment.dotFiles()[1].hasHash());
}

TEST_CASE("PtyEnviron createShared constructs a shared instance", "[job_io][pty][environ]")
{
    auto environment = PtyEnviron::createShared();

    REQUIRE(environment);
    REQUIRE(environment->isEmpty());
    REQUIRE(environment->size() == 0);
}

TEST_CASE("PtyEnviron createUniq constructs a unique instance", "[job_io][pty][environ]")
{
    auto environment = PtyEnviron::createUniq();

    REQUIRE(environment);
    REQUIRE(environment->isEmpty());
    REQUIRE(environment->size() == 0);
}