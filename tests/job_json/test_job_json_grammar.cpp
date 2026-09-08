#include <catch2/catch_test_macros.hpp>

#include <job_json_grammar.h>

using namespace job::json;

TEST_CASE("JSON grammar code points", "[job_json][grammar]")
{
    STATIC_REQUIRE(grammar::decimal_point == U'.');
    STATIC_REQUIRE(grammar::minus == U'-');
    STATIC_REQUIRE(grammar::plus == U'+');
    STATIC_REQUIRE(grammar::zero == U'0');
    STATIC_REQUIRE(grammar::escape == U'\\');
    STATIC_REQUIRE(grammar::quotation_mark == U'"');
}

TEST_CASE("JSON grammar digit ranges", "[job_json][grammar]")
{
    STATIC_REQUIRE(grammar::is_digit1_9(U'1'));
    STATIC_REQUIRE(grammar::is_digit1_9(U'9'));

    STATIC_REQUIRE_FALSE(grammar::is_digit1_9(U'0'));
    STATIC_REQUIRE_FALSE(grammar::is_digit1_9(U':'));

    STATIC_REQUIRE(grammar::is_DIGIT(U'0'));
    STATIC_REQUIRE(grammar::is_DIGIT(U'9'));

    STATIC_REQUIRE_FALSE(grammar::is_DIGIT(U'/'));
    STATIC_REQUIRE_FALSE(grammar::is_DIGIT(U':'));
}

TEST_CASE("JSON grammar exponent predicate", "[job_json][grammar]")
{
    STATIC_REQUIRE(grammar::is_e(U'e'));
    STATIC_REQUIRE(grammar::is_e(U'E'));

    STATIC_REQUIRE_FALSE(grammar::is_e(U'd'));
    STATIC_REQUIRE_FALSE(grammar::is_e(U'f'));
}

TEST_CASE("JSON grammar unescaped characters", "[job_json][grammar]")
{
    STATIC_REQUIRE(grammar::is_unescaped(U' '));
    STATIC_REQUIRE(grammar::is_unescaped(U'!'));
    STATIC_REQUIRE(grammar::is_unescaped(U'#'));
    STATIC_REQUIRE(grammar::is_unescaped(U'['));
    STATIC_REQUIRE(grammar::is_unescaped(U']'));
    STATIC_REQUIRE(grammar::is_unescaped(U'\U0010FFFF'));

    STATIC_REQUIRE_FALSE(grammar::is_unescaped(U'\x1F'));
    STATIC_REQUIRE_FALSE(grammar::is_unescaped(U'"'));
    STATIC_REQUIRE_FALSE(grammar::is_unescaped(U'\\'));
}

TEST_CASE("JSON grammar hexadecimal digits", "[job_json][grammar]")
{
    STATIC_REQUIRE(grammar::is_HEXDIG(U'0'));
    STATIC_REQUIRE(grammar::is_HEXDIG(U'9'));

    STATIC_REQUIRE(grammar::is_HEXDIG(U'a'));
    STATIC_REQUIRE(grammar::is_HEXDIG(U'f'));
    STATIC_REQUIRE(grammar::is_HEXDIG(U'A'));
    STATIC_REQUIRE(grammar::is_HEXDIG(U'F'));

    STATIC_REQUIRE_FALSE(grammar::is_HEXDIG(U'g'));
    STATIC_REQUIRE_FALSE(grammar::is_HEXDIG(U'G'));
}

TEST_CASE("JSON grammar literal strings", "[job_json][grammar]")
{
    STATIC_REQUIRE(grammar::false_ == "false");
    STATIC_REQUIRE(grammar::null == "null");
    STATIC_REQUIRE(grammar::true_ == "true");
}