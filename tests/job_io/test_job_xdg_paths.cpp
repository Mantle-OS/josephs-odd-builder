#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_dir.h>
#include <job_permissions.h>
#include <job_syslink.h>
#include <job_tmp_dir.h>
#include <job_tmp_file.h>
#include <job_xdg_paths.h>

namespace job::io::test {

[[nodiscard]] std::filesystem::path tmpXdgPath(std::string_view name)
{
    return std::filesystem::temp_directory_path() /
           ("job_xdg_" + std::to_string(::getpid()) + "_" + std::string(name));
}

class ScopedEnv final
{
public:
    ScopedEnv() = default;

    ~ScopedEnv()
    {
        for (auto it = m_original.rbegin(); it != m_original.rend(); ++it) {
            if (it->value)
                ::setenv(it->name.c_str(), it->value->c_str(), 1);
            else
                ::unsetenv(it->name.c_str());
        }
    }

    ScopedEnv(const ScopedEnv &) = delete;
    ScopedEnv &operator=(const ScopedEnv &) = delete;
    ScopedEnv(ScopedEnv &&) = delete;
    ScopedEnv &operator=(ScopedEnv &&) = delete;

    void set(std::string_view name, std::string_view value)
    {
        remember(name);
        REQUIRE(::setenv(std::string{name}.c_str(), std::string{value}.c_str(), 1) == 0);
    }

    void unset(std::string_view name)
    {
        remember(name);
        REQUIRE(::unsetenv(std::string{name}.c_str()) == 0);
    }

private:
    struct Original final
    {
        std::string name;
        std::optional<std::string> value;
    };

    void remember(std::string_view name)
    {
        for (const auto &entry : m_original) {
            if (entry.name == name)
                return;
        }

        const std::string key{name};
        const char *value = ::getenv(key.c_str());

        m_original.push_back({
            .name = key,
            .value = value ? std::optional<std::string>{value} : std::nullopt
        });
    }

    std::vector<Original> m_original;
};

static void clearXdgEnvironment(ScopedEnv &env)
{
    env.unset("XDG_DATA_HOME");
    env.unset("XDG_CONFIG_HOME");
    env.unset("XDG_STATE_HOME");
    env.unset("XDG_CACHE_HOME");
    env.unset("XDG_RUNTIME_DIR");
    env.unset("XDG_DATA_DIRS");
    env.unset("XDG_CONFIG_DIRS");
}

} // namespace job::io::test

using namespace job::io;
using namespace job::io::test;

//////////////////////////////////////////////////////////
// Block 1: usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobXdgPaths resolves standard XDG defaults from HOME",
          "[io][xdg][paths][usage]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");

    REQUIRE(JobXdgPaths::dataHome() == "/tmp/job-xdg-home/.local/share");
    REQUIRE(JobXdgPaths::configHome() == "/tmp/job-xdg-home/.config");
    REQUIRE(JobXdgPaths::stateHome() == "/tmp/job-xdg-home/.local/state");
    REQUIRE(JobXdgPaths::cacheHome() == "/tmp/job-xdg-home/.cache");
    REQUIRE(JobXdgPaths::executableHome() == "/tmp/job-xdg-home/.local/bin");

    REQUIRE(JobXdgPaths::dataDirs() ==
            JobXdgPaths::PathList{
                "/usr/local/share",
                "/usr/share"
            });

    REQUIRE(JobXdgPaths::configDirs() ==
            JobXdgPaths::PathList{
                "/etc/xdg"
            });
}

TEST_CASE("JobXdgPaths uses explicit XDG home directories",
          "[io][xdg][paths][usage]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "/opt/job/data");
    env.set("XDG_CONFIG_HOME", "/opt/job/config");
    env.set("XDG_STATE_HOME", "/opt/job/state");
    env.set("XDG_CACHE_HOME", "/opt/job/cache");

    REQUIRE(JobXdgPaths::dataHome() == "/opt/job/data");
    REQUIRE(JobXdgPaths::configHome() == "/opt/job/config");
    REQUIRE(JobXdgPaths::stateHome() == "/opt/job/state");
    REQUIRE(JobXdgPaths::cacheHome() == "/opt/job/cache");
}

