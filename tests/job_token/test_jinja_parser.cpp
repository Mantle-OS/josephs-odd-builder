#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "template/jinja_ast.h"
#include "template/jinja_lexer.h"
#include "template/jinja_parser.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("JinjaParser constructs AST for if-elif-else statements", "[token][jinja][parser][example]")
{
    std::string_view source =
        "{% if role == 'system' %}"
        "SYSTEM"
        "{% elif role == 'user' %}"
        "USER"
        "{% else %}"
        "ASSISTANT"
        "{% endif %}";

    JinjaLexer lexer(source);
    auto tokens = lexer.tokenizeAll();

    JinjaParser parser(tokens);
    auto root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->statements.size() == 1);
    CHECK(root->statements[0]->type == ast::NodeType::If);

    auto* ifNode = static_cast<const ast::IfNode*>(root->statements[0].get());
    REQUIRE(ifNode->branches.size() == 3);

    // Branch 0: If
    CHECK(ifNode->branches[0].condition != nullptr);
    CHECK(ifNode->branches[0].condition->type == ast::NodeType::BinaryOp);
    REQUIRE(ifNode->branches[0].body != nullptr);
    CHECK(ifNode->branches[0].body->statements.size() == 1);

    // Branch 1: Elif
    CHECK(ifNode->branches[1].condition != nullptr);

    // Branch 2: Else (null condition)
    CHECK(ifNode->branches[2].condition == nullptr);
    REQUIRE(ifNode->branches[2].body != nullptr);
}

TEST_CASE("JinjaParser constructs AST for filters and operator precedence", "[token][jinja][parser][precedence]")
{
    std::string_view source = "{{ value | trim | upper }}";

    JinjaLexer lexer(source);
    auto tokens = lexer.tokenizeAll();

    JinjaParser parser(tokens);
    auto root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->statements.size() == 1);
    CHECK(root->statements[0]->type == ast::NodeType::Output);

    auto* outNode = static_cast<const ast::OutputNode*>(root->statements[0].get());
    REQUIRE(outNode->expression != nullptr);
    CHECK(outNode->expression->type == ast::NodeType::Filter);

    auto* filterUpper = static_cast<const ast::FilterNode*>(outNode->expression.get());
    CHECK(filterUpper->filter_name == "upper");
    REQUIRE(filterUpper->target != nullptr);
    CHECK(filterUpper->target->type == ast::NodeType::Filter);

    auto* filterTrim = static_cast<const ast::FilterNode*>(filterUpper->target.get());
    CHECK(filterTrim->filter_name == "trim");
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("JinjaParser catches syntax errors and missing closing tags", "[token][jinja][parser][edge_cases]")
{
    SECTION("Unclosed if statement throws ParseError") {
        std::string_view source = "{% if condition %} No endif here!";
        JinjaLexer lexer(source);
        auto tokens = lexer.tokenizeAll();

        JinjaParser parser(tokens);
        CHECK_THROWS_AS(parser.parse(), ParseError);
    }

    SECTION("Unclosed expression tag throws ParseError") {
        std::string_view source = "{{ open_variable ";
        JinjaLexer lexer(source);
        auto tokens = lexer.tokenizeAll();

        JinjaParser parser(tokens);
        CHECK_THROWS_AS(parser.parse(), ParseError);
    }

    SECTION("Empty input produces empty block without error") {
        JinjaParser parser(std::vector<JinjaToken>{});
        auto root = parser.parse();
        REQUIRE(root != nullptr);
        CHECK(root->statements.empty());
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark JinjaParser AST building latency", "[token][jinja][parser][benchmark]")
{
    std::string tmpl =
        "{% for msg in messages %}"
        "{% if msg.role == 'system' %}[SYS]{{ msg.content }}[/SYS]"
        "{% elif msg.role == 'user' %}[USER]{{ msg.content }}[/USER]"
        "{% else %}[AI]{{ msg.content }}[/AI]{% endif %}"
        "{% endfor %}";

    JinjaLexer lexer(tmpl);
    auto tokens = lexer.tokenizeAll();

    BENCHMARK("Parse token stream to AST") {
        JinjaParser parser(tokens);
        return parser.parse();
    };
}
#endif

} // namespace job::token::test