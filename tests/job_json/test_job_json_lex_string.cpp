#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_lex.h>

namespace {

void requireStringLex(
    std::string_view source,
    bool expectedEscapes,
    std::string_view expectedToken)
{
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::String);
    REQUIRE(lex.valid());
    REQUIRE_FALSE(lex.isEnd());
    REQUIRE(lex.hasEscapes() == expectedEscapes);
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

TEST_CASE("JsonLexer lexes empty string", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("")";

    requireStringLex(source, false, source);
}

TEST_CASE("JsonLexer lexes plain string", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("cake")";

    requireStringLex(source, false, source);
}

TEST_CASE("JsonLexer keeps spaces inside strings", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("the cake is real")";

    requireStringLex(source, false, source);
}

TEST_CASE("JsonLexer string range includes quotation marks", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("json")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::String);
    REQUIRE(lex.range().begin() == 0);
    REQUIRE(lex.range().end() == source.size());
    REQUIRE(lex.range().size() == source.size());
    REQUIRE(lex.view(source) == source);
}

TEST_CASE("JsonLexer lexes string after leading whitespace", "[job_json][lex][string]")
{
    constexpr std::string_view source = " \t\r\n\"cake\"";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::String);
    REQUIRE(lex.range().begin() == 4);
    REQUIRE(lex.range().end() == source.size());
    REQUIRE(lex.view(source) == "\"cake\"");
    REQUIRE_FALSE(lex.hasEscapes());
}

TEST_CASE("JsonLexer preserves raw UTF-8 string bytes", "[job_json][lex][string]")
{
    // constexpr std::string_view source = u8R"("Grüße 世界 😀")";
    constexpr std::string_view source = R"("Grüße 世界 😀")";
    requireStringLex(source, false, source);
}

TEST_CASE("JsonLexer marks escaped quotation mark", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("say \"hello\"")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer escaped quotation mark does not terminate string", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("a\"b")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::String);
    REQUIRE(lex.hasEscapes());
    REQUIRE(lex.view(source) == source);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::End);
}

TEST_CASE("JsonLexer accepts all simple JSON escapes", "[job_json][lex][string]")
{
    requireStringLex(R"("\"")", true, R"("\"")");
    requireStringLex(R"("\\")", true, R"("\\")");
    requireStringLex(R"("\/")", true, R"("\/")");
    requireStringLex(R"("\b")", true, R"("\b")");
    requireStringLex(R"("\f")", true, R"("\f")");
    requireStringLex(R"("\n")", true, R"("\n")");
    requireStringLex(R"("\r")", true, R"("\r")");
    requireStringLex(R"("\t")", true, R"("\t")");
}

TEST_CASE("JsonLexer accepts unicode escape", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\u0041")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer accepts lowercase unicode hex digits", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\u00af")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer accepts uppercase unicode hex digits", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\u00AF")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer accepts mixed unicode hex digits", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\uAaF0")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer accepts multiple unicode escapes", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\u004a\u0053\u004f\u004e")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer lexes surrogate escape code units without interpreting them", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\uD83D\uDE00")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer accepts lone surrogate escape lexically", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\uD83D")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer accepts mixed plain and escaped content", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("cake\ncourt\u0021")";

    requireStringLex(source, true, source);
}

TEST_CASE("JsonLexer does not mark plain backslash-free string as escaped", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("abcdef0123456789 !#$%&'()*+,-./:;<=>?@[]^_`{|}~")";

    requireStringLex(source, false, source);
}

TEST_CASE("JsonLexer rejects unterminated string", "[job_json][lex][string]")
{
    constexpr std::string_view source = "\"cake";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects string containing only opening quote", "[job_json][lex][string]")
{
    constexpr std::string_view source = "\"";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects backslash at end of source", "[job_json][lex][string]")
{
    constexpr std::string_view source = "\"cake\\";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects unknown escape", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\x41")";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects escaped single quote", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("\'")";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects short unicode escape", "[job_json][lex][string]")
{
    requireInvalidLex(R"("\u")");
    requireInvalidLex(R"("\u0")");
    requireInvalidLex(R"("\u00")");
    requireInvalidLex(R"("\u000")");
}

TEST_CASE("JsonLexer rejects non-hex unicode escape", "[job_json][lex][string]")
{
    requireInvalidLex(R"("\uZZZZ")");
    requireInvalidLex(R"("\u12G4")");
    requireInvalidLex(R"("\u000g")");
}

TEST_CASE("JsonLexer rejects unescaped control characters", "[job_json][lex][string]")
{
    const std::string_view nullChar{"\"\0\"", 3};
    const std::string_view unitSeparator{"\"\x1f\"", 3};

    requireInvalidLex(nullChar);
    requireInvalidLex(unitSeparator);
}

TEST_CASE("JsonLexer rejects unescaped newline in string", "[job_json][lex][string]")
{
    constexpr std::string_view source = "\"line\nbreak\"";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects unescaped carriage return in string", "[job_json][lex][string]")
{
    constexpr std::string_view source = "\"line\rbreak\"";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer rejects unescaped tab in string", "[job_json][lex][string]")
{
    constexpr std::string_view source = "\"line\tbreak\"";

    requireInvalidLex(source);
}

TEST_CASE("JsonLexer terminates at first unescaped quotation mark", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("cake""court")";
    job::json::JsonLexer lexer{source};

    const auto first = lexer.next();

    REQUIRE(first.type() == job::json::JsonLexType::String);
    REQUIRE(first.view(source) == R"("cake")");
    REQUIRE_FALSE(first.hasEscapes());

    const auto second = lexer.next();

    REQUIRE(second.type() == job::json::JsonLexType::String);
    REQUIRE(second.view(source) == R"("court")");
    REQUIRE_FALSE(second.hasEscapes());

    REQUIRE(lexer.next().type() == job::json::JsonLexType::End);
}

TEST_CASE("JsonLexer preserves escaped string source range exactly", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"(  "a\n\u0042"  )";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::String);
    REQUIRE(lex.hasEscapes());
    REQUIRE(lex.range().begin() == 2);
    REQUIRE(lex.range().end() == 13);
    REQUIRE(lex.view(source) == R"("a\n\u0042")");
}

TEST_CASE("JsonLexer string token remains borrowed from original source", "[job_json][lex][string]")
{
    constexpr std::string_view source = R"("borrowed")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto view = lex.view(source);

    REQUIRE(view.data() == source.data());
    REQUIRE(view.size() == source.size());
}