TEST_CASE("JobXdgPaths builds ordered data and config search paths",
          "[io][xdg][paths][search][usage]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "/job/data-home");
    env.set("XDG_DATA_DIRS", "/job/data-a:/job/data-b");
    env.set("XDG_CONFIG_HOME", "/job/config-home");
    env.set("XDG_CONFIG_DIRS", "/job/config-a:/job/config-b");

    REQUIRE(JobXdgPaths::dataSearchPaths() ==
            JobXdgPaths::PathList{
                "/job/data-home",
                "/job/data-a",
                "/job/data-b"
            });

    REQUIRE(JobXdgPaths::configSearchPaths() ==
            JobXdgPaths::PathList{
                "/job/config-home",
                "/job/config-a",
                "/job/config-b"
            });

    REQUIRE(JobXdgPaths::searchPaths(PtyEnvXdg::SearchGroup::Data) ==
            JobXdgPaths::dataSearchPaths());

    REQUIRE(JobXdgPaths::searchPaths(PtyEnvXdg::SearchGroup::Config) ==
            JobXdgPaths::configSearchPaths());
}

TEST_CASE("JobXdgPaths derives common resource directories from data search roots",
          "[io][xdg][paths][resources][usage]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "/job/data-home");
    env.set("XDG_DATA_DIRS", "/job/data-a:/job/data-b");

    REQUIRE(JobXdgPaths::applicationDirs() ==
            JobXdgPaths::PathList{
                "/job/data-home/applications",
                "/job/data-a/applications",
                "/job/data-b/applications"
            });

    REQUIRE(JobXdgPaths::desktopDirectoryDirs() ==
            JobXdgPaths::PathList{
                "/job/data-home/desktop-directories",
                "/job/data-a/desktop-directories",
                "/job/data-b/desktop-directories"
            });

    REQUIRE(JobXdgPaths::iconDirs() ==
            JobXdgPaths::PathList{
                "/job/data-home/icons",
                "/job/data-a/icons",
                "/job/data-b/icons"
            });

    REQUIRE(JobXdgPaths::mimeDirs() ==
            JobXdgPaths::PathList{
                "/job/data-home/mime",
                "/job/data-a/mime",
                "/job/data-b/mime"
            });
}

TEST_CASE("JobXdgPaths builds writable paths beneath each XDG home",
          "[io][xdg][paths][writable][usage]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "/job/data");
    env.set("XDG_CONFIG_HOME", "/job/config");
    env.set("XDG_STATE_HOME", "/job/state");
    env.set("XDG_CACHE_HOME", "/job/cache");

    REQUIRE(JobXdgPaths::dataPath("applications/job.desktop") ==
            "/job/data/applications/job.desktop");

    REQUIRE(JobXdgPaths::configPath("job/settings.conf") ==
            "/job/config/job/settings.conf");

    REQUIRE(JobXdgPaths::statePath("job/session.state") ==
            "/job/state/job/session.state");

    REQUIRE(JobXdgPaths::cachePath("job/index.cache") ==
            "/job/cache/job/index.cache");
}

TEST_CASE("JobXdgPaths locate follows XDG search priority",
          "[io][xdg][paths][locate][usage]")
{
    JobTmpDir root(tmpXdgPath("locate"));

    const auto home = root.path() / "home";
    const auto homeData = root.path() / "home-data";
    const auto dataA = root.path() / "data-a";
    const auto dataB = root.path() / "data-b";

    REQUIRE(JobDir::createParents(home));
    REQUIRE(JobDir::createParents(dataA / "applications"));
    REQUIRE(JobDir::createParents(dataB / "applications"));

    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", home.string());
    env.set("XDG_DATA_HOME", homeData.string());
    env.set("XDG_DATA_DIRS", dataA.string() + ":" + dataB.string());

    JobTmpFile firstFile(
        dataA / "applications/example.desktop",
        std::string_view(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=First\n"
            "Exec=/bin/true\n"));

    JobTmpFile secondFile(
        dataB / "applications/example.desktop",
        std::string_view(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=Second\n"
            "Exec=/bin/true\n"));

    const auto first = JobXdgPaths::locate(
        PtyEnvXdg::SearchGroup::Data,
        "applications/example.desktop");

    REQUIRE(first);
    REQUIRE(*first == firstFile.path());

    const auto all = JobXdgPaths::locateAll(
        PtyEnvXdg::SearchGroup::Data,
        "applications/example.desktop");

    REQUIRE(all ==
            JobXdgPaths::PathList{
                firstFile.path(),
                secondFile.path()
            });
}

