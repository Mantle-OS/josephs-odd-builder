#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include <job_yaml_block_scalar_decoder.h>

#include "test_job_yaml_block_scalar_fixtures_generated.h"

using namespace std::string_view_literals;

namespace job::yaml::tests {

static bool splitBlockScalarSource(std::string_view source,
                                   std::string_view &header,
                                   std::string_view &body) noexcept
{
    const std::size_t lineBreak = source.find('\n');

    if (lineBreak == std::string_view::npos) {
        header = source;
        body = {};
        return true;
    }

    std::size_t headerSize = lineBreak;

    if (headerSize > 0 && source[headerSize - 1] == '\r')
        --headerSize;

    header = source.substr(0, headerSize);
    body = source.substr(lineBreak + 1);
    return true;
}

static void testFixture(const YamlBlockScalarFixture &fixture)
{
    // WARN("fixture: " << fixture.name);
    // WARN("production: " << fixture.production);
    // WARN("source: " << fixture.source);

    std::string_view header;
    std::string_view body;

    REQUIRE(splitBlockScalarSource(fixture.source, header, body));

    YamlDecodedScalar decoded;
    const bool result = YamlBlockScalarDecoder::decode(header,
                                                       body,
                                                       0,
                                                       decoded);

    REQUIRE(result == fixture.valid);

    if (!fixture.valid)
        return;

    REQUIRE(decoded.value() == fixture.expected);
    REQUIRE(decoded.transformed());
    REQUIRE_FALSE(decoded.borrowed());
}

// ========================================
// Header parsing
// ========================================

TEST_CASE("YamlBlockScalarDecoder parses literal block scalar header",
          "[job_yaml][block_scalar_decoder][header][literal]")
{
    YamlBlockScalarHeader header;

    REQUIRE(YamlBlockScalarDecoder::parseHeader("|", header));

    REQUIRE(header.style == YamlBlockScalarStyle::Literal);
    REQUIRE(header.chomping == YamlBlockScalarChomping::Clip);
    REQUIRE(header.indentation == 0);
}

TEST_CASE("YamlBlockScalarDecoder parses folded block scalar header",
          "[job_yaml][block_scalar_decoder][header][folded]")
{
    YamlBlockScalarHeader header;

    REQUIRE(YamlBlockScalarDecoder::parseHeader(">", header));

    REQUIRE(header.style == YamlBlockScalarStyle::Folded);
    REQUIRE(header.chomping == YamlBlockScalarChomping::Clip);
    REQUIRE(header.indentation == 0);
}

TEST_CASE("YamlBlockScalarDecoder parses chomping indicators",
          "[job_yaml][block_scalar_decoder][header][chomping]")
{
    SECTION("strip")
    {
        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader("|-", header));

        REQUIRE(header.style == YamlBlockScalarStyle::Literal);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Strip);
        REQUIRE(header.indentation == 0);
    }

    SECTION("keep")
    {
        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader(">+", header));

        REQUIRE(header.style == YamlBlockScalarStyle::Folded);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Keep);
        REQUIRE(header.indentation == 0);
    }
}

TEST_CASE("YamlBlockScalarDecoder parses explicit indentation",
          "[job_yaml][block_scalar_decoder][header][indent]")
{
    for (std::size_t indentation = 1; indentation <= 9; ++indentation) {
        const char digit = static_cast<char>('0' + indentation);
        const char source[] = {'|', digit};

        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader(
            std::string_view{source, 2},
            header));

        REQUIRE(header.style == YamlBlockScalarStyle::Literal);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Clip);
        REQUIRE(header.indentation == indentation);
    }
}

