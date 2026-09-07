#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <pty_dot.h>

using namespace job::io;

TEST_CASE("PtyDot stores source metadata", "[job_io][pty][dot]")
{
    PtyDot dot("/etc/profile",
               PtyDot::Role::SystemProfile,
               PtyDot::Format::Shell);

    REQUIRE(dot.enabled());
    REQUIRE(dot.role() == PtyDot::Role::SystemProfile);
    REQUIRE(dot.format() == PtyDot::Format::Shell);
    REQUIRE(dot.path() == "/etc/profile");
    REQUIRE_FALSE(dot.hasHash());
    REQUIRE(dot.hash().empty());
}

TEST_CASE("PtyDot can be enabled and disabled", "[job_io][pty][dot]")
{
    PtyDot dot("/etc/environment",
               PtyDot::Role::SystemEnvironment,
               PtyDot::Format::Environment);

    REQUIRE(dot.enabled());

    dot.setEnabled(false);
    REQUIRE_FALSE(dot.enabled());

    dot.setEnabled(true);
    REQUIRE(dot.enabled());
}

TEST_CASE("PtyDot role can be changed", "[job_io][pty][dot]")
{
    PtyDot dot("/tmp/job.env",
               PtyDot::Role::Custom,
               PtyDot::Format::DotEnv);

    REQUIRE(dot.role() == PtyDot::Role::Custom);

    dot.setRole(PtyDot::Role::Project);

    REQUIRE(dot.role() == PtyDot::Role::Project);
}

TEST_CASE("PtyDot format can be changed", "[job_io][pty][dot]")
{
    PtyDot dot("/tmp/job.env",
               PtyDot::Role::Custom,
               PtyDot::Format::Custom);

    REQUIRE(dot.format() == PtyDot::Format::Custom);

    dot.setFormat(PtyDot::Format::DotEnv);

    REQUIRE(dot.format() == PtyDot::Format::DotEnv);
}

TEST_CASE("PtyDot stores and clears a hash", "[job_io][pty][dot][hash]")
{
    PtyDot dot("/etc/profile",
               PtyDot::Role::SystemProfile,
               PtyDot::Format::Shell);

    const std::vector<unsigned char> hash{0x01, 0x23, 0x45, 0x67};

    REQUIRE_FALSE(dot.hasHash());

    dot.setHash(hash);

    REQUIRE(dot.hasHash());
    REQUIRE(dot.hash() == hash);

    dot.clearHash();

    REQUIRE_FALSE(dot.hasHash());
    REQUIRE(dot.hash().empty());
}

TEST_CASE("PtyDot changing path invalidates its hash", "[job_io][pty][dot][hash]")
{
    PtyDot dot("/etc/profile",
               PtyDot::Role::SystemProfile,
               PtyDot::Format::Shell);

    dot.setHash({0xAA, 0xBB, 0xCC});

    REQUIRE(dot.hasHash());

    dot.setPath("/etc/bash.bashrc");

    REQUIRE(dot.path() == "/etc/bash.bashrc");
    REQUIRE_FALSE(dot.hasHash());
    REQUIRE(dot.hash().empty());
}

TEST_CASE("PtyDot setting the same path preserves its hash", "[job_io][pty][dot][hash]")
{
    PtyDot dot("/etc/profile",
               PtyDot::Role::SystemProfile,
               PtyDot::Format::Shell);

    const std::vector<unsigned char> hash{0x10, 0x20, 0x30};

    dot.setHash(hash);
    dot.setPath("/etc/profile");

    REQUIRE(dot.path() == "/etc/profile");
    REQUIRE(dot.hasHash());
    REQUIRE(dot.hash() == hash);
}

TEST_CASE("PtyDot metadata changes preserve its hash", "[job_io][pty][dot][hash]")
{
    PtyDot dot("/etc/profile",
               PtyDot::Role::SystemProfile,
               PtyDot::Format::Shell);

    const std::vector<unsigned char> hash{0xDE, 0xAD, 0xBE, 0xEF};

    dot.setHash(hash);

    dot.setEnabled(false);
    REQUIRE(dot.hash() == hash);

    dot.setRole(PtyDot::Role::Custom);
    REQUIRE(dot.hash() == hash);

    dot.setFormat(PtyDot::Format::Custom);
    REQUIRE(dot.hash() == hash);

    REQUIRE(dot.hasHash());
}

