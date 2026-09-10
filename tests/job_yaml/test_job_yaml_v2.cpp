#include <job_yaml.h>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace job::yaml::tests {

struct StringObject
{
    int count{};
    bool enabled{};
    std::string name{};
};

TEST_CASE("JobYaml converts object to string", "[job_yaml][string]")
{
    const StringObject object{
        .count = 42,
        .enabled = true,
        .name = "JOB",
    };

    std::string yaml;

    REQUIRE(JobYaml::toString(object, yaml));

    REQUIRE(yaml ==
            "count: 42\n"
            "enabled: true\n"
            "name: \"JOB\"\n");
}

TEST_CASE("JobYaml converts string to object", "[job_yaml][string]")
{
    constexpr std::string_view yaml =
        "count: 42\n"
        "enabled: true\n"
        "name: \"JOB\"\n";

    StringObject object;

    REQUIRE(JobYaml::fromString(yaml, object));

    REQUIRE(object.count == 42);
    REQUIRE(object.enabled);
    REQUIRE(object.name == "JOB");
}

TEST_CASE("JobYaml round trips object through string", "[job_yaml][string]")
{
    const StringObject source{
        .count = 42,
        .enabled = true,
        .name = "JOB",
    };

    std::string yaml;
    REQUIRE(JobYaml::toString(source, yaml));

    StringObject destination;
    REQUIRE(JobYaml::fromString(yaml, destination));

    REQUIRE(destination.count == source.count);
    REQUIRE(destination.enabled == source.enabled);
    REQUIRE(destination.name == source.name);
}

TEST_CASE("JobYaml converts string to node", "[job_yaml][string]")
{
    constexpr std::string_view yaml =
        "count: 42\n"
        "enabled: true\n"
        "name: \"JOB\"\n";

    YamlNode node;

    REQUIRE(JobYaml::fromString(yaml, node));
    REQUIRE(node.isMapping());

    const YamlNode *count = node.member("count");
    const YamlNode *enabled = node.member("enabled");
    const YamlNode *name = node.member("name");

    REQUIRE(count != nullptr);
    REQUIRE(enabled != nullptr);
    REQUIRE(name != nullptr);

    REQUIRE(count->isScalar());
    REQUIRE(enabled->isScalar());
    REQUIRE(name->isScalar());

    REQUIRE(count->scalar() == "42");
    REQUIRE(enabled->scalar() == "true");
    REQUIRE(name->scalar() == "JOB");
}

} // namespace job::yaml::tests