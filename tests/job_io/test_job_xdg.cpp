#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_fifo_scheduler.h>
#include <job_thread_pool.h>

#include <job_dir.h>
#include <job_tmp_dir.h>
#include <job_tmp_file.h>
#include <job_xdg.h>

namespace job::io::test {

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

[[nodiscard]] std::filesystem::path tmpXdgCatalogPath(std::string_view name)
{
    static std::size_t counter = 0;

    return std::filesystem::temp_directory_path() /
           ("job_xdg_catalog_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + "_" + std::string{name});
}




class XdgTestTree final
{
public:
    explicit XdgTestTree(std::string_view name) :
        m_root(tmpXdgCatalogPath(name)),
        m_dataHome(m_root.path() / "data-home"),
        m_dataA(m_root.path() / "data-a"),
        m_dataB(m_root.path() / "data-b")
    {
        REQUIRE(JobDir::createParents(applicationsHome()));
        REQUIRE(JobDir::createParents(applicationsA()));
        REQUIRE(JobDir::createParents(applicationsB()));

        m_env.unset("XDG_CONFIG_HOME");
        m_env.unset("XDG_STATE_HOME");
        m_env.unset("XDG_CACHE_HOME");
        m_env.unset("XDG_RUNTIME_DIR");
        m_env.unset("XDG_CONFIG_DIRS");

        m_env.set("HOME", (m_root.path() / "home").string());
        m_env.set("XDG_DATA_HOME", m_dataHome.string());
        m_env.set("XDG_DATA_DIRS", m_dataA.string() + ":" + m_dataB.string());
    }

    ~XdgTestTree() = default;

    XdgTestTree(const XdgTestTree &) = delete;
    XdgTestTree &operator=(const XdgTestTree &) = delete;
    XdgTestTree(XdgTestTree &&) = delete;
    XdgTestTree &operator=(XdgTestTree &&) = delete;

    [[nodiscard]] const std::filesystem::path &root() const noexcept
    {
        return m_root.path();
    }

    [[nodiscard]] std::filesystem::path applicationsHome() const
    {
        return m_dataHome / "applications";
    }

    [[nodiscard]] std::filesystem::path applicationsA() const
    {
        return m_dataA / "applications";
    }

    [[nodiscard]] std::filesystem::path applicationsB() const
    {
        return m_dataB / "applications";
    }

    JobTmpFile &desktopFile(const std::filesystem::path &applicationsDir,
                            const std::filesystem::path &relativePath,
                            std::string_view source)
    {
        const auto path = applicationsDir / relativePath;

        if (relativePath.has_parent_path())
            REQUIRE(JobDir::createParents(path.parent_path()));

        m_files.push_back(JobTmpFile::createUniq(path, source));
        return *m_files.back();
    }

private:
    ScopedEnv m_env;
    JobTmpDir m_root;
    std::filesystem::path m_dataHome;
    std::filesystem::path m_dataA;
    std::filesystem::path m_dataB;
    std::vector<JobTmpFile::UPtr> m_files;
};

[[nodiscard]] std::string applicationSource(std::string_view name,
                                            std::string_view exec = "/bin/true",
                                            std::string_view extra = {})
{
    std::string source =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=" + std::string{name} + "\n";

    if (!exec.empty())
        source += "Exec=" + std::string{exec} + "\n";

    source += extra;
    return source;
}

[[nodiscard]] std::string linkSource(std::string_view name,
                                     std::string_view url,
                                     std::string_view extra = {})
{
    return
        "[Desktop Entry]\n"
        "Type=Link\n"
        "Name=" + std::string{name} + "\n"
        "URL=" + std::string{url} + "\n" +
        std::string{extra};
}

[[nodiscard]] const JobXdgEntry *requireEntry(JobXdg &xdg,
                                              std::string_view desktopFileId)
{
    const JobXdgEntry *entry = xdg.find(desktopFileId);
    REQUIRE(entry != nullptr);
    return entry;
}

} // namespace job::io::test

using namespace job::io;
using namespace job::io::test;

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobXdg instance is a process-wide singleton",
          "[job_io][xdg][usage][singleton]")
{
    JobXdg &first = JobXdg::instance();
    JobXdg &second = JobXdg::instance();

    REQUIRE(&first == &second);
    REQUIRE(&first.paths() == &second.paths());
}

