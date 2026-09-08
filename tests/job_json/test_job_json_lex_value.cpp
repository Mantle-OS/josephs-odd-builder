#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_lex.h>

namespace {

void requireLex(
    std::string_view source,
    job::json::JsonLexType expectedType,
    std::string_view expectedToken)
{
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == expectedType);
    REQUIRE(lex.valid());
    REQUIRE_FALSE(lex.isEnd());
    REQUIRE(lex.view(source) == expectedToken);
}

void requireInvalidLex(std::string_view source)
{
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::Invalid);
    REQUIRE_FALSE(lex.valid());
    REQUIRE_FALSE(lex.isEnd());
}

} // namespace

TEST_CASE("JsonLexer lexes true literal", "[job_json][lex][value]")
{
    requireLex("true", job::json::JsonLexType::True, "true");
}

TEST_CASE("JsonLexer lexes false literal", "[job_json][lex][value]")
{
    requireLex("false", job::json::JsonLexType::False, "false");
}

TEST_CASE("JsonLexer lexes null literal", "[job_json][lex][value]")
{
    requireLex("null", job::json::JsonLexType::Null, "null");
}

TEST_CASE("JsonLexer literal ranges are exact", "[job_json][lex][value]")
{
    constexpr std::string_view source = "  true false null";
    job::json::JsonLexer lexer{source};

    const auto trueLex = lexer.next();
    REQUIRE(trueLex.type() == job::json::JsonLexType::True);
    REQUIRE(trueLex.range().begin() == 2);
    REQUIRE(trueLex.range().end() == 6);
    REQUIRE(trueLex.view(source) == "true");

    const auto falseLex = lexer.next();
    REQUIRE(falseLex.type() == job::json::JsonLexType::False);
    REQUIRE(falseLex.range().begin() == 7);
    REQUIRE(falseLex.range().end() == 12);
    REQUIRE(falseLex.view(source) == "false");

    const auto nullLex = lexer.next();
    REQUIRE(nullLex.type() == job::json::JsonLexType::Null);
    REQUIRE(nullLex.range().begin() == 13);
    REQUIRE(nullLex.range().end() == 17);
    REQUIRE(nullLex.view(source) == "null");

    REQUIRE(lexer.next().type() == job::json::JsonLexType::End);
}

TEST_CASE("JsonLexer rejects truncated literals", "[job_json][lex][value]")
{
    requireInvalidLex("t");
    requireInvalidLex("tr");
    requireInvalidLex("tru");

    requireInvalidLex("f");
    requireInvalidLex("fa");
    requireInvalidLex("fal");
    requireInvalidLex("fals");

    requireInvalidLex("n");
    requireInvalidLex("nu");
    requireInvalidLex("nul");
}

TEST_CASE("JsonLexer rejects misspelled literals", "[job_json][lex][value]")
{
    requireInvalidLex("True");
    requireInvalidLex("FALSE");
    requireInvalidLex("Null");

    requireInvalidLex("treu");
    requireInvalidLex("flase");
    requireInvalidLex("nill");
}

TEST_CASE("JsonLexer lexes zero", "[job_json][lex][value]")
{
    requireLex("0", job::json::JsonLexType::Number, "0");
}

TEST_CASE("JsonLexer lexes negative zero", "[job_json][lex][value]")
{
    requireLex("-0", job::json::JsonLexType::Number, "-0");
}

TEST_CASE("JsonLexer lexes positive integer forms", "[job_json][lex][value]")
{
    requireLex("1", job::json::JsonLexType::Number, "1");
    requireLex("9", job::json::JsonLexType::Number, "9");
    requireLex("10", job::json::JsonLexType::Number, "10");
    requireLex("42", job::json::JsonLexType::Number, "42");
    requireLex("1234567890", job::json::JsonLexType::Number, "1234567890");
}

TEST_CASE("JsonLexer lexes negative integer forms", "[job_json][lex][value]")
{
    requireLex("-1", job::json::JsonLexType::Number, "-1");
    requireLex("-9", job::json::JsonLexType::Number, "-9");
    requireLex("-10", job::json::JsonLexType::Number, "-10");
    requireLex("-42", job::json::JsonLexType::Number, "-42");
    requireLex("-1234567890", job::json::JsonLexType::Number, "-1234567890");
}

TEST_CASE("JsonLexer preserves large integer lexeme without conversion", "[job_json][lex][value]")
{
    constexpr std::string_view source = "18446744073709551615";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::Number);
    REQUIRE(lex.view(source) == source);
    REQUIRE(lex.range().size() == source.size());
}

TEST_CASE("JsonLexer lexes fractional numbers", "[job_json][lex][value]")
{
    requireLex("0.0", job::json::JsonLexType::Number, "0.0");
    requireLex("1.0", job::json::JsonLexType::Number, "1.0");
    requireLex("1.25", job::json::JsonLexType::Number, "1.25");
    requireLex("-1.25", job::json::JsonLexType::Number, "-1.25");
    requireLex("123.456789", job::json::JsonLexType::Number, "123.456789");
}

TEST_CASE("JsonLexer lexes exponent numbers", "[job_json][lex][value]")
{
    requireLex("1e0", job::json::JsonLexType::Number, "1e0");
    requireLex("1E0", job::json::JsonLexType::Number, "1E0");

    requireLex("1e5", job::json::JsonLexType::Number, "1e5");
    requireLex("1E5", job::json::JsonLexType::Number, "1E5");

    requireLex("1e+5", job::json::JsonLexType::Number, "1e+5");
    requireLex("1e-5", job::json::JsonLexType::Number, "1e-5");

    requireLex("1E+5", job::json::JsonLexType::Number, "1E+5");
    requireLex("1E-5", job::json::JsonLexType::Number, "1E-5");
}