//////////////////////////////////////////////////////////
// Block 2: edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobXdgPaths ignores relative XDG home paths",
          "[io][xdg][paths][edge]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "relative/data");
    env.set("XDG_CONFIG_HOME", "relative/config");
    env.set("XDG_STATE_HOME", "relative/state");
    env.set("XDG_CACHE_HOME", "relative/cache");

    REQUIRE(JobXdgPaths::dataHome() == "/tmp/job-xdg-home/.local/share");
    REQUIRE(JobXdgPaths::configHome() == "/tmp/job-xdg-home/.config");
    REQUIRE(JobXdgPaths::stateHome() == "/tmp/job-xdg-home/.local/state");
    REQUIRE(JobXdgPaths::cacheHome() == "/tmp/job-xdg-home/.cache");
}

TEST_CASE("JobXdgPaths filters invalid entries from XDG path lists",
          "[io][xdg][paths][list][edge]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_DIRS",
            "/valid/a:relative-a::/valid/b:relative-b");

    REQUIRE(JobXdgPaths::dataDirs() ==
            JobXdgPaths::PathList{
                "/valid/a",
                "/valid/b"
            });
}

TEST_CASE("JobXdgPaths rejects requests using the wrong XDG value type",
          "[io][xdg][paths][edge]")
{
    REQUIRE_FALSE(JobXdgPaths::path(PtyEnvXdg::Env::DataDirs));
    REQUIRE_FALSE(JobXdgPaths::path(PtyEnvXdg::Env::ConfigDirs));

    REQUIRE(JobXdgPaths::paths(PtyEnvXdg::Env::DataHome).empty());
    REQUIRE(JobXdgPaths::paths(PtyEnvXdg::Env::ConfigHome).empty());
}

TEST_CASE("JobXdgPaths rejects invalid resource lookup paths",
          "[io][xdg][paths][locate][edge]")
{
    REQUIRE_FALSE(JobXdgPaths::locate(
        PtyEnvXdg::SearchGroup::Data,
        {}));

    REQUIRE_FALSE(JobXdgPaths::locate(
        PtyEnvXdg::SearchGroup::Data,
        "/absolute/path"));

    REQUIRE_FALSE(JobXdgPaths::locate(
        PtyEnvXdg::SearchGroup::Data,
        "../escape"));

    REQUIRE(JobXdgPaths::locateAll(
                PtyEnvXdg::SearchGroup::Data,
                "../escape").empty());

    REQUIRE(JobXdgPaths::dataPath("../escape").empty());
    REQUIRE(JobXdgPaths::configPath("../escape").empty());
    REQUIRE(JobXdgPaths::statePath("../escape").empty());
    REQUIRE(JobXdgPaths::cachePath("../escape").empty());
}

TEST_CASE("JobXdgPaths returns no search paths for no search group", "[io][xdg][paths][search][edge]")
{
    REQUIRE(JobXdgPaths::searchPaths(PtyEnvXdg::SearchGroup::None).empty());
}

