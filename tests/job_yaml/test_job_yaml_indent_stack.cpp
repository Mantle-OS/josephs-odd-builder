#include <catch2/catch_test_macros.hpp>

#include <job_yaml_indent_stack.h>

namespace job::yaml::tests {

constexpr bool constexprIndentStackTest()
{
    YamlIndentStack stack;

    if (!stack.empty())
        return false;

    if (stack.current() != 0)
        return false;

    if (stack.relation(0) != YamlIndentRelation::Same)
        return false;

    if (stack.relation(2) != YamlIndentRelation::Deeper)
        return false;

    if (!stack.push(2))
        return false;

    if (!stack.push(4))
        return false;

    if (stack.depth() != 2)
        return false;

    if (stack.current() != 4)
        return false;

    if (stack.relation(4) != YamlIndentRelation::Same)
        return false;

    if (stack.relation(6) != YamlIndentRelation::Deeper)
        return false;

    if (stack.relation(2) != YamlIndentRelation::Shallower)
        return false;

    if (!stack.popTo(2))
        return false;

    if (stack.current() != 2)
        return false;

    if (!stack.popTo(0))
        return false;

    return stack.empty() && stack.current() == 0;
}

constexpr bool constexprInvalidPopToPreservesStackTest()
{
    YamlIndentStack stack;

    if (!stack.push(2))
        return false;

    if (!stack.push(4))
        return false;

    if (!stack.push(6))
        return false;

    if (stack.popTo(3))
        return false;

    return stack.depth() == 3 &&
           stack.current() == 6 &&
           stack.contains(2) &&
           stack.contains(4) &&
           stack.contains(6);
}

static_assert(constexprIndentStackTest());
static_assert(constexprInvalidPopToPreservesStackTest());

TEST_CASE("YamlIndentStack starts at implicit root indentation",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.empty());
    REQUIRE(stack.depth() == 0);
    REQUIRE(stack.current() == 0);
    REQUIRE(stack.relation(0) == YamlIndentRelation::Same);
    REQUIRE(stack.relation(1) == YamlIndentRelation::Deeper);
}

TEST_CASE("YamlIndentStack pushes increasing indentation levels",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.depth() == 1);
    REQUIRE(stack.current() == 2);

    REQUIRE(stack.push(4));
    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);

    REQUIRE(stack.push(8));
    REQUIRE(stack.depth() == 3);
    REQUIRE(stack.current() == 8);
}

TEST_CASE("YamlIndentStack accepts arbitrary increasing indentation widths",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(1));
    REQUIRE(stack.push(3));
    REQUIRE(stack.push(9));

    REQUIRE(stack.depth() == 3);
    REQUIRE(stack.current() == 9);

    REQUIRE(stack.contains(1));
    REQUIRE(stack.contains(3));
    REQUIRE(stack.contains(9));
}

TEST_CASE("YamlIndentStack rejects equal indentation pushes",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));

    REQUIRE_FALSE(stack.push(2));
    REQUIRE(stack.depth() == 1);
    REQUIRE(stack.current() == 2);
}

TEST_CASE("YamlIndentStack rejects shallower indentation pushes",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));

    REQUIRE_FALSE(stack.push(3));
    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);

    REQUIRE_FALSE(stack.push(1));
    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);
}

TEST_CASE("YamlIndentStack reports indentation relation",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.relation(0) == YamlIndentRelation::Same);
    REQUIRE(stack.relation(2) == YamlIndentRelation::Deeper);

    REQUIRE(stack.push(2));

    REQUIRE(stack.relation(2) == YamlIndentRelation::Same);
    REQUIRE(stack.relation(4) == YamlIndentRelation::Deeper);
    REQUIRE(stack.relation(0) == YamlIndentRelation::Shallower);

    REQUIRE(stack.push(6));

    REQUIRE(stack.relation(6) == YamlIndentRelation::Same);
    REQUIRE(stack.relation(8) == YamlIndentRelation::Deeper);
    REQUIRE(stack.relation(2) == YamlIndentRelation::Shallower);
}

TEST_CASE("YamlIndentStack contains active indentation levels",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE_FALSE(stack.contains(0));
    REQUIRE_FALSE(stack.contains(2));

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(8));

    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(4));
    REQUIRE(stack.contains(8));

    REQUIRE_FALSE(stack.contains(0));
    REQUIRE_FALSE(stack.contains(3));
    REQUIRE_FALSE(stack.contains(6));
}

TEST_CASE("YamlIndentStack does not store implicit root indentation",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.current() == 0);
    REQUIRE_FALSE(stack.contains(0));

    REQUIRE(stack.push(2));

    REQUIRE(stack.current() == 2);
    REQUIRE_FALSE(stack.contains(0));

    REQUIRE(stack.popTo(0));

    REQUIRE(stack.empty());
    REQUIRE(stack.current() == 0);
    REQUIRE_FALSE(stack.contains(0));
}

TEST_CASE("YamlIndentStack pops the current indentation level",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(6));

    REQUIRE(stack.pop());
    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);

    REQUIRE(stack.pop());
    REQUIRE(stack.depth() == 1);
    REQUIRE(stack.current() == 2);

    REQUIRE(stack.pop());
    REQUIRE(stack.empty());
    REQUIRE(stack.depth() == 0);
    REQUIRE(stack.current() == 0);
}

TEST_CASE("YamlIndentStack pop on empty stack fails without changing state",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE_FALSE(stack.pop());
    REQUIRE(stack.empty());
    REQUIRE(stack.depth() == 0);
    REQUIRE(stack.current() == 0);
}

