#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
    #include "test_job_json_utils.h"
#endif

#include <string>
#include <string_view>

#include <job_json_number_kernel.h>
#include <job_json_string_kernel.h>

namespace {

job::json::JsonLex lexString(std::string_view source)
{
    job::json::JsonLexer lexer{source};
    return lexer.next();
}

} // namespace

TEST_CASE("JsonStringKernel returns string content without quotation marks", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("cake")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto content = job::json::JsonStringKernel::content(lex, source);

    REQUIRE(content == "cake");
    REQUIRE(content.size() == 4);
}

TEST_CASE("JsonStringKernel handles empty string content", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto content = job::json::JsonStringKernel::content(lex, source);

    REQUIRE(content.empty());
}

TEST_CASE("JsonStringKernel content preserves escaped source text", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("cake\ncourt")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto content = job::json::JsonStringKernel::content(lex, source);

    REQUIRE(lex.hasEscapes());
    REQUIRE(content == R"(cake\ncourt)");
}

TEST_CASE("JsonStringKernel borrowed returns unescaped source view", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("benchmark-compute-node")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto value = job::json::JsonStringKernel::borrowed(lex, source);

    REQUIRE_FALSE(lex.hasEscapes());
    REQUIRE(value == "benchmark-compute-node");
}

TEST_CASE("JsonStringKernel borrowed view aliases original source", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("borrowed-cake")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto value = job::json::JsonStringKernel::borrowed(lex, source);

    REQUIRE(value.data() == source.data() + 1);
    REQUIRE(value.size() == source.size() - 2);
}

TEST_CASE("JsonStringKernel borrowed handles empty string", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto value = job::json::JsonStringKernel::borrowed(lex, source);

    REQUIRE(value.empty());
    REQUIRE(value.data() == source.data() + 1);
}

TEST_CASE("JsonStringKernel borrowed preserves raw UTF8 bytes", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("Grüße 世界 😀")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto value = job::json::JsonStringKernel::borrowed(lex, source);

    REQUIRE_FALSE(lex.hasEscapes());
    REQUIRE(value == "Grüße 世界 😀");
    REQUIRE(value.data() == source.data() + 1);
}

TEST_CASE("JsonStringKernel decodes simple escaped string", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("cake\ncourt")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.hasEscapes());

    std::string value;

    REQUIRE(job::json::JsonStringKernel::decode(lex, source, value));
    REQUIRE(value == "cake\ncourt");
}

TEST_CASE("JsonStringKernel decodes unicode escaped string", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("cake\u0020court\u0021")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.hasEscapes());

    std::string value;

    REQUIRE(job::json::JsonStringKernel::decode(lex, source, value));
    REQUIRE(value == "cake court!");
}

TEST_CASE("JsonStringKernel decodes surrogate pair", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("\uD83D\uDE00")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    REQUIRE(lex.hasEscapes());

    std::string value;

    REQUIRE(job::json::JsonStringKernel::decode(lex, source, value));

    const std::string expected{"\xF0\x9F\x98\x80", 4};

    REQUIRE(value == expected);
}

TEST_CASE("JsonStringKernel decode preserves embedded null", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"("\u0000")";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();

    std::string value;

    REQUIRE(job::json::JsonStringKernel::decode(lex, source, value));
    REQUIRE(value.size() == 1);
    REQUIRE(value[0] == '\0');
}

TEST_CASE("JsonStringKernel content preserves exact interior range", "[job_json][string_kernel]")
{
    constexpr std::string_view source = R"(  "cake"  )";
    job::json::JsonLexer lexer{source};

    const auto lex = lexer.next();
    const auto content = job::json::JsonStringKernel::content(lex, source);

    REQUIRE(lex.range().begin() == 2);
    REQUIRE(lex.range().end() == 8);

    REQUIRE(content == "cake");
    REQUIRE(content.data() == source.data() + 3);
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JsonStringKernel benchmarks", "[job_json][string_kernel][benchmark]")
{
    constexpr std::string_view plainSource = R"("benchmark-compute-node")";
    const auto plainLex = lexString(plainSource);

    constexpr std::string_view escapedSource = R"("benchmark\ncompute\u002dnode")";
    const auto escapedLex = lexString(escapedSource);

    constexpr std::string_view floatSource = "123.456";

    BENCHMARK("JsonStringKernel borrowed string_view")
    {
        job::json::tests::benchmarkDoNotOptimize(plainSource);
        job::json::tests::benchmarkDoNotOptimize(plainLex);

        const auto value = job::json::JsonStringKernel::borrowed(plainLex, plainSource);

        job::json::tests::benchmarkDoNotOptimize(value);

        return value.size();
    };

    BENCHMARK("JsonNumberKernel float from string_view")
    {
        const char *text = "123.456";
        job::json::tests::benchmarkDoNotOptimize(text);

        const std::string_view valueView{text};
        job::json::tests::benchmarkDoNotOptimize(valueView);

        float value{};
        const bool ok = job::json::JsonNumberKernel::parse(valueView, value);

        job::json::tests::benchmarkDoNotOptimize(ok);
        job::json::tests::benchmarkDoNotOptimize(value);

        return value;
    };

    BENCHMARK("JsonNumberKernel float from bounded string_view")
    {
        const char *text = "123.456";
        job::json::tests::benchmarkDoNotOptimize(text);

        const std::string_view valueView{text, 7};
        job::json::tests::benchmarkDoNotOptimize(valueView);

        float value{};
        const bool ok = job::json::JsonNumberKernel::parse(valueView, value);

        job::json::tests::benchmarkDoNotOptimize(ok);
        job::json::tests::benchmarkDoNotOptimize(value);

        return value;
    };


    BENCHMARK("JsonStringKernel escaped decode")
    {
        job::json::tests::benchmarkDoNotOptimize(escapedSource);
        job::json::tests::benchmarkDoNotOptimize(escapedLex);

        std::string value;
        const bool ok = job::json::JsonStringKernel::decode(escapedLex, escapedSource, value);

        job::json::tests::benchmarkDoNotOptimize(ok);
        job::json::tests::benchmarkDoNotOptimize(value);

        return value.size();
    };
}

#endif
