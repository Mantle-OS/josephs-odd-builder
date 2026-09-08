#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_lex.h>

namespace {

void requireLex(
    const job::json::JsonLex &lex,
    job::json::JsonLexType type,
    std::string_view source,
    std::string_view expected)
{
    REQUIRE(lex.type() == type);
    REQUIRE(lex.view(source) == expected);
}

} // namespace

TEST_CASE("JsonLex default constructs invalid", "[job_json][lex][structure]")
{
    constexpr job::json::JsonLex lex;

    STATIC_REQUIRE(lex.type() == job::json::JsonLexType::Invalid);
    STATIC_REQUIRE_FALSE(lex.valid());
    STATIC_REQUIRE_FALSE(lex.isEnd());
    STATIC_REQUIRE_FALSE(lex.hasEscapes());

    STATIC_REQUIRE(lex.range().begin() == 0);
    STATIC_REQUIRE(lex.range().end() == 0);
    STATIC_REQUIRE(lex.range().empty());
}

TEST_CASE("JsonLex stores type range and escape state", "[job_json][lex][structure]")
{
    constexpr job::json::JsonLex lex{
        job::json::JsonLexType::String,
        job::json::JsonSourceRange{2, 8},
        true
    };

    STATIC_REQUIRE(lex.type() == job::json::JsonLexType::String);
    STATIC_REQUIRE(lex.valid());
    STATIC_REQUIRE_FALSE(lex.isEnd());
    STATIC_REQUIRE(lex.hasEscapes());

    STATIC_REQUIRE(lex.range().begin() == 2);
    STATIC_REQUIRE(lex.range().end() == 8);
    STATIC_REQUIRE(lex.range().size() == 6);
}

TEST_CASE("JsonLex recognizes end token separately from invalid", "[job_json][lex][structure]")
{
    constexpr job::json::JsonLex lex{
        job::json::JsonLexType::End,
        job::json::JsonSourceRange{4, 4}
    };

    STATIC_REQUIRE(lex.valid());
    STATIC_REQUIRE(lex.isEnd());
    STATIC_REQUIRE_FALSE(lex.hasEscapes());
}

TEST_CASE("JsonLexer emits object punctuation", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "{}:,";
    job::json::JsonLexer lexer{source};

    requireLex(lexer.next(), job::json::JsonLexType::BeginObject, source, "{");
    requireLex(lexer.next(), job::json::JsonLexType::EndObject, source, "}");
    requireLex(lexer.next(), job::json::JsonLexType::NameSeparator, source, ":");
    requireLex(lexer.next(), job::json::JsonLexType::ValueSeparator, source, ",");

    const auto end = lexer.next();

    REQUIRE(end.type() == job::json::JsonLexType::End);
    REQUIRE(end.isEnd());
    REQUIRE(end.range().begin() == source.size());
    REQUIRE(end.range().end() == source.size());
    REQUIRE(end.view(source).empty());
}

TEST_CASE("JsonLexer emits array punctuation", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "[],";
    job::json::JsonLexer lexer{source};

    requireLex(lexer.next(), job::json::JsonLexType::BeginArray, source, "[");
    requireLex(lexer.next(), job::json::JsonLexType::EndArray, source, "]");
    requireLex(lexer.next(), job::json::JsonLexType::ValueSeparator, source, ",");
    requireLex(lexer.next(), job::json::JsonLexType::End, source, "");
}

TEST_CASE("JsonLexer skips RFC JSON whitespace", "[job_json][lex][structure]")
{
    constexpr std::string_view source = " \t\r\n{\n\t }\r ";
    job::json::JsonLexer lexer{source};

    const auto begin = lexer.next();

    REQUIRE(begin.type() == job::json::JsonLexType::BeginObject);
    REQUIRE(begin.range().begin() == 4);
    REQUIRE(begin.range().end() == 5);
    REQUIRE(begin.view(source) == "{");

    const auto endObject = lexer.next();

    REQUIRE(endObject.type() == job::json::JsonLexType::EndObject);
    REQUIRE(endObject.view(source) == "}");

    const auto end = lexer.next();

    REQUIRE(end.type() == job::json::JsonLexType::End);
    REQUIRE(end.range().begin() == source.size());
    REQUIRE(end.range().end() == source.size());
}

TEST_CASE("JsonLexer accepts source containing only whitespace", "[job_json][lex][structure]")
{
    constexpr std::string_view source = " \t\r\n";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::End);
    REQUIRE(lex.isEnd());
    REQUIRE(lex.range().begin() == source.size());
    REQUIRE(lex.range().end() == source.size());
    REQUIRE(lexer.offset() == source.size());
}

TEST_CASE("JsonLexer accepts empty source as end", "[job_json][lex][structure]")
{
    constexpr std::string_view source;
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.type() == job::json::JsonLexType::End);
    REQUIRE(lex.isEnd());
    REQUIRE(lex.range().empty());
    REQUIRE(lex.range().begin() == 0);
    REQUIRE(lex.range().end() == 0);
    REQUIRE(lexer.offset() == 0);
}