TEST_CASE("YamlIndentStack pops to an existing indentation level",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(6));
    REQUIRE(stack.push(8));

    REQUIRE(stack.popTo(4));

    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);
    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(4));
    REQUIRE_FALSE(stack.contains(6));
    REQUIRE_FALSE(stack.contains(8));
}

TEST_CASE("YamlIndentStack popTo current indentation preserves stack",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));

    REQUIRE(stack.popTo(4));

    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);
    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(4));
}

TEST_CASE("YamlIndentStack pops through multiple levels",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(1));
    REQUIRE(stack.push(3));
    REQUIRE(stack.push(5));
    REQUIRE(stack.push(9));

    REQUIRE(stack.popTo(1));

    REQUIRE(stack.depth() == 1);
    REQUIRE(stack.current() == 1);
    REQUIRE(stack.contains(1));
    REQUIRE_FALSE(stack.contains(3));
    REQUIRE_FALSE(stack.contains(5));
    REQUIRE_FALSE(stack.contains(9));
}

TEST_CASE("YamlIndentStack pops to implicit root",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(6));

    REQUIRE(stack.popTo(0));

    REQUIRE(stack.empty());
    REQUIRE(stack.depth() == 0);
    REQUIRE(stack.current() == 0);
}

TEST_CASE("YamlIndentStack popTo implicit root succeeds on empty stack",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.popTo(0));
    REQUIRE(stack.empty());
    REQUIRE(stack.depth() == 0);
    REQUIRE(stack.current() == 0);
}

TEST_CASE("YamlIndentStack invalid popTo preserves active indentation levels",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(6));

    REQUIRE_FALSE(stack.popTo(3));

    REQUIRE(stack.depth() == 3);
    REQUIRE(stack.current() == 6);
    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(4));
    REQUIRE(stack.contains(6));
    REQUIRE_FALSE(stack.contains(3));
}

TEST_CASE("YamlIndentStack invalid popTo below first level preserves stack",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));

    REQUIRE_FALSE(stack.popTo(1));

    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);
    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(4));
}

TEST_CASE("YamlIndentStack invalid popTo between active levels preserves stack",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(6));
    REQUIRE(stack.push(10));

    REQUIRE_FALSE(stack.popTo(8));

    REQUIRE(stack.depth() == 3);
    REQUIRE(stack.current() == 10);
    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(6));
    REQUIRE(stack.contains(10));
    REQUIRE_FALSE(stack.contains(8));
}

TEST_CASE("YamlIndentStack invalid popTo deeper than current preserves stack",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));

    REQUIRE_FALSE(stack.popTo(6));

    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);
    REQUIRE(stack.contains(2));
    REQUIRE(stack.contains(4));
}

TEST_CASE("YamlIndentStack remains usable after invalid popTo",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(8));

    REQUIRE_FALSE(stack.popTo(6));

    REQUIRE(stack.depth() == 3);
    REQUIRE(stack.current() == 8);

    REQUIRE(stack.popTo(4));

    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 4);

    REQUIRE(stack.push(10));

    REQUIRE(stack.depth() == 3);
    REQUIRE(stack.current() == 10);
}

TEST_CASE("YamlIndentStack clear removes all indentation levels",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));
    REQUIRE(stack.push(6));

    stack.clear();

    REQUIRE(stack.empty());
    REQUIRE(stack.depth() == 0);
    REQUIRE(stack.current() == 0);
    REQUIRE_FALSE(stack.contains(2));
    REQUIRE_FALSE(stack.contains(4));
    REQUIRE_FALSE(stack.contains(6));
}

TEST_CASE("YamlIndentStack can be reused after clear",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    REQUIRE(stack.push(2));
    REQUIRE(stack.push(4));

    stack.clear();

    REQUIRE(stack.push(3));
    REQUIRE(stack.push(7));

    REQUIRE(stack.depth() == 2);
    REQUIRE(stack.current() == 7);
    REQUIRE(stack.contains(3));
    REQUIRE(stack.contains(7));
}

TEST_CASE("YamlIndentStack supports maximum configured depth",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i) {
        const YamlIndentStack::Indent indent = i + 1;

        REQUIRE(stack.push(indent));
        REQUIRE(stack.depth() == i + 1);
        REQUIRE(stack.current() == indent);
    }

    REQUIRE(stack.depth() == YamlIndentStack::MaxDepth);
    REQUIRE_FALSE(stack.empty());
}

TEST_CASE("YamlIndentStack rejects push beyond maximum depth",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i) {
        const YamlIndentStack::Indent indent = i + 1;
        REQUIRE(stack.push(indent));
    }

    const std::size_t previousDepth = stack.depth();
    const YamlIndentStack::Indent previousIndent = stack.current();

    REQUIRE_FALSE(stack.push(previousIndent + 1));

    REQUIRE(stack.depth() == previousDepth);
    REQUIRE(stack.current() == previousIndent);
}

TEST_CASE("YamlIndentStack remains unchanged after rejected push at maximum depth",
          "[job_yaml][indent_stack]")
{
    YamlIndentStack stack;

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i)
        REQUIRE(stack.push(i + 1));

    const std::size_t previousDepth = stack.depth();
    const YamlIndentStack::Indent previousIndent = stack.current();

    REQUIRE_FALSE(stack.push(previousIndent + 100));

    REQUIRE(stack.depth() == previousDepth);
    REQUIRE(stack.current() == previousIndent);
    REQUIRE(stack.contains(previousIndent));
}

} // namespace job::yaml::tests