TEST_CASE("JobXdg discovers and parses an application desktop entry",
          "[job_io][xdg][usage][catalog][application]")
{
    XdgTestTree tree("application");

    tree.desktopFile(
        tree.applicationsHome(),
        "org.job.Example.desktop",
        applicationSource(
            "JOB Example",
            "/usr/bin/example --verbose",
            "Comment=Example desktop application\n"
            "Icon=job-example\n"
            "Categories=Utility;Development;\n"
            "MimeType=text/plain;application/json;\n"
            "StartupNotify=true\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE(xdg.size() == 1);
    REQUIRE_FALSE(xdg.empty());
    REQUIRE(xdg.contains("org.job.Example.desktop"));

    const JobXdgEntry *entry = requireEntry(xdg, "org.job.Example.desktop");

    REQUIRE(entry->desktopFileId() == "org.job.Example.desktop");
    REQUIRE(entry->type() == JobXdgEntryType::Application);
    REQUIRE(entry->name().value == "JOB Example");
    REQUIRE(entry->comment().value == "Example desktop application");
    REQUIRE(entry->icon().value == "job-example");
    REQUIRE(entry->exec() == "/usr/bin/example --verbose");
    REQUIRE(entry->categories() == std::vector<std::string>{"Utility", "Development"});
    REQUIRE(entry->mimeTypes() == std::vector<std::string>{"text/plain", "application/json"});
    REQUIRE(entry->startupNotify() == std::optional<bool>{true});
}

TEST_CASE("JobXdg parses localized presentation fields",
          "[job_io][xdg][usage][localized]")
{
    XdgTestTree tree("localized");

    tree.desktopFile(
        tree.applicationsHome(),
        "localized.desktop",
        applicationSource(
            "Terminal",
            "/bin/true",
            "Name[de]=Terminal DE\n"
            "Name[fr]=Terminal FR\n"
            "GenericName=Terminal Emulator\n"
            "GenericName[de]=Terminalemulator\n"
            "Comment=Open a terminal\n"
            "Comment[de]=Ein Terminal öffnen\n"
            "Icon=utilities-terminal\n"
            "Keywords=shell;command;\n"
            "Keywords[de]=Shell;Befehl;\n"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "localized.desktop");

    REQUIRE(entry->name().value == "Terminal");
    REQUIRE(entry->name().localized.at("de") == "Terminal DE");
    REQUIRE(entry->name().localized.at("fr") == "Terminal FR");

    REQUIRE(entry->genericName().value == "Terminal Emulator");
    REQUIRE(entry->genericName().localized.at("de") == "Terminalemulator");

    REQUIRE(entry->comment().value == "Open a terminal");
    REQUIRE(entry->comment().localized.at("de") == "Ein Terminal öffnen");

    REQUIRE(entry->icon().value == "utilities-terminal");
    REQUIRE(entry->keywords().value == "shell;command;");
    REQUIRE(entry->keywords().localized.at("de") == "Shell;Befehl;");
}

TEST_CASE("JobXdg parses desktop actions and preserves action extensions",
          "[job_io][xdg][usage][actions]")
{
    XdgTestTree tree("actions");

    tree.desktopFile(
        tree.applicationsHome(),
        "actions.desktop",
        applicationSource(
            "Action Example",
            "/bin/true",
            "Actions=NewWindow;Private;\n"
            "\n"
            "[Desktop Action NewWindow]\n"
            "Name=New Window\n"
            "Name[de]=Neues Fenster\n"
            "Icon=window-new\n"
            "Exec=/usr/bin/example --new-window\n"
            "X-JOB-Action=one\n"
            "\n"
            "[Desktop Action Private]\n"
            "Name=Private Window\n"
            "Exec=/usr/bin/example --private\n"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "actions.desktop");

    REQUIRE(entry->actions() == std::vector<std::string>{"NewWindow", "Private"});
    REQUIRE(entry->desktopActions().size() == 2);

    const auto &first = entry->desktopActions()[0];
    REQUIRE(first.id == "NewWindow");
    REQUIRE(first.name.value == "New Window");
    REQUIRE(first.name.localized.at("de") == "Neues Fenster");
    REQUIRE(first.icon.value == "window-new");
    REQUIRE(first.exec == "/usr/bin/example --new-window");
    REQUIRE(first.extensionEntries.size() == 1);
    REQUIRE(first.extensionEntries.front().key == "X-JOB-Action");
    REQUIRE(first.extensionEntries.front().value == "one");

    const auto &second = entry->desktopActions()[1];
    REQUIRE(second.id == "Private");
    REQUIRE(second.name.value == "Private Window");
    REQUIRE(second.exec == "/usr/bin/example --private");
}

TEST_CASE("JobXdg preserves unknown desktop entry keys and groups",
          "[job_io][xdg][usage][extensions]")
{
    XdgTestTree tree("extensions");

    tree.desktopFile(
        tree.applicationsHome(),
        "extensions.desktop",
        applicationSource(
            "Extension Example",
            "/bin/true",
            "X-JOB-Feature=Enabled\n"
            "X-KDE-ServiceName=org.job.Test\n"
            "\n"
            "[X-JOB Metadata]\n"
            "Owner=JOB\n"
            "Version=1\n"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "extensions.desktop");

    REQUIRE(entry->extensionEntries().size() == 2);
    REQUIRE(entry->extensionEntries()[0].key == "X-JOB-Feature");
    REQUIRE(entry->extensionEntries()[0].value == "Enabled");
    REQUIRE(entry->extensionEntries()[1].key == "X-KDE-ServiceName");
    REQUIRE(entry->extensionEntries()[1].value == "org.job.Test");

    REQUIRE(entry->extensionGroups().size() == 1);
    REQUIRE(entry->extensionGroups().front().name == "X-JOB Metadata");
    REQUIRE(entry->extensionGroups().front().entries.size() == 2);
    REQUIRE(entry->extensionGroups().front().entries[0].key == "Owner");
    REQUIRE(entry->extensionGroups().front().entries[0].value == "JOB");
    REQUIRE(entry->extensionGroups().front().entries[1].key == "Version");
    REQUIRE(entry->extensionGroups().front().entries[1].value == "1");
}

TEST_CASE("JobXdg parses Link desktop entries",
          "[job_io][xdg][usage][link]")
{
    XdgTestTree tree("link");

    tree.desktopFile(
        tree.applicationsHome(),
        "website.desktop",
        linkSource("JOB Website", "https://example.invalid/job"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "website.desktop");

    REQUIRE(entry->type() == JobXdgEntryType::Link);
    REQUIRE(entry->name().value == "JOB Website");
    REQUIRE(entry->url() == "https://example.invalid/job");
    REQUIRE_FALSE(xdg.exec(*entry));
}

TEST_CASE("JobXdg derives desktop file IDs from application subdirectories",
          "[job_io][xdg][usage][desktop_file_id]")
{
    XdgTestTree tree("nested_id");

    tree.desktopFile(
        tree.applicationsHome(),
        "vendor/tools/example.desktop",
        applicationSource("Nested", "/bin/true"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    REQUIRE(xdg.contains("vendor-tools-example.desktop"));
    REQUIRE_FALSE(xdg.contains("vendor/tools/example.desktop"));

    const JobXdgEntry *entry = requireEntry(xdg, "vendor-tools-example.desktop");
    REQUIRE(entry->path() == tree.applicationsHome() / "vendor/tools/example.desktop");
}

TEST_CASE("JobXdg higher priority XDG_DATA_HOME entry wins duplicate desktop ID",
          "[job_io][xdg][usage][precedence]")
{
    XdgTestTree tree("precedence_home");

    tree.desktopFile(
        tree.applicationsHome(),
        "same.desktop",
        applicationSource("Home Winner", "/bin/true"));

    tree.desktopFile(
        tree.applicationsA(),
        "same.desktop",
        applicationSource("Data A", "/bin/true"));

    tree.desktopFile(
        tree.applicationsB(),
        "same.desktop",
        applicationSource("Data B", "/bin/true"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    REQUIRE(xdg.size() == 1);

    const JobXdgEntry *entry = requireEntry(xdg, "same.desktop");
    REQUIRE(entry->name().value == "Home Winner");
    REQUIRE(entry->path() == tree.applicationsHome() / "same.desktop");
}

TEST_CASE("JobXdg earlier XDG_DATA_DIRS entry wins duplicate desktop ID",
          "[job_io][xdg][usage][precedence]")
{
    XdgTestTree tree("precedence_dirs");

    tree.desktopFile(
        tree.applicationsA(),
        "same.desktop",
        applicationSource("Data A Winner", "/bin/true"));

    tree.desktopFile(
        tree.applicationsB(),
        "same.desktop",
        applicationSource("Data B", "/bin/true"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    REQUIRE(xdg.size() == 1);

    const JobXdgEntry *entry = requireEntry(xdg, "same.desktop");
    REQUIRE(entry->name().value == "Data A Winner");
    REQUIRE(entry->path() == tree.applicationsA() / "same.desktop");
}

TEST_CASE("JobXdg Hidden entry masks lower priority copies",
          "[job_io][xdg][usage][precedence][hidden]")
{
    XdgTestTree tree("hidden_mask");

    tree.desktopFile(
        tree.applicationsHome(),
        "masked.desktop",
        applicationSource(
            "Masked",
            "/bin/true",
            "Hidden=true\n"));

    tree.desktopFile(
        tree.applicationsA(),
        "masked.desktop",
        applicationSource("Lower Copy", "/bin/true"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    REQUIRE_FALSE(xdg.contains("masked.desktop"));
    REQUIRE(xdg.find("masked.desktop") == nullptr);
    REQUIRE(xdg.empty());
}

TEST_CASE("JobXdg resolves a desktop Exec into XdgExec",
          "[job_io][xdg][usage][exec]")
{
    XdgTestTree tree("exec_basic");

    tree.desktopFile(
        tree.applicationsHome(),
        "exec.desktop",
        applicationSource(
            "Exec Example",
            "/usr/bin/example --verbose value",
            "Path=/tmp/job-working\n"
            "Terminal=true\n"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    const auto resolved = xdg.exec("exec.desktop");

    REQUIRE(resolved);
    REQUIRE(resolved->program == "/usr/bin/example");
    REQUIRE(resolved->arguments == std::vector<std::string>{"--verbose", "value"});
    REQUIRE(resolved->workingDirectory == "/tmp/job-working");
    REQUIRE(resolved->terminal);
}

TEST_CASE("JobXdg expands supported Exec field codes",
          "[job_io][xdg][usage][exec][field_codes]")
{
    XdgTestTree tree("exec_fields");

    const auto &file = tree.desktopFile(tree.applicationsHome(),
                                        "fields.desktop",
                                        applicationSource(
                                            "Field Example",
                                            "/usr/bin/example --name=%c --desktop=%k %% %i %F %U",
                                            "Icon=job-icon\n"));

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());

    const auto resolved = xdg.exec("fields.desktop");

    REQUIRE(resolved);
    REQUIRE(resolved->program == "/usr/bin/example");
    REQUIRE(resolved->arguments ==
            std::vector<std::string>{
                "--name=Field Example",
                "--desktop=" + file.pathString(),
                "%",
                "--icon",
                "job-icon"
            });
}

TEST_CASE("JobXdg exec rejects field codes inside quoted arguments",
          "[job_io][xdg][edge][exec][field_codes][quoting]")
{
    XdgTestTree tree("quoted_field");

    tree.desktopFile(
        tree.applicationsHome(),
        "bad.desktop",
        applicationSource(
            "Quoted Field",
            "/usr/bin/example \"--name=%c\""));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE_FALSE(xdg.exec("bad.desktop"));
}


TEST_CASE("JobXdg reloadParallel produces the same catalog as serial reload",
          "[job_io][xdg][usage][parallel]")
{
    XdgTestTree tree("parallel");

    for (std::size_t i = 0; i < 256; ++i) {
        tree.desktopFile(
            tree.applicationsHome(),
            "entry-" + std::to_string(i) + ".desktop",
            applicationSource(
                "Entry " + std::to_string(i),
                "/bin/true",
                "Categories=Utility;JOB;\n"));
    }

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdg::EntryList serial = xdg.entries();




    auto scheduler = std::make_shared<job::threads::FifoScheduler>();
    auto pool = job::threads::ThreadPool::create(scheduler, 4);

    REQUIRE(pool);
    REQUIRE(xdg.reloadParallel(*pool));

    const JobXdg::EntryList parallel = xdg.entries();

    REQUIRE(parallel.size() == serial.size());

    for (std::size_t i = 0; i < serial.size(); ++i) {
        REQUIRE(parallel[i].desktopFileId() == serial[i].desktopFileId());
        REQUIRE(parallel[i].path() == serial[i].path());
        REQUIRE(parallel[i].type() == serial[i].type());
        REQUIRE(parallel[i].name().value == serial[i].name().value);
        REQUIRE(parallel[i].exec() == serial[i].exec());
        REQUIRE(parallel[i].categories() == serial[i].categories());
    }

    pool->shutdown();
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobXdg ignores malformed and incomplete desktop files",
          "[job_io][xdg][edge][invalid]")
{
    XdgTestTree tree("invalid");

    tree.desktopFile(
        tree.applicationsHome(),
        "valid.desktop",
        applicationSource("Valid", "/bin/true"));

    tree.desktopFile(
        tree.applicationsHome(),
        "missing-name.desktop",
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Exec=/bin/true\n");

    tree.desktopFile(
        tree.applicationsHome(),
        "missing-type.desktop",
        "[Desktop Entry]\n"
        "Name=Missing Type\n"
        "Exec=/bin/true\n");

    tree.desktopFile(
        tree.applicationsHome(),
        "unknown-type.desktop",
        "[Desktop Entry]\n"
        "Type=Potato\n"
        "Name=Unknown\n");

    tree.desktopFile(
        tree.applicationsHome(),
        "application-no-exec.desktop",
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=No Exec\n");

    tree.desktopFile(
        tree.applicationsHome(),
        "link-no-url.desktop",
        "[Desktop Entry]\n"
        "Type=Link\n"
        "Name=No URL\n");

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE(xdg.size() == 1);
    REQUIRE(xdg.contains("valid.desktop"));
}

TEST_CASE("JobXdg accepts DBusActivatable application without Exec",
          "[job_io][xdg][edge][dbus]")
{
    XdgTestTree tree("dbus");

    tree.desktopFile(
        tree.applicationsHome(),
        "dbus.desktop",
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=DBus Application\n"
        "DBusActivatable=true\n");

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "dbus.desktop");

    REQUIRE(entry->dbusActivatable());
    REQUIRE(entry->exec().empty());
    REQUIRE_FALSE(xdg.exec(*entry));
}

TEST_CASE("JobXdg rejects duplicate groups and duplicate keys",
          "[job_io][xdg][edge][duplicates]")
{
    XdgTestTree tree("duplicates");

    tree.desktopFile(
        tree.applicationsHome(),
        "duplicate-key.desktop",
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=First\n"
        "Name=Second\n"
        "Exec=/bin/true\n");

    tree.desktopFile(
        tree.applicationsHome(),
        "duplicate-group.desktop",
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=First\n"
        "Exec=/bin/true\n"
        "[Desktop Entry]\n"
        "Name=Second\n");

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE(xdg.empty());
}

TEST_CASE("JobXdg rejects invalid boolean values",
          "[job_io][xdg][edge][boolean]")
{
    XdgTestTree tree("invalid_boolean");

    tree.desktopFile(
        tree.applicationsHome(),
        "invalid.desktop",
        applicationSource(
            "Invalid Boolean",
            "/bin/true",
            "Terminal=yes\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE(xdg.empty());
}

TEST_CASE("JobXdg accepts CRLF source without trailing newline",
          "[job_io][xdg][edge][line_endings]")
{
    XdgTestTree tree("crlf");

    tree.desktopFile(
        tree.applicationsHome(),
        "crlf.desktop",
        "[Desktop Entry]\r\n"
        "# comment\r\n"
        "Type=Application\r\n"
        "Name=CRLF Entry\r\n"
        "Exec=/bin/true");

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "crlf.desktop");

    REQUIRE(entry->name().value == "CRLF Entry");
    REQUIRE(entry->exec() == "/bin/true");
}

TEST_CASE("JobXdg unescapes desktop entry string values",
          "[job_io][xdg][edge][escape]")
{
    XdgTestTree tree("escape");

    tree.desktopFile(
        tree.applicationsHome(),
        "escape.desktop",
        applicationSource(
            "Escaped\\sName",
            "/bin/true",
            "Comment=line\\nnext\\tcolumn\\\\tail\\;semi\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "escape.desktop");

    REQUIRE(entry->name().value == "Escaped Name");
    REQUIRE(entry->comment().value == "line\nnext\tcolumn\\tail;semi");
}

TEST_CASE("JobXdg parses escaped semicolons in list values",
          "[job_io][xdg][edge][list][escape]")
{
    XdgTestTree tree("list_escape");

    tree.desktopFile(
        tree.applicationsHome(),
        "list.desktop",
        applicationSource(
            "List",
            "/bin/true",
            "Categories=One;Two\\;Three;Four;\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "list.desktop");

    REQUIRE(entry->categories() ==
            std::vector<std::string>{"One", "Two;Three", "Four"});
}

TEST_CASE("JobXdg drops desktop actions not listed by Actions",
          "[job_io][xdg][edge][actions]")
{
    XdgTestTree tree("unlisted_action");

    tree.desktopFile(
        tree.applicationsHome(),
        "actions.desktop",
        applicationSource(
            "Actions",
            "/bin/true",
            "Actions=Listed;\n"
            "\n"
            "[Desktop Action Listed]\n"
            "Name=Listed Action\n"
            "Exec=/bin/true\n"
            "\n"
            "[Desktop Action Unlisted]\n"
            "Name=Unlisted Action\n"
            "Exec=/bin/true\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "actions.desktop");

    REQUIRE(entry->desktopActions().size() == 1);
    REQUIRE(entry->desktopActions().front().id == "Listed");
}

TEST_CASE("JobXdg drops listed desktop action without a base Name",
          "[job_io][xdg][edge][actions]")
{
    XdgTestTree tree("action_no_name");

    tree.desktopFile(
        tree.applicationsHome(),
        "actions.desktop",
        applicationSource(
            "Actions",
            "/bin/true",
            "Actions=NoName;\n"
            "\n"
            "[Desktop Action NoName]\n"
            "Name[de]=Ohne Name\n"
            "Exec=/bin/true\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());

    const JobXdgEntry *entry = requireEntry(xdg, "actions.desktop");

    REQUIRE(entry->actions() == std::vector<std::string>{"NoName"});
    REQUIRE(entry->desktopActions().empty());
}

TEST_CASE("JobXdg exec returns nullopt for missing desktop ID",
          "[job_io][xdg][edge][exec]")
{
    XdgTestTree tree("missing_exec");

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE_FALSE(xdg.exec("does-not-exist.desktop"));
}

TEST_CASE("JobXdg exec rejects malformed quoting",
          "[job_io][xdg][edge][exec][quoting]")
{
    XdgTestTree tree("bad_quote");

    tree.desktopFile(
        tree.applicationsHome(),
        "bad.desktop",
        applicationSource(
            "Bad Quote",
            "/usr/bin/example \"unterminated"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE_FALSE(xdg.exec("bad.desktop"));
}

TEST_CASE("JobXdg exec rejects unsupported field codes",
          "[job_io][xdg][edge][exec][field_codes]")
{
    XdgTestTree tree("bad_field");

    tree.desktopFile(
        tree.applicationsHome(),
        "bad.desktop",
        applicationSource(
            "Bad Field",
            "/usr/bin/example %Z"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE_FALSE(xdg.exec("bad.desktop"));
}

TEST_CASE("JobXdg exec honors absolute TryExec availability",
          "[job_io][xdg][edge][exec][try_exec]")
{
    XdgTestTree tree("try_exec_absolute");

    tree.desktopFile(
        tree.applicationsHome(),
        "available.desktop",
        applicationSource(
            "Available",
            "/bin/true",
            "TryExec=/bin/true\n"));

    tree.desktopFile(
        tree.applicationsHome(),
        "missing.desktop",
        applicationSource(
            "Missing",
            "/bin/true",
            "TryExec=/definitely/not/a/real/job-program\n"));

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE(xdg.exec("available.desktop"));
    REQUIRE_FALSE(xdg.exec("missing.desktop"));
}

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobXdg serial reload benchmark",
          "[job_io][xdg][benchmark][reload][serial]")
{
    XdgTestTree tree("benchmark_serial");

    for (std::size_t i = 0; i < 1000; ++i) {
        tree.desktopFile(
            tree.applicationsHome(),
            "entry-" + std::to_string(i) + ".desktop",
            applicationSource(
                "Entry " + std::to_string(i),
                "/bin/true",
                "Comment=Generated benchmark desktop entry\n"
                "Icon=job-example\n"
                "Categories=Utility;JOB;\n"
                "MimeType=text/plain;application/json;\n"));
    }

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reload());
    REQUIRE(xdg.size() == 1000);

    BENCHMARK("reload 1000 desktop entries serially")
    {
        return xdg.reload();
    };
}

TEST_CASE("JobXdg parallel reload benchmark",
          "[job_io][xdg][benchmark][reload][parallel]")
{
    XdgTestTree tree("benchmark_parallel");

    for (std::size_t i = 0; i < 1000; ++i) {
        tree.desktopFile(
            tree.applicationsHome(),
            "entry-" + std::to_string(i) + ".desktop",
            applicationSource(
                "Entry " + std::to_string(i),
                "/bin/true",
                "Comment=Generated benchmark desktop entry\n"
                "Icon=job-example\n"
                "Categories=Utility;JOB;\n"
                "MimeType=text/plain;application/json;\n"));
    }


    // job::threads::
    auto scheduler = std::make_shared<job::threads::FifoScheduler>();
    auto pool = job::threads::ThreadPool::create(scheduler, 4);

    REQUIRE(pool);

    JobXdg &xdg = JobXdg::instance();
    REQUIRE(xdg.reloadParallel(*pool));
    REQUIRE(xdg.size() == 1000);

    BENCHMARK("reload 1000 desktop entries with buffered parallel parse") {
        return xdg.reloadParallel(*pool);
    };

    pool->shutdown();
}


TEST_CASE("JobXdg dual parser reload benchmark",
          "[job_io][xdg][benchmark][reload][dual]")
{
    XdgTestTree tree("benchmark_dual");

    for (std::size_t i = 0; i < 1000; ++i) {
        tree.desktopFile(
            tree.applicationsHome(),
            "entry-" + std::to_string(i) + ".desktop",
            applicationSource(
                "Entry " + std::to_string(i),
                "/bin/true",
                "Comment=Generated benchmark desktop entry\n"
                "Icon=job-example\n"
                "Categories=Utility;JOB;\n"
                "MimeType=text/plain;application/json;\n"));
    }

    JobXdg &xdg = JobXdg::instance();

    REQUIRE(xdg.reload());
    REQUIRE(xdg.size() == 1000);
    REQUIRE(xdg.desktopEntries().size() == 1000);

    BENCHMARK("reload 1000 desktop entries through both parsers")
    {
        return xdg.reload();
    };
}

TEST_CASE("JobXdgDesktopEntry reflection parser benchmark",
          "[job_io][xdg][desktop-entry][benchmark][reflection]")
{
    std::vector<std::string> sources;
    sources.reserve(1000);

    for (std::size_t i = 0; i < 1000; ++i) {
        sources.push_back(
            applicationSource(
                "Entry " + std::to_string(i),
                "/bin/true",
                "Comment=Generated benchmark desktop entry\n"
                "Icon=job-example\n"
                "Categories=Utility;JOB;\n"
                "MimeType=text/plain;application/json;\n"));
    }

    for (const auto &source : sources) {
        JobXdgDesktopEntry entry;
        REQUIRE(entry.fromIni(source));
    }

    BENCHMARK("parse 1000 desktop entries with JobIni reflection")
    {
        std::size_t parsed = 0;

        for (const auto &source : sources) {
            JobXdgDesktopEntry entry;

            if (entry.fromIni(source))
                ++parsed;
        }

        return parsed;
    };
}







#endif
