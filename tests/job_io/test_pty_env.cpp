#include <catch2/catch_test_macros.hpp>

#include <pty_env.h>

using namespace job::io;

TEST_CASE("PtyEnv stores an environment variable", "[job_io][pty][env]")
{
    PtyEnv env("JOB_TEST", "alpha");

    REQUIRE(env.name() == "JOB_TEST");
    REQUIRE(env.value() == "alpha");
    REQUIRE_FALSE(env.enabled());
    REQUIRE(env.envType() == PtyEnv::EnvType::Custom);
}

TEST_CASE("PtyEnv stores POSIX environment type", "[job_io][pty][env]")
{
    PtyEnv env("PATH", "/usr/bin:/bin", true, PtyEnv::EnvType::Posix);

    REQUIRE(env.name() == "PATH");
    REQUIRE(env.value() == "/usr/bin:/bin");
    REQUIRE(env.enabled());
    REQUIRE(env.envType() == PtyEnv::EnvType::Posix);
}

TEST_CASE("PtyEnv stores XDG environment type", "[job_io][pty][env]")
{
    PtyEnv env("XDG_CONFIG_HOME", "/tmp/config", true, PtyEnv::EnvType::Xdg);

    REQUIRE(env.name() == "XDG_CONFIG_HOME");
    REQUIRE(env.value() == "/tmp/config");
    REQUIRE(env.enabled());
    REQUIRE(env.envType() == PtyEnv::EnvType::Xdg);
}

TEST_CASE("PtyEnv can be enabled and disabled", "[job_io][pty][env]")
{
    PtyEnv env("JOB_TEST", "value");

    REQUIRE_FALSE(env.enabled());

    env.setEnabled(true);
    REQUIRE(env.enabled());

    env.setEnabled(false);
    REQUIRE_FALSE(env.enabled());
}

TEST_CASE("PtyEnv environment type can be changed", "[job_io][pty][env]")
{
    PtyEnv env("JOB_TEST", "value");

    REQUIRE(env.envType() == PtyEnv::EnvType::Custom);

    env.setEnvType(PtyEnv::EnvType::Posix);
    REQUIRE(env.envType() == PtyEnv::EnvType::Posix);

    env.setEnvType(PtyEnv::EnvType::Xdg);
    REQUIRE(env.envType() == PtyEnv::EnvType::Xdg);

    env.setEnvType(PtyEnv::EnvType::Custom);
    REQUIRE(env.envType() == PtyEnv::EnvType::Custom);
}

TEST_CASE("PtyEnv name can be changed", "[job_io][pty][env]")
{
    PtyEnv env("OLD_NAME", "value");

    REQUIRE(env.name() == "OLD_NAME");

    env.setName("NEW_NAME");

    REQUIRE(env.name() == "NEW_NAME");
    REQUIRE(env.value() == "value");
}

TEST_CASE("PtyEnv value can be changed", "[job_io][pty][env]")
{
    PtyEnv env("JOB_TEST", "alpha");

    REQUIRE(env.value() == "alpha");

    env.setValue("beta");

    REQUIRE(env.value() == "beta");
}

TEST_CASE("PtyEnv preserves an empty environment value", "[job_io][pty][env]")
{
    PtyEnv env("JOB_EMPTY", "");

    REQUIRE(env.name() == "JOB_EMPTY");
    REQUIRE(env.value().empty());
    REQUIRE_FALSE(env.enabled());
    REQUIRE(env.envType() == PtyEnv::EnvType::Custom);
}

TEST_CASE("PtyEnv splits colon separated values", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", "/usr/local/bin:/usr/bin:/bin");

    const auto values = env.values();

    REQUIRE(values.size() == 3);
    REQUIRE(values[0] == "/usr/local/bin");
    REQUIRE(values[1] == "/usr/bin");
    REQUIRE(values[2] == "/bin");
}

TEST_CASE("PtyEnv preserves empty components in colon separated values", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", "/bin::/usr/bin:");

    const auto values = env.values();

    REQUIRE(values.size() == 4);
    REQUIRE(values[0] == "/bin");
    REQUIRE(values[1].empty());
    REQUIRE(values[2] == "/usr/bin");
    REQUIRE(values[3].empty());
}

TEST_CASE("PtyEnv preserves a leading empty component", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", ":/bin:/usr/bin");

    const auto values = env.values();

    REQUIRE(values.size() == 3);
    REQUIRE(values[0].empty());
    REQUIRE(values[1] == "/bin");
    REQUIRE(values[2] == "/usr/bin");
}