TEST_CASE("YamlBlockScalarDecoder accepts either header modifier order",
          "[job_yaml][block_scalar_decoder][header]")
{
    SECTION("indentation then chomping")
    {
        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader("|2-", header));

        REQUIRE(header.style == YamlBlockScalarStyle::Literal);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Strip);
        REQUIRE(header.indentation == 2);
    }

    SECTION("chomping then indentation")
    {
        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader("|-2", header));

        REQUIRE(header.style == YamlBlockScalarStyle::Literal);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Strip);
        REQUIRE(header.indentation == 2);
    }

    SECTION("folded indentation then keep")
    {
        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader(">2+", header));

        REQUIRE(header.style == YamlBlockScalarStyle::Folded);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Keep);
        REQUIRE(header.indentation == 2);
    }

    SECTION("folded keep then indentation")
    {
        YamlBlockScalarHeader header;

        REQUIRE(YamlBlockScalarDecoder::parseHeader(">+2", header));

        REQUIRE(header.style == YamlBlockScalarStyle::Folded);
        REQUIRE(header.chomping == YamlBlockScalarChomping::Keep);
        REQUIRE(header.indentation == 2);
    }
}

TEST_CASE("YamlBlockScalarDecoder rejects malformed headers",
          "[job_yaml][block_scalar_decoder][header][invalid]")
{
    YamlBlockScalarHeader header;

    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader("", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader("x", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader("|0", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader(">0", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader("|22", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader(">++", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader("|--", header));
    REQUIRE_FALSE(YamlBlockScalarDecoder::parseHeader("|x", header));
}

// ========================================
// Generated Stage 11 fixtures
// ========================================

TEST_CASE("YamlBlockScalarDecoder decodes generated literal fixtures",
          "[job_yaml][block_scalar_decoder][literal][generated]")
{
    for (const auto &fixture : LiteralBlockScalarFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlBlockScalarDecoder decodes generated folded fixtures",
          "[job_yaml][block_scalar_decoder][folded][generated]")
{
    for (const auto &fixture : FoldedBlockScalarFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlBlockScalarDecoder decodes generated chomping fixtures",
          "[job_yaml][block_scalar_decoder][chomping][generated]")
{
    for (const auto &fixture : BlockScalarChompingFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlBlockScalarDecoder decodes generated indentation fixtures",
          "[job_yaml][block_scalar_decoder][indent][generated]")
{
    for (const auto &fixture : BlockScalarIndentationFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlBlockScalarDecoder decodes generated header order fixtures",
          "[job_yaml][block_scalar_decoder][header][generated]")
{
    for (const auto &fixture : BlockScalarHeaderOrderFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlBlockScalarDecoder rejects generated invalid fixtures",
          "[job_yaml][block_scalar_decoder][invalid][generated]")
{
    for (const auto &fixture : InvalidBlockScalarFixtures)
        testFixture(fixture);
}

// ========================================
// Parent indentation
// ========================================

TEST_CASE("YamlBlockScalarDecoder applies parent indentation to explicit indentation",
          "[job_yaml][block_scalar_decoder][indent][parent]")
{
    YamlDecodedScalar decoded;

    REQUIRE(YamlBlockScalarDecoder::decode(
        "|2",
        "    alpha\n"
        "    beta\n",
        2,
        decoded));

    REQUIRE(decoded.value() == "alpha\nbeta\n");
    REQUIRE(decoded.transformed());
}

TEST_CASE("YamlBlockScalarDecoder discovers indentation relative to parent",
          "[job_yaml][block_scalar_decoder][indent][parent][auto]")
{
    YamlDecodedScalar decoded;

    REQUIRE(YamlBlockScalarDecoder::decode(
        "|",
        "    alpha\n"
        "    beta\n",
        2,
        decoded));

    REQUIRE(decoded.value() == "alpha\nbeta\n");
}

TEST_CASE("YamlBlockScalarDecoder preserves indentation beyond required content indentation",
          "[job_yaml][block_scalar_decoder][indent][more_indented]")
{
    YamlDecodedScalar decoded;

    REQUIRE(YamlBlockScalarDecoder::decode(
        "|",
        "  alpha\n"
        "    beta\n"
        "  gamma\n",
        0,
        decoded));

    REQUIRE(decoded.value() == "alpha\n  beta\ngamma\n");
}

// ========================================
// Folding
// ========================================

TEST_CASE("YamlBlockScalarDecoder preserves more-indented folded lines",
          "[job_yaml][block_scalar_decoder][folded][more_indented]")
{
    YamlDecodedScalar decoded;

    REQUIRE(YamlBlockScalarDecoder::decode(
        ">",
        "  alpha\n"
        "    beta\n"
        "    gamma\n"
        "  delta\n",
        0,
        decoded));

    REQUIRE(decoded.value() == "alpha\n  beta\n  gamma\ndelta\n");
}

TEST_CASE("YamlBlockScalarDecoder folds ordinary adjacent lines",
          "[job_yaml][block_scalar_decoder][folded]")
{
    YamlDecodedScalar decoded;

    REQUIRE(YamlBlockScalarDecoder::decode(
        ">",
        "  alpha\n"
        "  beta\n"
        "  gamma\n",
        0,
        decoded));

    REQUIRE(decoded.value() == "alpha beta gamma\n");
}

TEST_CASE("YamlBlockScalarDecoder preserves folded blank line semantics",
          "[job_yaml][block_scalar_decoder][folded][blank]")
{
    SECTION("one blank line")
    {
        YamlDecodedScalar decoded;

        REQUIRE(YamlBlockScalarDecoder::decode(
            ">",
            "  alpha\n"
            "\n"
            "  beta\n",
            0,
            decoded));

        REQUIRE(decoded.value() == "alpha\nbeta\n");
    }

    SECTION("two blank lines")
    {
        YamlDecodedScalar decoded;

        REQUIRE(YamlBlockScalarDecoder::decode(
            ">",
            "  alpha\n"
            "\n"
            "\n"
            "  beta\n",
            0,
            decoded));

        REQUIRE(decoded.value() == "alpha\n\nbeta\n");
    }
}

// ========================================
// Physical line handling
// ========================================

TEST_CASE("YamlBlockScalarDecoder normalizes CRLF line breaks",
          "[job_yaml][block_scalar_decoder][line_break][crlf]")
{
    SECTION("literal")
    {
        YamlDecodedScalar decoded;

        REQUIRE(YamlBlockScalarDecoder::decode(
            "|",
            "  alpha\r\n"
            "  beta\r\n",
            0,
            decoded));

        REQUIRE(decoded.value() == "alpha\nbeta\n");
    }

    SECTION("folded")
    {
        YamlDecodedScalar decoded;

        REQUIRE(YamlBlockScalarDecoder::decode(
            ">",
            "  alpha\r\n"
            "  beta\r\n",
            0,
            decoded));

        REQUIRE(decoded.value() == "alpha beta\n");
    }
}

TEST_CASE("YamlBlockScalarDecoder rejects tab in indentation",
          "[job_yaml][block_scalar_decoder][indent][tab][invalid]")
{
    YamlDecodedScalar decoded;

    REQUIRE_FALSE(YamlBlockScalarDecoder::decode(
        "|",
        "\talpha\n",
        0,
        decoded));
}

// ========================================
// Result state
// ========================================

TEST_CASE("YamlBlockScalarDecoder produces transformed scalar storage",
          "[job_yaml][block_scalar_decoder][decoded_scalar]")
{
    YamlDecodedScalar decoded;

    REQUIRE(YamlBlockScalarDecoder::decode(
        "|",
        "  alpha\n",
        0,
        decoded));

    REQUIRE(decoded.value() == "alpha\n");
    REQUIRE(decoded.transformed());
    REQUIRE_FALSE(decoded.borrowed());
}

TEST_CASE("YamlBlockScalarDecoder replaces previous decoded result",
          "[job_yaml][block_scalar_decoder][decoded_scalar]")
{
    YamlDecodedScalar decoded;
    decoded.setBorrowed("old"sv);

    REQUIRE(YamlBlockScalarDecoder::decode(
        ">-",
        "  alpha\n"
        "  beta\n",
        0,
        decoded));

    REQUIRE(decoded.value() == "alpha beta");
    REQUIRE(decoded.transformed());
    REQUIRE_FALSE(decoded.borrowed());
}

} // namespace job::yaml::tests