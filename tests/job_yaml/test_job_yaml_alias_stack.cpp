#include <catch2/catch_test_macros.hpp>

#include <job_yaml_alias_stack.h>

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Default state
// ========================================

TEST_CASE("YamlAliasStack default constructs empty", "[job_yaml][alias_stack]")
{
    const YamlAliasStack stack;

    REQUIRE(stack.empty());
    REQUIRE(stack.size() == 0);
    REQUIRE_FALSE(stack.contains("alpha"));
}

// ========================================
// Push
// ========================================

TEST_CASE("YamlAliasStack pushes alias", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));

    REQUIRE_FALSE(stack.empty());
    REQUIRE(stack.size() == 1);
    REQUIRE(stack.contains("alpha"));
}

TEST_CASE("YamlAliasStack rejects empty alias name", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE_FALSE(stack.push({}));

    REQUIRE(stack.empty());
    REQUIRE(stack.size() == 0);
}

TEST_CASE("YamlAliasStack rejects duplicate active alias", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE_FALSE(stack.push("alpha"));

    REQUIRE(stack.size() == 1);
    REQUIRE(stack.contains("alpha"));
}

TEST_CASE("YamlAliasStack allows distinct active aliases", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.push("beta"));
    REQUIRE(stack.push("gamma"));

    REQUIRE(stack.size() == 3);
    REQUIRE(stack.contains("alpha"));
    REQUIRE(stack.contains("beta"));
    REQUIRE(stack.contains("gamma"));
}

TEST_CASE("YamlAliasStack alias names are borrowed views", "[job_yaml][alias_stack]")
{
    constexpr std::string_view source = "alpha"sv;

    YamlAliasStack stack;

    REQUIRE(stack.push(source));
    REQUIRE(stack.contains(source));
}

// ========================================
// Contains
// ========================================

TEST_CASE("YamlAliasStack contains reports active aliases", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.push("beta"));

    REQUIRE(stack.contains("alpha"));
    REQUIRE(stack.contains("beta"));
    REQUIRE_FALSE(stack.contains("gamma"));
}

// ========================================
// Pop
// ========================================

TEST_CASE("YamlAliasStack pop removes most recent alias", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.push("beta"));

    stack.pop();

    REQUIRE(stack.size() == 1);
    REQUIRE(stack.contains("alpha"));
    REQUIRE_FALSE(stack.contains("beta"));
}

TEST_CASE("YamlAliasStack pop allows alias to be pushed again", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.push("beta"));

    stack.pop();

    REQUIRE(stack.push("beta"));
    REQUIRE(stack.size() == 2);
    REQUIRE(stack.contains("alpha"));
    REQUIRE(stack.contains("beta"));
}

TEST_CASE("YamlAliasStack pop on empty stack is tolerated", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    stack.pop();

    REQUIRE(stack.empty());
    REQUIRE(stack.size() == 0);
}

// ========================================
// Cycle detection behavior
// ========================================

TEST_CASE("YamlAliasStack rejects direct recursive alias", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE_FALSE(stack.push("alpha"));

    REQUIRE(stack.size() == 1);
}

TEST_CASE("YamlAliasStack rejects indirect recursive alias", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.push("beta"));
    REQUIRE(stack.push("gamma"));

    REQUIRE_FALSE(stack.push("alpha"));
    REQUIRE_FALSE(stack.push("beta"));

    REQUIRE(stack.size() == 3);
}

// ========================================
// Clear
// ========================================

TEST_CASE("YamlAliasStack clear removes all aliases", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.push("beta"));

    stack.clear();

    REQUIRE(stack.empty());
    REQUIRE(stack.size() == 0);
    REQUIRE_FALSE(stack.contains("alpha"));
    REQUIRE_FALSE(stack.contains("beta"));
}

TEST_CASE("YamlAliasStack can be reused after clear", "[job_yaml][alias_stack]")
{
    YamlAliasStack stack;

    REQUIRE(stack.push("alpha"));

    stack.clear();

    REQUIRE(stack.push("alpha"));
    REQUIRE(stack.size() == 1);
    REQUIRE(stack.contains("alpha"));
}

} // namespace job::yaml::tests