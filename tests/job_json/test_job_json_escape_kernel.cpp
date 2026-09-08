#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_escape_kernel.h>

namespace {

void requireDecode(std::string_view input, std::string_view expected)
{
    std::string result;

    REQUIRE(job::json::JsonEscapeKernel::decode(input, result));
    REQUIRE(result.size() == expected.size());
    REQUIRE(result == expected);
}

void requireDecodeFailure(std::string_view input, std::string initial)
{
    std::string result = initial;

    REQUIRE_FALSE(job::json::JsonEscapeKernel::decode(input, result));
    REQUIRE(result == initial);
}

} // namespace

TEST_CASE("JsonEscapeKernel passes through plain text", "[job_json][escape_kernel]")
{
    requireDecode("", "");
    requireDecode("cake", "cake");
    requireDecode("the cake is real", "the cake is real");
}

TEST_CASE("JsonEscapeKernel decodes simple escapes", "[job_json][escape_kernel]")
{
    requireDecode(R"(\")", "\"");
    requireDecode(R"(\\)", "\\");
    requireDecode(R"(\/)", "/");
    requireDecode(R"(\b)", std::string_view{"\b", 1});
    requireDecode(R"(\f)", std::string_view{"\f", 1});
    requireDecode(R"(\n)", std::string_view{"\n", 1});
    requireDecode(R"(\r)", std::string_view{"\r", 1});
    requireDecode(R"(\t)", std::string_view{"\t", 1});
}

TEST_CASE("JsonEscapeKernel decodes mixed simple escapes", "[job_json][escape_kernel]")
{
    const std::string expected = "line1\nline2\t\"cake\"\\done";

    requireDecode(R"(line1\nline2\t\"cake\"\\done)", expected);
}

TEST_CASE("JsonEscapeKernel decodes unicode ASCII escape", "[job_json][escape_kernel]")
{
    requireDecode(R"(\u0041)", "A");
    requireDecode(R"(\u0061)", "a");
    requireDecode(R"(\u0030)", "0");
}

TEST_CASE("JsonEscapeKernel decodes unicode null byte", "[job_json][escape_kernel]")
{
    std::string result;

    REQUIRE(job::json::JsonEscapeKernel::decode(R"(\u0000)", result));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == '\0');
}

TEST_CASE("JsonEscapeKernel decodes two byte UTF8 codepoint", "[job_json][escape_kernel]")
{
    const std::string expected{"\xC2\xA2", 2};

    requireDecode(R"(\u00A2)", expected);
}

TEST_CASE("JsonEscapeKernel decodes three byte UTF8 codepoint", "[job_json][escape_kernel]")
{
    const std::string expected{"\xE2\x82\xAC", 3};

    requireDecode(R"(\u20AC)", expected);
}

TEST_CASE("JsonEscapeKernel decodes four byte UTF8 surrogate pair", "[job_json][escape_kernel]")
{
    const std::string expected{"\xF0\x9F\x98\x80", 4};

    requireDecode(R"(\uD83D\uDE00)", expected);
}

TEST_CASE("JsonEscapeKernel decodes maximum unicode scalar", "[job_json][escape_kernel]")
{
    const std::string expected{"\xF4\x8F\xBF\xBF", 4};

    requireDecode(R"(\uDBFF\uDFFF)", expected);
}

TEST_CASE("JsonEscapeKernel accepts lowercase hex digits", "[job_json][escape_kernel]")
{
    const std::string expected{"\xC2\xAF", 2};

    requireDecode(R"(\u00af)", expected);
}

TEST_CASE("JsonEscapeKernel accepts uppercase hex digits", "[job_json][escape_kernel]")
{
    const std::string expected{"\xC2\xAF", 2};

    requireDecode(R"(\u00AF)", expected);
}

TEST_CASE("JsonEscapeKernel accepts mixed hex digits", "[job_json][escape_kernel]")
{
    const std::string expected{"\xEA\xAB\xB0", 3};

    requireDecode(R"(\uAAF0)", expected);
}

TEST_CASE("JsonEscapeKernel decodes multiple unicode escapes", "[job_json][escape_kernel]")
{
    requireDecode(R"(\u004a\u0053\u004f\u004e)", "JSON");
}

TEST_CASE("JsonEscapeKernel decodes mixed raw and escaped content", "[job_json][escape_kernel]")
{
    requireDecode(R"(cake\ncourt\u0021)", "cake\ncourt!");
}

TEST_CASE("JsonEscapeKernel preserves raw UTF8 bytes", "[job_json][escape_kernel]")
{
    constexpr std::string_view input = "Grüße 世界 😀";

    requireDecode(input, input);
}

TEST_CASE("JsonEscapeKernel rejects lone high surrogate", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\uD83D)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects lone low surrogate", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\uDE00)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects high surrogate followed by non low surrogate", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\uD83D\u0041)", "unchanged");
    requireDecodeFailure(R"(\uD83D\uD83D)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects truncated surrogate pair", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\uD83D\u)", "unchanged");
    requireDecodeFailure(R"(\uD83D\u0)", "unchanged");
    requireDecodeFailure(R"(\uD83D\u00)", "unchanged");
    requireDecodeFailure(R"(\uD83D\u000)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects high surrogate without second unicode escape", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\uD83Dx)", "unchanged");
    requireDecodeFailure(R"(\uD83D\n)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects short unicode escape", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\u)", "unchanged");
    requireDecodeFailure(R"(\u0)", "unchanged");
    requireDecodeFailure(R"(\u00)", "unchanged");
    requireDecodeFailure(R"(\u000)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects invalid unicode hex digit", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\uZZZZ)", "unchanged");
    requireDecodeFailure(R"(\u12G4)", "unchanged");
    requireDecodeFailure(R"(\u000g)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects unknown escape", "[job_json][escape_kernel]")
{
    requireDecodeFailure(R"(\x41)", "unchanged");
    requireDecodeFailure(R"(\')", "unchanged");
    requireDecodeFailure(R"(\a)", "unchanged");
}

TEST_CASE("JsonEscapeKernel rejects trailing backslash", "[job_json][escape_kernel]")
{
    requireDecodeFailure("cake\\", "unchanged");
}

TEST_CASE("JsonEscapeKernel leaves destination unchanged after partial decode failure", "[job_json][escape_kernel]")
{
    std::string result = "sentinel";

    REQUIRE_FALSE(job::json::JsonEscapeKernel::decode(R"(cake\ncourt\x)", result));
    REQUIRE(result == "sentinel");
}

TEST_CASE("JsonEscapeKernel handles adjacent surrogate pairs", "[job_json][escape_kernel]")
{
    const std::string expected{
        "\xF0\x9F\x98\x80"
        "\xF0\x9F\x98\x81",
        8
    };

    requireDecode(R"(\uD83D\uDE00\uD83D\uDE01)", expected);
}

TEST_CASE("JsonEscapeKernel handles escaped slash and unicode together", "[job_json][escape_kernel]")
{
    requireDecode(R"(https:\/\/example.com\/\u0063ake)", "https://example.com/cake");
}