TEST_CASE("JsonLexer preserves punctuation source ranges", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "{ [ ] : , }";
    job::json::JsonLexer lexer{source};

    const auto beginObject = lexer.next();
    REQUIRE(beginObject.type() == job::json::JsonLexType::BeginObject);
    REQUIRE(beginObject.range().begin() == 0);
    REQUIRE(beginObject.range().end() == 1);

    const auto beginArray = lexer.next();
    REQUIRE(beginArray.type() == job::json::JsonLexType::BeginArray);
    REQUIRE(beginArray.range().begin() == 2);
    REQUIRE(beginArray.range().end() == 3);

    const auto endArray = lexer.next();
    REQUIRE(endArray.type() == job::json::JsonLexType::EndArray);
    REQUIRE(endArray.range().begin() == 4);
    REQUIRE(endArray.range().end() == 5);

    const auto nameSeparator = lexer.next();
    REQUIRE(nameSeparator.type() == job::json::JsonLexType::NameSeparator);
    REQUIRE(nameSeparator.range().begin() == 6);
    REQUIRE(nameSeparator.range().end() == 7);

    const auto valueSeparator = lexer.next();
    REQUIRE(valueSeparator.type() == job::json::JsonLexType::ValueSeparator);
    REQUIRE(valueSeparator.range().begin() == 8);
    REQUIRE(valueSeparator.range().end() == 9);

    const auto endObject = lexer.next();
    REQUIRE(endObject.type() == job::json::JsonLexType::EndObject);
    REQUIRE(endObject.range().begin() == 10);
    REQUIRE(endObject.range().end() == 11);
}

TEST_CASE("JsonLexer emits invalid token for unknown structural byte", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "@";
    job::json::JsonLexer lexer{source};

    const auto invalid = lexer.next();

    REQUIRE(invalid.type() == job::json::JsonLexType::Invalid);
    REQUIRE_FALSE(invalid.valid());
    REQUIRE_FALSE(invalid.isEnd());
    REQUIRE(invalid.range().begin() == 0);
    REQUIRE(invalid.range().end() == 1);
    REQUIRE(invalid.view(source) == "@");
    REQUIRE(lexer.offset() == 1);

    const auto end = lexer.next();

    REQUIRE(end.type() == job::json::JsonLexType::End);
    REQUIRE(end.isEnd());
}

TEST_CASE("JsonLexer advances past consecutive invalid bytes", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "@#$";
    job::json::JsonLexer lexer{source};

    requireLex(lexer.next(), job::json::JsonLexType::Invalid, source, "@");
    REQUIRE(lexer.offset() == 1);

    requireLex(lexer.next(), job::json::JsonLexType::Invalid, source, "#");
    REQUIRE(lexer.offset() == 2);

    requireLex(lexer.next(), job::json::JsonLexType::Invalid, source, "$");
    REQUIRE(lexer.offset() == 3);

    requireLex(lexer.next(), job::json::JsonLexType::End, source, "");
}

TEST_CASE("JsonLexer exposes borrowed source", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "{}";
    job::json::JsonLexer lexer{source};

    REQUIRE(lexer.source().data() == source.data());
    REQUIRE(lexer.source().size() == source.size());
}

TEST_CASE("JsonLexer tracks offset across tokens and whitespace", "[job_json][lex][structure]")
{
    constexpr std::string_view source = " { [ ] } ";
    job::json::JsonLexer lexer{source};

    REQUIRE(lexer.offset() == 0);
    REQUIRE(lexer.next().type() == job::json::JsonLexType::BeginObject);
    REQUIRE(lexer.offset() == 2);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::BeginArray);
    REQUIRE(lexer.offset() == 4);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::EndArray);
    REQUIRE(lexer.offset() == 6);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::EndObject);
    REQUIRE(lexer.offset() == 8);

    REQUIRE(lexer.next().type() == job::json::JsonLexType::End);
    REQUIRE(lexer.offset() == source.size());
}

TEST_CASE("JsonLexer tokenizes nested structural sequence", "[job_json][lex][structure]")
{
    constexpr std::string_view source = "{:[,{}]}";
    job::json::JsonLexer lexer{source};
    requireLex(lexer.next(), job::json::JsonLexType::BeginObject, source, "{");
    requireLex(lexer.next(), job::json::JsonLexType::NameSeparator, source, ":");
    requireLex(lexer.next(), job::json::JsonLexType::BeginArray, source, "[");
    requireLex(lexer.next(), job::json::JsonLexType::ValueSeparator, source, ",");
    requireLex(lexer.next(), job::json::JsonLexType::BeginObject, source, "{");
    requireLex(lexer.next(), job::json::JsonLexType::EndObject, source, "}");
    requireLex(lexer.next(), job::json::JsonLexType::EndArray, source, "]");
    requireLex(lexer.next(), job::json::JsonLexType::EndObject, source, "}");
    requireLex(lexer.next(), job::json::JsonLexType::End, source, "");
}

