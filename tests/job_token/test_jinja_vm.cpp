#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "template/jinja_lexer.h"
#include "template/jinja_parser.h"
#include "template/jinja_vm.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("JinjaVM evaluates expressions and built-in filters", "[token][jinja][vm][example]")
{
    std::string_view source = "{{ text | trim | lower }} - {{ count + 1 }}";

    JinjaLexer lexer(source);
    auto tokens = lexer.tokenizeAll();
    JinjaParser parser(tokens);
    auto ast = parser.parse();
    REQUIRE(ast != nullptr);

    ValueMap ctx;
    ctx["text"] = Value("  HELLO WORLD  ");
    ctx["count"] = Value(41);

    JinjaVM vm;
    std::string result = vm.execute(*ast, ctx);

    CHECK(result == "hello world - 42");
}

TEST_CASE("JinjaVM manages loop variables: index, first, last", "[token][jinja][vm][loop_context]")
{
    std::string_view source =
        "{% for item in items %}"
        "{{ loop.index }}:{{ item }}{% if not loop.last %},{% endif %}"
        "{% endfor %}";

    JinjaLexer lexer(source);
    auto tokens = lexer.tokenizeAll();
    JinjaParser parser(tokens);
    auto ast = parser.parse();
    REQUIRE(ast != nullptr);

    ValueMap ctx;
    ctx["items"] = Value(ValueList{Value("A"), Value("B"), Value("C")});

    JinjaVM vm;
    std::string result = vm.execute(*ast, ctx);

    CHECK(result == "1:A,2:B,3:C");
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("JinjaVM handles null values, undefined variables, and short-circuit logic", "[token][jinja][vm][edge_cases]")
{
    JinjaVM vm;

    SECTION("Undefined variable evaluates to empty string in output") {
        std::string_view source = "Val: [{{ nonexistent }}]";
        JinjaLexer lexer(source);
        auto ast = JinjaParser(lexer.tokenizeAll()).parse();

        CHECK(vm.execute(*ast, ValueMap{}) == "Val: []");
    }

    SECTION("Empty list executes for-else branch") {
        std::string_view source = "{% for x in empty_list %}ITEM{% else %}EMPTY{% endfor %}";
        JinjaLexer lexer(source);
        auto ast = JinjaParser(lexer.tokenizeAll()).parse();

        ValueMap ctx;
        ctx["empty_list"] = Value(ValueList{});

        CHECK(vm.execute(*ast, ctx) == "EMPTY");
    }

    SECTION("Short-circuit boolean evaluation") {
        std::string_view source = "{% if false and nonexistent.field %}YES{% else %}NO{% endif %}";
        JinjaLexer lexer(source);
        auto ast = JinjaParser(lexer.tokenizeAll()).parse();

        CHECK(vm.execute(*ast, ValueMap{}) == "NO");
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark JinjaVM evaluation performance", "[token][jinja][vm][benchmark]")
{
    std::string_view source =
        "{% for msg in messages %}"
        "<|im_start|>{{ msg.role }}\n{{ msg.content }}<|im_end|>\n"
        "{% endfor %}"
        "{% if add_generation_prompt %}<|im_start|>assistant\n{% endif %}";

    JinjaLexer lexer(source);
    auto ast = JinjaParser(lexer.tokenizeAll()).parse();

    ValueList msgs;
    for (int i = 0; i < 20; ++i) {
        ValueMap m;
        m["role"] = Value((i % 2 == 0) ? "user" : "assistant");
        m["content"] = Value("Benchmarking message contents for iteration " + std::to_string(i));
        msgs.emplace_back(std::move(m));
    }

    ValueMap ctx;
    ctx["messages"] = Value(std::move(msgs));
    ctx["add_generation_prompt"] = Value(true);

    JinjaVM vm;

    BENCHMARK("Evaluate 20-message ChatML prompt in VM") {
        return vm.execute(*ast, ctx);
    };
}
#endif

} // namespace job::token::test