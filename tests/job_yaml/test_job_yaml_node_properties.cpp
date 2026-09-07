#include <catch2/catch_test_macros.hpp>

#include <job_yaml_node_properties.h>

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Default state
// ========================================

TEST_CASE("YamlNodeProperties default constructs empty", "[job_yaml][node_properties]")
{
    const YamlNodeProperties properties;

    REQUIRE(properties.empty());
    REQUIRE_FALSE(properties.hasAnchor());
    REQUIRE_FALSE(properties.hasTag());
    REQUIRE(properties.anchor.empty());
    REQUIRE(properties.tag.empty());
}

// ========================================
// Anchor
// ========================================

TEST_CASE("YamlNodeProperties reports anchor", "[job_yaml][node_properties]")
{
    YamlNodeProperties properties;
    properties.anchor = "alpha"sv;

    REQUIRE_FALSE(properties.empty());
    REQUIRE(properties.hasAnchor());
    REQUIRE_FALSE(properties.hasTag());
    REQUIRE(properties.anchor == "alpha");
}

TEST_CASE("YamlNodeProperties anchor is a borrowed view", "[job_yaml][node_properties]")
{
    constexpr std::string_view source = "alpha"sv;

    YamlNodeProperties properties;
    properties.anchor = source;

    REQUIRE(properties.anchor.data() == source.data());
    REQUIRE(properties.anchor.size() == source.size());
}

// ========================================
// Tag
// ========================================

TEST_CASE("YamlNodeProperties reports tag", "[job_yaml][node_properties]")
{
    YamlNodeProperties properties;
    properties.tag = "!example"sv;

    REQUIRE_FALSE(properties.empty());
    REQUIRE_FALSE(properties.hasAnchor());
    REQUIRE(properties.hasTag());
    REQUIRE(properties.tag == "!example");
}

TEST_CASE("YamlNodeProperties tag is a borrowed view", "[job_yaml][node_properties]")
{
    constexpr std::string_view source = "!example"sv;

    YamlNodeProperties properties;
    properties.tag = source;

    REQUIRE(properties.tag.data() == source.data());
    REQUIRE(properties.tag.size() == source.size());
}

// ========================================
// Combined properties
// ========================================

TEST_CASE("YamlNodeProperties holds anchor and tag together", "[job_yaml][node_properties]")
{
    YamlNodeProperties properties{
        .anchor = "alpha"sv,
        .tag = "!example"sv
    };

    REQUIRE_FALSE(properties.empty());
    REQUIRE(properties.hasAnchor());
    REQUIRE(properties.hasTag());
    REQUIRE(properties.anchor == "alpha");
    REQUIRE(properties.tag == "!example");
}

// ========================================
// Clear
// ========================================

TEST_CASE("YamlNodeProperties clear resets all properties", "[job_yaml][node_properties]")
{
    YamlNodeProperties properties{
        .anchor = "alpha"sv,
        .tag = "!example"sv
    };

    properties.clear();

    REQUIRE(properties.empty());
    REQUIRE_FALSE(properties.hasAnchor());
    REQUIRE_FALSE(properties.hasTag());
    REQUIRE(properties.anchor.empty());
    REQUIRE(properties.tag.empty());
}

// ========================================
// constexpr
// ========================================

static_assert([] {
    YamlNodeProperties properties;

    if (!properties.empty())
        return false;

    properties.anchor = "alpha"sv;

    if (!properties.hasAnchor() || properties.hasTag() || properties.empty())
        return false;

    properties.tag = "!example"sv;

    if (!properties.hasAnchor() || !properties.hasTag() || properties.empty())
        return false;

    properties.clear();

    return properties.empty() &&
           !properties.hasAnchor() &&
           !properties.hasTag();
}());

} // namespace job::yaml::tests