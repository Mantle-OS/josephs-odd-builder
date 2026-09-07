#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <job_xdg_desktop_entry.h>

namespace job::io::test {

TEST_CASE("JobXdgDesktopEntry follows JOB object conventions",
          "[job_io][xdg][desktop-entry]")
{
    static_assert(std::derived_from<JobXdgDesktopEntry, JobIni>);
    static_assert(std::copy_constructible<JobXdgDesktopEntry>);
    static_assert(std::is_copy_assignable_v<JobXdgDesktopEntry>);
    static_assert(std::move_constructible<JobXdgDesktopEntry>);
    static_assert(std::is_move_assignable_v<JobXdgDesktopEntry>);

    JobXdgDesktopEntry entry;

    REQUIRE(entry.Type == JobXdgEntryType::Unknown);
    REQUIRE(entry.Version.empty());

    REQUIRE(entry.Name.value.empty());
    REQUIRE(entry.Name.localized.empty());
    REQUIRE(entry.GenericName.value.empty());
    REQUIRE(entry.Comment.value.empty());
    REQUIRE(entry.Icon.value.empty());
    REQUIRE(entry.Keywords.value.empty());

    REQUIRE_FALSE(entry.NoDisplay);
    REQUIRE_FALSE(entry.Hidden);
    REQUIRE(entry.OnlyShowIn.empty());
    REQUIRE(entry.NotShowIn.empty());

    REQUIRE_FALSE(entry.DBusActivatable);
    REQUIRE(entry.TryExec.empty());
    REQUIRE(entry.Exec.empty());
    REQUIRE(entry.Path.empty());
    REQUIRE_FALSE(entry.Terminal);

    REQUIRE(entry.Actions.empty());
    REQUIRE(entry.MimeType.empty());
    REQUIRE(entry.Categories.empty());
    REQUIRE(entry.Implements.empty());

    REQUIRE_FALSE(entry.StartupNotify.has_value());
    REQUIRE(entry.StartupWMClass.empty());

    REQUIRE_FALSE(entry.PrefersNonDefaultGPU);
    REQUIRE_FALSE(entry.SingleMainWindow);

    REQUIRE(entry.URL.empty());

    REQUIRE(entry.desktopActions.empty());
    REQUIRE(entry.extensionEntries.empty());
    REQUIRE(entry.extensionGroups.empty());

    REQUIRE(entry.path.empty());
    REQUIRE(entry.desktopFileId.empty());

    auto shared = JobXdgDesktopEntry::createShared();
    auto unique = JobXdgDesktopEntry::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);
}

TEST_CASE("JobXdgDesktopEntry parses basic Desktop Entry fields",
          "[job_io][xdg][desktop-entry][ini]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Version=1.5
Name=Spectacle
GenericName=Screenshot Capture Utility
Comment=Capture the screen
Icon=spectacle
NoDisplay=false
Hidden=false
DBusActivatable=true
TryExec=/usr/bin/spectacle
Exec=/usr/bin/spectacle
Path=/tmp
Terminal=false
StartupNotify=false
StartupWMClass=spectacle
PrefersNonDefaultGPU=true
SingleMainWindow=true
URL=https://example.com
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));

    REQUIRE(entry.Version == "1.5");

    REQUIRE(entry.Name.value == "Spectacle");
    REQUIRE(entry.GenericName.value == "Screenshot Capture Utility");
    REQUIRE(entry.Comment.value == "Capture the screen");
    REQUIRE(entry.Icon.value == "spectacle");

    REQUIRE_FALSE(entry.NoDisplay);
    REQUIRE_FALSE(entry.Hidden);
    REQUIRE(entry.DBusActivatable);

    REQUIRE(entry.TryExec == "/usr/bin/spectacle");
    REQUIRE(entry.Exec == "/usr/bin/spectacle");
    REQUIRE(entry.Path == "/tmp");

    REQUIRE_FALSE(entry.Terminal);

    REQUIRE(entry.StartupNotify.has_value());
    REQUIRE_FALSE(*entry.StartupNotify);

    REQUIRE(entry.StartupWMClass == "spectacle");

    REQUIRE(entry.PrefersNonDefaultGPU);
    REQUIRE(entry.SingleMainWindow);

    REQUIRE(entry.URL == "https://example.com");
}

