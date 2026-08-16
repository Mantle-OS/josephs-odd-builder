#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include <vector>

#include "template/jinja_lexer.h"
#include "template/jinja_token.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("JinjaLexer tokenizes expressions, blocks, and literals", "[token][jinja][lexer][example]")
{
    // Real-world Jinja chat template snippet
    std::string_view source =
        "{{ bos_token }}"
        "{% for message in messages %}"
        "{{ message['role'] + ': ' + message['content'] }}"
        "{% endfor %}";

    JinjaLexer lexer(source);
    std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    // Verify token sequence
    REQUIRE(!tokens.empty());
    CHECK(tokens[0].is(JinjaTokenType::ExprBegin));
    CHECK(tokens[1].is(JinjaTokenType::Identifier));
    CHECK(tokens[1].text == "bos_token");
    CHECK(tokens[2].is(JinjaTokenType::ExprEnd));

    CHECK(tokens[3].is(JinjaTokenType::BlockBegin));
    CHECK(tokens[4].is(JinjaTokenType::KwFor));
    CHECK(tokens[5].is(JinjaTokenType::Identifier));
    CHECK(tokens[5].text == "message");
    CHECK(tokens[6].is(JinjaTokenType::KwIn));
    CHECK(tokens[7].is(JinjaTokenType::Identifier));
    CHECK(tokens[7].text == "messages");
    CHECK(tokens[8].is(JinjaTokenType::BlockEnd));

    CHECK(tokens.back().is(JinjaTokenType::Eof));
}

TEST_CASE("JinjaLexer handles whitespace trimming delimiters", "[token][jinja][lexer][trim]")
{
    // Delimiters with '-' request stripping outer whitespace
    std::string_view source = "Leading   {{- 'trimmed' -}}   Trailing";

    JinjaLexer lexer(source);
    auto tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() >= 5);
    CHECK(tokens[0].is(JinjaTokenType::Text));
    CHECK(tokens[0].text == "Leading"); // Trailing whitespace stripped before {{-

    CHECK(tokens[1].is(JinjaTokenType::ExprBegin));
    CHECK(tokens[2].is(JinjaTokenType::StringLiteral));
    CHECK(tokens[3].is(JinjaTokenType::ExprEnd));

    CHECK(tokens[4].is(JinjaTokenType::Text));
    CHECK(tokens[4].text == "Trailing"); // Leading whitespace stripped after -}}
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("JinjaLexer handles empty, whitespace-only, and malformed inputs", "[token][jinja][lexer][edge_cases]")
{
    SECTION("Empty source produces single EOF token") {
        JinjaLexer lexer("");
        auto tokens = lexer.tokenizeAll();
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].is(JinjaTokenType::Eof));
    }

    SECTION("Comments are stripped out entirely") {
        std::string_view source = "Text1{# this is a comment #}Text2";
        JinjaLexer lexer(source);
        auto tokens = lexer.tokenizeAll();

        REQUIRE(tokens.size() >= 3);
        CHECK(tokens[0].text == "Text1");
        CHECK(tokens[1].text == "Text2");
        CHECK(tokens[2].is(JinjaTokenType::Eof));
    }

    SECTION("Numbers and float parsing in tags") {
        std::string_view source = "{{ 42 + 3.14159 }}";
        JinjaLexer lexer(source);
        auto tokens = lexer.tokenizeAll();

        REQUIRE(tokens.size() >= 6);
        CHECK(tokens[1].is(JinjaTokenType::NumberLiteral));
        CHECK(tokens[1].text == "42");
        CHECK(tokens[2].is(JinjaTokenType::Plus));
        CHECK(tokens[3].is(JinjaTokenType::NumberLiteral));
        CHECK(tokens[3].text == "3.14159");
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark JinjaLexer tokenization throughput", "[token][jinja][lexer][benchmark]")
{
    std::string largeTemplate;
    for (int i = 0; i < 50; ++i) {
        largeTemplate += "{% for msg in messages %}{{ msg.role }}: {{ msg.content }}{% endfor %}\n";
    }

    BENCHMARK("Tokenize 50-turn Jinja chat template") {
        JinjaLexer lexer(largeTemplate);
        return lexer.tokenizeAll();
    };
}
#endif

} // namespace job::token::test