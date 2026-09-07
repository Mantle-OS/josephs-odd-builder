#include <catch2/catch_test_macros.hpp>

#include <type_traits>

#include <job_yaml_parser_state.h>

namespace job::yaml::tests {

constexpr bool constexprParserStateTest()
{
    YamlParserState state{"name: value"};

    if (!state.atRoot())
        return false;

    if (state.depth() != 0)
        return false;

    if (state.cursor().offset() != 0)
        return false;

    if (state.cursor().current() != 'n')
        return false;

    if (!state.push(YamlParserContext::Mapping, 0))
        return false;

    if (state.atRoot())
        return false;

    if (state.depth() != 1)
        return false;

    if (state.context() != YamlParserContext::Mapping)
        return false;

    if (state.indent() != 0)
        return false;

    state.pop();

    return state.atRoot() && state.depth() == 0;
}

static_assert(std::is_trivially_default_constructible_v<YamlParserFrame>);
static_assert(std::is_trivially_destructible_v<YamlParserFrame>);
static_assert(constexprParserStateTest());

TEST_CASE("YamlParserState starts at the root", "[job_yaml][parser_state]")
{
    YamlParserState state{"name: value"};

    REQUIRE(state.atRoot());
    REQUIRE(state.depth() == 0);
    REQUIRE(state.cursor().offset() == 0);
    REQUIRE(state.cursor().source() == "name: value");
    REQUIRE(state.indents().empty());
}

TEST_CASE("YamlParserState exposes its source cursor", "[job_yaml][parser_state][cursor]")
{
    YamlParserState state{"abcdef"};

    REQUIRE(state.cursor().current() == 'a');
    REQUIRE(state.cursor().offset() == 0);

    state.cursor().advance(2);

    REQUIRE(state.cursor().current() == 'c');
    REQUIRE(state.cursor().offset() == 2);
    REQUIRE(state.cursor().remaining() == "cdef");
}

TEST_CASE("YamlParserState pushes mapping frames", "[job_yaml][parser_state][frame]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 0));

    REQUIRE_FALSE(state.atRoot());
    REQUIRE(state.depth() == 1);
    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == 0);
    REQUIRE(state.frame().context == YamlParserContext::Mapping);
    REQUIRE(state.frame().indent == 0);
}

TEST_CASE("YamlParserState pushes sequence frames", "[job_yaml][parser_state][frame]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Sequence, 2));

    REQUIRE_FALSE(state.atRoot());
    REQUIRE(state.depth() == 1);
    REQUIRE(state.context() == YamlParserContext::Sequence);
    REQUIRE(state.indent() == 2);
    REQUIRE(state.frame().context == YamlParserContext::Sequence);
    REQUIRE(state.frame().indent == 2);
}

TEST_CASE("YamlParserState stores frame indentation", "[job_yaml][parser_state][frame][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 4));

    REQUIRE(state.depth() == 1);
    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == 4);
    REQUIRE(state.frame().indent == 4);
}

TEST_CASE("YamlParserState tracks nested structural contexts",
          "[job_yaml][parser_state][frame]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 0));
    REQUIRE(state.push(YamlParserContext::Sequence, 2));
    REQUIRE(state.push(YamlParserContext::Mapping, 4));

    REQUIRE(state.depth() == 3);
    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == 4);

    state.pop();

    REQUIRE(state.depth() == 2);
    REQUIRE(state.context() == YamlParserContext::Sequence);
    REQUIRE(state.indent() == 2);

    state.pop();

    REQUIRE(state.depth() == 1);
    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == 0);

    state.pop();

    REQUIRE(state.atRoot());
    REQUIRE(state.depth() == 0);
}

TEST_CASE("YamlParserState restores parent frame indentation after pop",
          "[job_yaml][parser_state][frame][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 0));
    REQUIRE(state.push(YamlParserContext::Mapping, 2));
    REQUIRE(state.push(YamlParserContext::Sequence, 6));

    REQUIRE(state.indent() == 6);

    state.pop();

    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == 2);

    state.pop();

    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == 0);
}

TEST_CASE("YamlParserState permits frames with equal indentation",
          "[job_yaml][parser_state][frame][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 2));
    REQUIRE(state.push(YamlParserContext::Sequence, 2));

    REQUIRE(state.depth() == 2);
    REQUIRE(state.context() == YamlParserContext::Sequence);
    REQUIRE(state.indent() == 2);
}

TEST_CASE("YamlParserState does not validate frame indentation ordering",
          "[job_yaml][parser_state][frame][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 4));
    REQUIRE(state.push(YamlParserContext::Sequence, 2));

    REQUIRE(state.depth() == 2);
    REQUIRE(state.context() == YamlParserContext::Sequence);
    REQUIRE(state.indent() == 2);
}

TEST_CASE("YamlParserState clears structural frames", "[job_yaml][parser_state][frame]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 0));
    REQUIRE(state.push(YamlParserContext::Sequence, 2));
    REQUIRE(state.depth() == 2);

    state.clearFrames();

    REQUIRE(state.atRoot());
    REQUIRE(state.depth() == 0);
}