TEST_CASE("JobXdgDesktopEntry parses localized strings",
          "[job_io][xdg][desktop-entry][ini][localized]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Name=Spectacle
Name[de]=Spectacle Deutsch
Name[fr]=Spectacle Français
GenericName=Screenshot Utility
GenericName[de]=Bildschirmfoto
Comment=Capture the screen
Comment[fr]=Capturer l'écran
Icon=spectacle
Keywords=screenshot
Keywords[de]=bildschirmfoto
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));

    REQUIRE(entry.Name.value == "Spectacle");
    REQUIRE(entry.Name.localized.size() == 2);
    REQUIRE(entry.Name.localized.at("de") == "Spectacle Deutsch");
    REQUIRE(entry.Name.localized.at("fr") == "Spectacle Français");

    REQUIRE(entry.GenericName.value == "Screenshot Utility");
    REQUIRE(entry.GenericName.localized.size() == 1);
    REQUIRE(entry.GenericName.localized.at("de") == "Bildschirmfoto");

    REQUIRE(entry.Comment.value == "Capture the screen");
    REQUIRE(entry.Comment.localized.size() == 1);
    REQUIRE(entry.Comment.localized.at("fr") == "Capturer l'écran");

    REQUIRE(entry.Icon.value == "spectacle");

    REQUIRE(entry.Keywords.value == "screenshot");
    REQUIRE(entry.Keywords.localized.size() == 1);
    REQUIRE(entry.Keywords.localized.at("de") == "bildschirmfoto");
}

TEST_CASE("JobXdgDesktopEntry parses semicolon lists",
          "[job_io][xdg][desktop-entry][ini][list]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
OnlyShowIn=KDE;GNOME;
NotShowIn=XFCE;
Actions=FullScreen;ActiveWindow;Region;
MimeType=image/png;image/jpeg;
Categories=Qt;KDE;Utility;
Implements=org.example.One;org.example.Two;
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));

    REQUIRE(entry.OnlyShowIn ==
            std::vector<std::string>{"KDE", "GNOME"});

    REQUIRE(entry.NotShowIn ==
            std::vector<std::string>{"XFCE"});

    REQUIRE(entry.Actions ==
            std::vector<std::string>{
                "FullScreen",
                "ActiveWindow",
                "Region"
            });

    REQUIRE(entry.MimeType ==
            std::vector<std::string>{
                "image/png",
                "image/jpeg"
            });

    REQUIRE(entry.Categories ==
            std::vector<std::string>{
                "Qt",
                "KDE",
                "Utility"
            });

    REQUIRE(entry.Implements ==
            std::vector<std::string>{
                "org.example.One",
                "org.example.Two"
            });
}

TEST_CASE("JobXdgDesktopEntry parses escaped semicolon list values",
          "[job_io][xdg][desktop-entry][ini][list][escape]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Categories=One\;Two;Three;
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));

    REQUIRE(entry.Categories ==
            std::vector<std::string>{
                "One;Two",
                "Three"
            });
}

TEST_CASE("JobXdgDesktopEntry preserves absent optional values",
          "[job_io][xdg][desktop-entry][ini][optional]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Name=Example
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));
    REQUIRE_FALSE(entry.StartupNotify.has_value());
}

TEST_CASE("JobXdgDesktopEntry parses optional false distinctly from absence",
          "[job_io][xdg][desktop-entry][ini][optional]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
StartupNotify=false
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));

    REQUIRE(entry.StartupNotify.has_value());
    REQUIRE_FALSE(*entry.StartupNotify);
}

TEST_CASE("JobXdgDesktopEntry ignores entries outside its INI group",
          "[job_io][xdg][desktop-entry][ini][group]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Name=Root Name
Exec=/usr/bin/root

[Desktop Action Something]
Name=Action Name
Exec=/usr/bin/action

[Some Extension Group]
Name=Extension Name
Exec=/usr/bin/extension
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE(entry.fromIni(source));

    REQUIRE(entry.Name.value == "Root Name");
    REQUIRE(entry.Exec == "/usr/bin/root");
}

TEST_CASE("JobXdgDesktopEntry ignores NoSerialize metadata during INI output",
          "[job_io][xdg][desktop-entry][ini][serialization]")
{
    JobXdgDesktopEntry entry;

    entry.path = "/tmp/example.desktop";
    entry.desktopFileId = "example.desktop";

    entry.Name.value = "Example";
    entry.Exec = "/usr/bin/example";
    entry.Terminal = false;

    const std::string ini = entry.toIni();

    REQUIRE(ini.find("path=") == std::string::npos);
    REQUIRE(ini.find("desktopFileId=") == std::string::npos);

    REQUIRE(ini.find("Name=Example") != std::string::npos);
    REQUIRE(ini.find("Exec=/usr/bin/example") != std::string::npos);
}

