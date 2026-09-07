#include <catch2/catch_test_macros.hpp>

#include <job_yaml_cursor.h>

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

constexpr bool constexprCursorTest()
{
    YamlCursor cursor{"abc"};

    if (!cursor)
        return false;

    if (cursor.empty())
        return false;

    if (cursor.current() != 'a')
        return false;

    if (cursor.peek() != 'b')
        return false;

    if (cursor.peek(2) != 'c')
        return false;

    if (cursor.peek(3) != '\0')
        return false;

    cursor.advance();

    if (cursor.offset() != 1 || cursor.current() != 'b')
        return false;

    if (!cursor.consume('b'))
        return false;

    if (cursor.offset() != 2 || cursor.current() != 'c')
        return false;

    if (cursor.consume('x'))
        return false;

    if (cursor.offset() != 2)
        return false;

    if (!cursor.consume("c"sv))
        return false;

    return cursor.empty() && !cursor && cursor.offset() == 3;
}

static_assert(constexprCursorTest());

TEST_CASE("YamlCursor initial state", "[job_yaml][cursor]")
{
    constexpr std::string_view source = "hello";

    YamlCursor cursor{source};

    REQUIRE_FALSE(cursor.empty());
    REQUIRE(static_cast<bool>(cursor));
    REQUIRE(cursor.source() == source);
    REQUIRE(cursor.remaining() == source);
    REQUIRE(cursor.size() == source.size());
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'h');
}

TEST_CASE("YamlCursor handles an empty source", "[job_yaml][cursor]")
{
    YamlCursor cursor{""};

    REQUIRE(cursor.empty());
    REQUIRE_FALSE(static_cast<bool>(cursor));
    REQUIRE(cursor.size() == 0);
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == '\0');
    REQUIRE(cursor.peek() == '\0');
    REQUIRE(cursor.remaining().empty());
}

TEST_CASE("YamlCursor peeks without advancing", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abcdef"};

    REQUIRE(cursor.current() == 'a');
    REQUIRE(cursor.peek() == 'b');
    REQUIRE(cursor.peek(2) == 'c');
    REQUIRE(cursor.peek(5) == 'f');
    REQUIRE(cursor.peek(6) == '\0');
    REQUIRE(cursor.peek(100) == '\0');
    REQUIRE(cursor.offset() == 0);
}

TEST_CASE("YamlCursor advances through source", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abcdef"};

    cursor.advance();

    REQUIRE(cursor.offset() == 1);
    REQUIRE(cursor.current() == 'b');
    REQUIRE(cursor.remaining() == "bcdef");

    cursor.advance(3);

    REQUIRE(cursor.offset() == 4);
    REQUIRE(cursor.current() == 'e');
    REQUIRE(cursor.remaining() == "ef");
}

TEST_CASE("YamlCursor advance saturates at end", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abc"};

    cursor.advance(100);

    REQUIRE(cursor.empty());
    REQUIRE_FALSE(static_cast<bool>(cursor));
    REQUIRE(cursor.offset() == 3);
    REQUIRE(cursor.current() == '\0');
    REQUIRE(cursor.peek() == '\0');
    REQUIRE(cursor.remaining().empty());

    cursor.advance();

    REQUIRE(cursor.offset() == 3);

    cursor.advance(100);

    REQUIRE(cursor.offset() == 3);
}

TEST_CASE("YamlCursor consumes a matching character", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abc"};

    REQUIRE(cursor.consume('a'));
    REQUIRE(cursor.offset() == 1);
    REQUIRE(cursor.current() == 'b');

    REQUIRE(cursor.consume('b'));
    REQUIRE(cursor.offset() == 2);
    REQUIRE(cursor.current() == 'c');

    REQUIRE(cursor.consume('c'));
    REQUIRE(cursor.empty());
    REQUIRE(cursor.offset() == 3);
}

TEST_CASE("YamlCursor failed character consume does not advance", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abc"};

    REQUIRE_FALSE(cursor.consume('x'));

    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'a');
    REQUIRE(cursor.remaining() == "abc");

    cursor.advance(3);

    REQUIRE_FALSE(cursor.consume('x'));
    REQUIRE(cursor.offset() == 3);
}

TEST_CASE("YamlCursor consumes a matching string view", "[job_yaml][cursor]")
{
    YamlCursor cursor{"hello world"};

    REQUIRE(cursor.consume("hello"sv));
    REQUIRE(cursor.offset() == 5);
    REQUIRE(cursor.remaining() == " world");

    REQUIRE(cursor.consume(" world"sv));
    REQUIRE(cursor.offset() == 11);
    REQUIRE(cursor.empty());
}

TEST_CASE("YamlCursor failed string consume does not advance", "[job_yaml][cursor]")
{
    YamlCursor cursor{"hello"};

    REQUIRE_FALSE(cursor.consume("world"sv));

    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'h');
    REQUIRE(cursor.remaining() == "hello");
}

TEST_CASE("YamlCursor consumes an empty string without advancing", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abc"};

    REQUIRE(cursor.consume(""sv));
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'a');
}

TEST_CASE("YamlCursor checks prefixes at current position", "[job_yaml][cursor]")
{
    YamlCursor cursor{"hello world"};

    REQUIRE(cursor.startsWith("hello"sv));
    REQUIRE(cursor.startsWith("hell"sv));
    REQUIRE(cursor.startsWith(""sv));
    REQUIRE_FALSE(cursor.startsWith("world"sv));

    cursor.advance(6);

    REQUIRE(cursor.startsWith("world"sv));
    REQUIRE_FALSE(cursor.startsWith("hello"sv));
}

TEST_CASE("YamlCursor remaining view follows offset", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abcdef"};

    REQUIRE(cursor.remaining() == "abcdef");

    cursor.advance(2);
    REQUIRE(cursor.remaining() == "cdef");

    cursor.advance(2);
    REQUIRE(cursor.remaining() == "ef");

    cursor.advance(2);
    REQUIRE(cursor.remaining().empty());
}

TEST_CASE("YamlCursor reset returns to beginning", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abcdef"};

    cursor.advance(4);

    REQUIRE(cursor.offset() == 4);
    REQUIRE(cursor.current() == 'e');

    cursor.reset();

    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'a');
    REQUIRE(cursor.remaining() == "abcdef");
}

TEST_CASE("YamlCursor reset moves to requested offset", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abcdef"};

    cursor.reset(3);

    REQUIRE(cursor.offset() == 3);
    REQUIRE(cursor.current() == 'd');
    REQUIRE(cursor.remaining() == "def");

    cursor.reset(1);

    REQUIRE(cursor.offset() == 1);
    REQUIRE(cursor.current() == 'b');
    REQUIRE(cursor.remaining() == "bcdef");
}

TEST_CASE("YamlCursor reset past end clamps to source size", "[job_yaml][cursor]")
{
    YamlCursor cursor{"abc"};

    cursor.reset(100);

    REQUIRE(cursor.offset() == 3);
    REQUIRE(cursor.empty());
    REQUIRE(cursor.current() == '\0');

    cursor.reset(0);

    REQUIRE(cursor.offset() == 0);
    REQUIRE_FALSE(cursor.empty());
    REQUIRE(cursor.current() == 'a');
}

TEST_CASE("YamlCursor source remains unchanged while cursor moves", "[job_yaml][cursor]")
{
    constexpr std::string_view source = "abcdef";

    YamlCursor cursor{source};

    cursor.advance(4);

    REQUIRE(cursor.source() == source);
    REQUIRE(cursor.source() == "abcdef");
    REQUIRE(cursor.remaining() == "ef");
}

} // namespace job::yaml::tests