TEST_CASE("JobXdgPaths validates a private runtime directory",
          "[io][xdg][paths][runtime][usage]")
{
    JobTmpDir runtime(tmpXdgPath("runtime"));

    REQUIRE(runtime.dir().hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE(JobXdgPaths::validateRuntimeDir(runtime.path()));

    ScopedEnv env;
    env.set("XDG_RUNTIME_DIR", runtime.pathString());

    const auto resolved = JobXdgPaths::runtimeDir();
    REQUIRE(resolved);
    REQUIRE(*resolved == runtime.path());
}


TEST_CASE("JobXdgPaths rejects runtime directory with non-private permissions",
          "[io][xdg][paths][runtime][edge]")
{
    JobTmpDir runtime(
        tmpXdgPath("runtime_mode"),
        IOPermissions::DefaultDirectory);

    REQUIRE_FALSE(runtime.dir().hasPermissions(IOPermissions::PrivateDirectory));
    REQUIRE_FALSE(JobXdgPaths::validateRuntimeDir(runtime.path()));

    ScopedEnv env;
    env.set("XDG_RUNTIME_DIR", runtime.pathString());

    REQUIRE_FALSE(JobXdgPaths::runtimeDir());
}


TEST_CASE("JobXdgPaths rejects runtime directory symlink",
          "[io][xdg][paths][runtime][symlink][edge]")
{
    JobTmpDir root(tmpXdgPath("runtime_link"));

    const auto target = root.path() / "target";
    const auto link = root.path() / "runtime";

    REQUIRE(JobDir::create(target, IOPermissions::PrivateDirectory));
    REQUIRE(JobSysLink::createDirectoryLink(link, target));
    REQUIRE(JobSysLink::isSysLink(link));

    REQUIRE_FALSE(JobXdgPaths::validateRuntimeDir(link));
}


TEST_CASE("JobXdgPaths rejects missing runtime directory",
          "[io][xdg][paths][runtime][edge]")
{
    JobTmpDir root(tmpXdgPath("missing_runtime"));
    const auto path = root.path() / "does-not-exist";

    REQUIRE_FALSE(JobXdgPaths::validateRuntimeDir(path));

    ScopedEnv env;
    env.set("XDG_RUNTIME_DIR", path.string());

    REQUIRE_FALSE(JobXdgPaths::runtimeDir());
}


TEST_CASE("JobXdgPaths ensureWritableDirectory creates missing parents privately",
          "[io][xdg][paths][writable][usage]")
{
    JobTmpDir root(tmpXdgPath("create"));
    const auto destination = root.path() / "a/b/c";

    REQUIRE(JobXdgPaths::ensureWritableDirectory(destination));

    JobDir dir(destination);

    REQUIRE(dir.exists());
    REQUIRE(dir.isDirectory());
    REQUIRE(dir.hasPermissions(IOPermissions::PrivateDirectory));
}


TEST_CASE("JobXdgPaths ensureWritableDirectory rejects relative path",
          "[io][xdg][paths][writable][edge]")
{
    REQUIRE_FALSE(JobXdgPaths::ensureWritableDirectory("relative/path"));
}

TEST_CASE("JobXdgPaths ensureWritableDirectory does not chmod existing directory",
          "[io][xdg][paths][writable][edge]")
{
    JobTmpDir root(
        tmpXdgPath("existing"),
        IOPermissions::DefaultDirectory);

    REQUIRE(root.dir().hasPermissions(IOPermissions::DefaultDirectory));
    REQUIRE(JobXdgPaths::ensureWritableDirectory(root.path()));
    REQUIRE(root.dir().hasPermissions(IOPermissions::DefaultDirectory));
}

//////////////////////////////////////////////////////////
// Block 3: benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobXdgPaths search path benchmark",
          "[io][xdg][paths][benchmark]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "/job/data-home");
    env.set("XDG_DATA_DIRS",
            "/job/a:/job/b:/job/c:/job/d:/job/e:/job/f:/job/g:/job/h");

    BENCHMARK("dataSearchPaths")
    {
        return JobXdgPaths::dataSearchPaths();
    };
}

TEST_CASE("JobXdgPaths resource path benchmark",
          "[io][xdg][paths][benchmark]")
{
    ScopedEnv env;
    clearXdgEnvironment(env);

    env.set("HOME", "/tmp/job-xdg-home");
    env.set("XDG_DATA_HOME", "/job/data-home");

    BENCHMARK("dataPath")
    {
        return JobXdgPaths::dataPath("applications/org.job.example.desktop");
    };
}

#endif