TEST_CASE("PtyEnv preserves consecutive empty components", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", "/bin:::/usr/bin");

    const auto values = env.values();

    REQUIRE(values.size() == 4);
    REQUIRE(values[0] == "/bin");
    REQUIRE(values[1].empty());
    REQUIRE(values[2].empty());
    REQUIRE(values[3] == "/usr/bin");
}

TEST_CASE("PtyEnv empty value produces one empty component", "[job_io][pty][env][values]")
{
    PtyEnv env("JOB_EMPTY", "");

    const auto values = env.values();

    REQUIRE(values.size() == 1);
    REQUIRE(values[0].empty());
}

TEST_CASE("PtyEnv single value produces one component", "[job_io][pty][env][values]")
{
    PtyEnv env("HOME", "/home/job");

    const auto values = env.values();

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == "/home/job");
}

TEST_CASE("PtyEnv values reflect a changed value", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", "/bin:/usr/bin");

    {
        const auto values = env.values();

        REQUIRE(values.size() == 2);
        REQUIRE(values[0] == "/bin");
        REQUIRE(values[1] == "/usr/bin");
    }

    env.setValue("/sbin:/usr/sbin:/opt/bin");

    {
        const auto values = env.values();

        REQUIRE(values.size() == 3);
        REQUIRE(values[0] == "/sbin");
        REQUIRE(values[1] == "/usr/sbin");
        REQUIRE(values[2] == "/opt/bin");
    }
}

TEST_CASE("PtyEnv values reflect replacement with an empty value", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", "/bin:/usr/bin");

    REQUIRE(env.values().size() == 2);

    env.setValue("");

    const auto values = env.values();

    REQUIRE(values.size() == 1);
    REQUIRE(values[0].empty());
}

TEST_CASE("PtyEnv values reflect replacement after empty components", "[job_io][pty][env][values]")
{
    PtyEnv env("PATH", "/bin::/usr/bin:");

    {
        const auto values = env.values();

        REQUIRE(values.size() == 4);
        REQUIRE(values[1].empty());
        REQUIRE(values[3].empty());
    }

    env.setValue("/opt/bin");

    const auto values = env.values();

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == "/opt/bin");
}

TEST_CASE("PtyEnv copied object retains independent owned values", "[job_io][pty][env][copy]")
{
    PtyEnv original("PATH", "/bin:/usr/bin", true, PtyEnv::EnvType::Posix);
    PtyEnv copy = original;

    REQUIRE(copy.name() == "PATH");
    REQUIRE(copy.value() == "/bin:/usr/bin");
    REQUIRE(copy.enabled());
    REQUIRE(copy.envType() == PtyEnv::EnvType::Posix);

    copy.setName("JOB_PATH");
    copy.setValue("/opt/bin");
    copy.setEnabled(false);
    copy.setEnvType(PtyEnv::EnvType::Custom);

    REQUIRE(original.name() == "PATH");
    REQUIRE(original.value() == "/bin:/usr/bin");
    REQUIRE(original.enabled());
    REQUIRE(original.envType() == PtyEnv::EnvType::Posix);

    REQUIRE(copy.name() == "JOB_PATH");
    REQUIRE(copy.value() == "/opt/bin");
    REQUIRE_FALSE(copy.enabled());
    REQUIRE(copy.envType() == PtyEnv::EnvType::Custom);
}

TEST_CASE("PtyEnv createShared constructs a shared instance", "[job_io][pty][env]")
{
    auto env = PtyEnv::createShared("JOB_SHARED", "value", true, PtyEnv::EnvType::Custom);

    REQUIRE(env);
    REQUIRE(env->name() == "JOB_SHARED");
    REQUIRE(env->value() == "value");
    REQUIRE(env->enabled());
    REQUIRE(env->envType() == PtyEnv::EnvType::Custom);
}

TEST_CASE("PtyEnv createUniq constructs a unique instance", "[job_io][pty][env]")
{
    auto env = PtyEnv::createUniq("JOB_UNIQUE", "value", true, PtyEnv::EnvType::Custom);

    REQUIRE(env);
    REQUIRE(env->name() == "JOB_UNIQUE");
    REQUIRE(env->value() == "value");
    REQUIRE(env->enabled());
    REQUIRE(env->envType() == PtyEnv::EnvType::Custom);
}