TEST_CASE("JobXdgDesktopEntry round trips supported INI values",
          "[job_io][xdg][desktop-entry][ini][roundtrip]")
{
    JobXdgDesktopEntry source;
    source.Type = JobXdgEntryType::Application;
    source.Version = "1.5";

    source.Name.value = "Example";
    source.Name.localized["de"] = "Beispiel";

    source.GenericName.value = "Example Application";
    source.Comment.value = "Example Comment";
    source.Icon.value = "example";

    source.NoDisplay = false;
    source.Hidden = false;

    source.OnlyShowIn = {"KDE", "GNOME"};
    source.NotShowIn = {"XFCE"};

    source.DBusActivatable = true;
    source.TryExec = "/usr/bin/example";
    source.Exec = "/usr/bin/example";
    source.Path = "/tmp";
    source.Terminal = false;

    source.Actions = {"One", "Two"};
    source.MimeType = {"text/plain"};
    source.Categories = {"Utility", "Development"};
    source.Implements = {"org.example.Interface"};

    source.StartupNotify = true;
    source.StartupWMClass = "example";

    source.PrefersNonDefaultGPU = true;
    source.SingleMainWindow = true;

    source.URL = "https://example.com";

    const std::string ini = source.toIni();

    JobXdgDesktopEntry destination;

    REQUIRE(destination.fromIni(ini));

    REQUIRE(destination.Type == source.Type);
    REQUIRE(destination.Version == source.Version);

    REQUIRE(destination.Name.value == source.Name.value);
    REQUIRE(destination.Name.localized == source.Name.localized);

    REQUIRE(destination.GenericName.value == source.GenericName.value);
    REQUIRE(destination.Comment.value == source.Comment.value);
    REQUIRE(destination.Icon.value == source.Icon.value);

    REQUIRE(destination.NoDisplay == source.NoDisplay);
    REQUIRE(destination.Hidden == source.Hidden);

    REQUIRE(destination.OnlyShowIn == source.OnlyShowIn);
    REQUIRE(destination.NotShowIn == source.NotShowIn);

    REQUIRE(destination.DBusActivatable == source.DBusActivatable);
    REQUIRE(destination.TryExec == source.TryExec);
    REQUIRE(destination.Exec == source.Exec);
    REQUIRE(destination.Path == source.Path);
    REQUIRE(destination.Terminal == source.Terminal);

    REQUIRE(destination.Actions == source.Actions);
    REQUIRE(destination.MimeType == source.MimeType);
    REQUIRE(destination.Categories == source.Categories);
    REQUIRE(destination.Implements == source.Implements);

    REQUIRE(destination.StartupNotify == source.StartupNotify);
    REQUIRE(destination.StartupWMClass == source.StartupWMClass);

    REQUIRE(destination.PrefersNonDefaultGPU ==
            source.PrefersNonDefaultGPU);

    REQUIRE(destination.SingleMainWindow ==
            source.SingleMainWindow);

    REQUIRE(destination.URL == source.URL);
}

TEST_CASE("JobXdgDesktopEntry rejects malformed localized keys",
          "[job_io][xdg][desktop-entry][ini][error]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Name[]=Broken
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE_FALSE(entry.fromIni(source));
    REQUIRE_FALSE(entry.lastErrorString.empty());
}

TEST_CASE("JobXdgDesktopEntry rejects malformed groups",
          "[job_io][xdg][desktop-entry][ini][error]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry
Name=Broken
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE_FALSE(entry.fromIni(source));
    REQUIRE_FALSE(entry.lastErrorString.empty());
}

TEST_CASE("JobXdgDesktopEntry rejects invalid boolean values",
          "[job_io][xdg][desktop-entry][ini][error]")
{
    constexpr std::string_view source = R"ini(
[Desktop Entry]
Terminal=yes
)ini";

    JobXdgDesktopEntry entry;

    REQUIRE_FALSE(entry.fromIni(source));
    REQUIRE_FALSE(entry.lastErrorString.empty());
}

} // namespace job::io::test