TEST_CASE("JsonLexer lexes fractional exponent numbers", "[job_json][lex][value]")
{
    requireLex("1.25e5", job::json::JsonLexType::Number, "1.25e5");
    requireLex("1.25E5", job::json::JsonLexType::Number, "1.25E5");
    requireLex("1.25e+5", job::json::JsonLexType::Number, "1.25e+5");
    requireLex("1.25e-5", job::json::JsonLexType::Number, "1.25e-5");

    requireLex("-0.125e10", job::json::JsonLexType::Number, "-0.125e10");
    requireLex("-123.456E-78", job::json::JsonLexType::Number, "-123.456E-78");
}

TEST_CASE("JsonLexer rejects lone minus", "[job_json][lex][value]")
{
    requireInvalidLex("-");
}

TEST_CASE("JsonLexer rejects leading zero integers", "[job_json][lex][value]")
{
    requireInvalidLex("00");
    requireInvalidLex("01");
    requireInvalidLex("012345");
    requireInvalidLex("-01");
    requireInvalidLex("-012345");
}

TEST_CASE("JsonLexer rejects missing integer before decimal point", "[job_json][lex][value]")
{
    requireInvalidLex(".1");
    requireInvalidLex("-.1");
}

TEST_CASE("JsonLexer rejects fraction without digits after decimal point", "[job_json][lex][value]")
{
    requireInvalidLex("0.");
    requireInvalidLex("1.");
    requireInvalidLex("-1.");
}

TEST_CASE("JsonLexer rejects exponent without digits", "[job_json][lex][value]")
{
    requireInvalidLex("1e");
    requireInvalidLex("1E");

    requireInvalidLex("1e+");
    requireInvalidLex("1e-");

    requireInvalidLex("1E+");
    requireInvalidLex("1E-");
}

TEST_CASE("JsonLexer rejects malformed exponent after fraction", "[job_json][lex][value]")
{
    requireInvalidLex("1.25e");
    requireInvalidLex("1.25e+");
    requireInvalidLex("1.25e-");
}

TEST_CASE("JsonLexer rejects plus sign before number", "[job_json][lex][value]")
{
    requireInvalidLex("+1");
    requireInvalidLex("+0");
    requireInvalidLex("+1.5");
}

TEST_CASE("JsonLexer rejects hexadecimal notation", "[job_json][lex][value]")
{
    requireInvalidLex("0x10");
    requireInvalidLex("-0x10");
}

TEST_CASE("JsonLexer rejects non JSON numeric spellings", "[job_json][lex][value]")
{
    requireInvalidLex("NaN");
    requireInvalidLex("Infinity");
    requireInvalidLex("-Infinity");
}

TEST_CASE("JsonLexer number range is exact after leading whitespace", "[job_json][lex][value]")
{
    constexpr std::string_view source = " \t -123.5e-2";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::Number);
    REQUIRE(lex.range().begin() == 3);
    REQUIRE(lex.range().end() == source.size());
    REQUIRE(lex.view(source) == "-123.5e-2");
}

TEST_CASE("JsonLexer stops number before structural delimiter", "[job_json][lex][value]")
{
    constexpr std::string_view source = "42,";
    job::json::JsonLexer lexer{source};

    const auto number = lexer.next();

    REQUIRE(number.type() == job::json::JsonLexType::Number);
    REQUIRE(number.view(source) == "42");

    const auto comma = lexer.next();

    REQUIRE(comma.type() == job::json::JsonLexType::ValueSeparator);
    REQUIRE(comma.view(source) == ",");

    REQUIRE(lexer.next().type() == job::json::JsonLexType::End);
}

TEST_CASE("JsonLexer stops number before object end", "[job_json][lex][value]")
{
    constexpr std::string_view source = "42}";
    job::json::JsonLexer lexer{source};

    const auto number = lexer.next();

    REQUIRE(number.type() == job::json::JsonLexType::Number);
    REQUIRE(number.view(source) == "42");

    const auto endObject = lexer.next();

    REQUIRE(endObject.type() == job::json::JsonLexType::EndObject);
    REQUIRE(endObject.view(source) == "}");
}

TEST_CASE("JsonLexer stops number before array end", "[job_json][lex][value]")
{
    constexpr std::string_view source = "42]";
    job::json::JsonLexer lexer{source};

    const auto number = lexer.next();

    REQUIRE(number.type() == job::json::JsonLexType::Number);
    REQUIRE(number.view(source) == "42");

    const auto endArray = lexer.next();

    REQUIRE(endArray.type() == job::json::JsonLexType::EndArray);
    REQUIRE(endArray.view(source) == "]");
}

TEST_CASE("JsonLexer stops number before whitespace", "[job_json][lex][value]")
{
    constexpr std::string_view source = "42 \t\r\n]";
    job::json::JsonLexer lexer{source};

    const auto number = lexer.next();

    REQUIRE(number.type() == job::json::JsonLexType::Number);
    REQUIRE(number.view(source) == "42");

    const auto endArray = lexer.next();

    REQUIRE(endArray.type() == job::json::JsonLexType::EndArray);
    REQUIRE(endArray.view(source) == "]");
}

TEST_CASE("JsonLexer tokenizes value sequence without interpreting grammar", "[job_json][lex][value]")
{
    constexpr std::string_view source = "true,false,null,1,-2,3.5,6e7";
    job::json::JsonLexer lexer{source};

    REQUIRE(lexer.next().type() == job::json::JsonLexType::True);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::ValueSeparator);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::False);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::ValueSeparator);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::Null);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::ValueSeparator);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::Number);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::ValueSeparator);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::Number);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::ValueSeparator);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::Number);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::ValueSeparator);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::Number);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::End);
}