TEST_CASE("YamlParserState clearFrames does not clear indentation state",
          "[job_yaml][parser_state][frame][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.indents().push(2));
    REQUIRE(state.indents().push(4));

    REQUIRE(state.push(YamlParserContext::Mapping, 0));
    REQUIRE(state.push(YamlParserContext::Sequence, 2));

    state.clearFrames();

    REQUIRE(state.atRoot());
    REQUIRE(state.depth() == 0);

    REQUIRE(state.indents().depth() == 2);
    REQUIRE(state.indents().current() == 4);
}

TEST_CASE("YamlParserState supports maximum structural depth",
          "[job_yaml][parser_state][depth]")
{
    YamlParserState state{"value"};

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i) {
        REQUIRE(state.push(YamlParserContext::Mapping, i));
        REQUIRE(state.depth() == i + 1);
        REQUIRE(state.indent() == i);
    }

    REQUIRE(state.depth() == YamlIndentStack::MaxDepth);
    REQUIRE_FALSE(state.atRoot());
}

TEST_CASE("YamlParserState rejects frames beyond maximum structural depth",
          "[job_yaml][parser_state][depth]")
{
    YamlParserState state{"value"};

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i)
        REQUIRE(state.push(YamlParserContext::Mapping, i));

    const std::size_t previousDepth = state.depth();
    const std::size_t previousIndent = state.indent();

    REQUIRE_FALSE(state.push(YamlParserContext::Sequence, YamlIndentStack::MaxDepth));

    REQUIRE(state.depth() == previousDepth);
    REQUIRE(state.context() == YamlParserContext::Mapping);
    REQUIRE(state.indent() == previousIndent);
}

TEST_CASE("YamlParserState exposes independent indentation state",
          "[job_yaml][parser_state][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.indents().empty());

    REQUIRE(state.indents().push(2));
    REQUIRE(state.indents().push(4));

    REQUIRE(state.indents().depth() == 2);
    REQUIRE(state.indents().current() == 4);

    REQUIRE(state.atRoot());
    REQUIRE(state.depth() == 0);
}

TEST_CASE("YamlParserState frame and indentation stacks are independent",
          "[job_yaml][parser_state][indent]")
{
    YamlParserState state{"value"};

    REQUIRE(state.push(YamlParserContext::Mapping, 0));
    REQUIRE(state.push(YamlParserContext::Sequence, 6));

    REQUIRE(state.indents().push(2));
    REQUIRE(state.indents().push(4));

    REQUIRE(state.depth() == 2);
    REQUIRE(state.indent() == 6);

    REQUIRE(state.indents().depth() == 2);
    REQUIRE(state.indents().current() == 4);
}

TEST_CASE("YamlParserState marks the current source position",
          "[job_yaml][parser_state][mark]")
{
    YamlParserState state{"abcdef"};

    state.cursor().advance(2);
    state.mark();

    REQUIRE(state.markOffset() == 2);

    state.cursor().advance(3);

    REQUIRE(state.cursor().offset() == 5);
    REQUIRE(state.markedView() == "cde");
}

TEST_CASE("YamlParserState marked view may be empty",
          "[job_yaml][parser_state][mark]")
{
    YamlParserState state{"abcdef"};

    state.cursor().advance(3);
    state.mark();

    REQUIRE(state.markOffset() == 3);
    REQUIRE(state.markedView().empty());
}

TEST_CASE("YamlParserState rewinds to the marked source position",
          "[job_yaml][parser_state][mark]")
{
    YamlParserState state{"abcdef"};

    state.cursor().advance(2);
    state.mark();
    state.cursor().advance(3);

    REQUIRE(state.cursor().offset() == 5);
    REQUIRE(state.markedView() == "cde");

    state.rewindToMark();

    REQUIRE(state.cursor().offset() == 2);
    REQUIRE(state.cursor().current() == 'c');
}

TEST_CASE("YamlParserState mark may be replaced",
          "[job_yaml][parser_state][mark]")
{
    YamlParserState state{"abcdef"};

    state.cursor().advance(1);
    state.mark();

    REQUIRE(state.markOffset() == 1);

    state.cursor().advance(3);
    state.mark();

    REQUIRE(state.markOffset() == 4);

    state.cursor().advance(2);

    REQUIRE(state.markedView() == "ef");
}

TEST_CASE("YamlParserState shared factory constructs state",
          "[job_yaml][parser_state][factory]")
{
    auto state = YamlParserState::createShared("name: value");

    REQUIRE(state);
    REQUIRE(state->cursor().source() == "name: value");
    REQUIRE(state->atRoot());
}

TEST_CASE("YamlParserState unique factory constructs state",
          "[job_yaml][parser_state][factory]")
{
    auto state = YamlParserState::createUniq("name: value");

    REQUIRE(state);
    REQUIRE(state->cursor().source() == "name: value");
    REQUIRE(state->atRoot());
}

} // namespace job::yaml::tests