TEST_CASE("PtyDot supports all source roles", "[job_io][pty][dot][role]")
{
    PtyDot dot("/tmp/source",
               PtyDot::Role::Custom,
               PtyDot::Format::Custom);

    const PtyDot::Role roles[] = {
        PtyDot::Role::SystemEnvironment,
        PtyDot::Role::SystemProfile,
        PtyDot::Role::SystemRc,
        PtyDot::Role::UserProfile,
        PtyDot::Role::UserRc,
        PtyDot::Role::ShellEnv,
        PtyDot::Role::Project,
        PtyDot::Role::Custom
    };

    for (const auto role : roles) {
        dot.setRole(role);
        REQUIRE(dot.role() == role);
    }
}

TEST_CASE("PtyDot supports all source formats", "[job_io][pty][dot][format]")
{
    PtyDot dot("/tmp/source",
               PtyDot::Role::Custom,
               PtyDot::Format::Custom);

    const PtyDot::Format formats[] = {
        PtyDot::Format::Environment,
        PtyDot::Format::Shell,
        PtyDot::Format::DotEnv,
        PtyDot::Format::DirEnv,
        PtyDot::Format::Custom
    };

    for (const auto format : formats) {
        dot.setFormat(format);
        REQUIRE(dot.format() == format);
    }
}

TEST_CASE("PtyDot copied object owns independent metadata", "[job_io][pty][dot][copy]")
{
    PtyDot original("/etc/profile",
                    PtyDot::Role::SystemProfile,
                    PtyDot::Format::Shell);

    original.setHash({0x01, 0x02, 0x03});

    PtyDot copy = original;

    REQUIRE(copy.path() == "/etc/profile");
    REQUIRE(copy.role() == PtyDot::Role::SystemProfile);
    REQUIRE(copy.format() == PtyDot::Format::Shell);
    REQUIRE(copy.hash() == original.hash());

    copy.setPath("/tmp/project.env");
    copy.setRole(PtyDot::Role::Project);
    copy.setFormat(PtyDot::Format::DotEnv);
    copy.setHash({0xAA});

    REQUIRE(original.path() == "/etc/profile");
    REQUIRE(original.role() == PtyDot::Role::SystemProfile);
    REQUIRE(original.format() == PtyDot::Format::Shell);
    REQUIRE(original.hash() == std::vector<unsigned char>{0x01, 0x02, 0x03});

    REQUIRE(copy.path() == "/tmp/project.env");
    REQUIRE(copy.role() == PtyDot::Role::Project);
    REQUIRE(copy.format() == PtyDot::Format::DotEnv);
    REQUIRE(copy.hash() == std::vector<unsigned char>{0xAA});
}

TEST_CASE("PtyDot createShared constructs a shared instance", "[job_io][pty][dot]")
{
    auto dot = PtyDot::createShared("/etc/profile",
                                    PtyDot::Role::SystemProfile,
                                    PtyDot::Format::Shell);

    REQUIRE(dot);
    REQUIRE(dot->path() == "/etc/profile");
    REQUIRE(dot->role() == PtyDot::Role::SystemProfile);
    REQUIRE(dot->format() == PtyDot::Format::Shell);
    REQUIRE(dot->enabled());
}

TEST_CASE("PtyDot createUniq constructs a unique instance", "[job_io][pty][dot]")
{
    auto dot = PtyDot::createUniq("/tmp/project.env",
                                  PtyDot::Role::Project,
                                  PtyDot::Format::DotEnv,
                                  false);

    REQUIRE(dot);
    REQUIRE(dot->path() == "/tmp/project.env");
    REQUIRE(dot->role() == PtyDot::Role::Project);
    REQUIRE(dot->format() == PtyDot::Format::DotEnv);
    REQUIRE_FALSE(dot->enabled());
}