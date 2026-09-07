#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include <job_yaml_node.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>
#include <job_yaml_scalar_kernel.h>

#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_scalar_fixtures_generated.h"

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Root scalars
// ========================================

TEST_CASE("YamlParser parses plain root scalar as borrowed source view",
          "[job_yaml][parser][scalar][plain][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlParser parses simple single quoted root scalar as borrowed interior source view",
          "[job_yaml][parser][scalar][single_quoted][source_view]")
{
    constexpr std::string_view source = "'alpha'";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar().data() == source.data() + 1);
    REQUIRE(result.scalar().size() == 5);
}

TEST_CASE("YamlParser parses transformed single quoted root scalar as owned value",
          "[job_yaml][parser][scalar][single_quoted][owned]")
{
    constexpr std::string_view source = "'it''s'";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "it's");
    REQUIRE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
}

TEST_CASE("YamlParser parses simple double quoted root scalar as borrowed interior source view",
          "[job_yaml][parser][scalar][double_quoted][source_view]")
{
    constexpr std::string_view source = "\"alpha\"";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar().data() == source.data() + 1);
    REQUIRE(result.scalar().size() == 5);
}

TEST_CASE("YamlParser parses transformed double quoted root scalar as owned value",
          "[job_yaml][parser][scalar][double_quoted][owned]")
{
    constexpr std::string_view source = "\"hello\\nworld\"";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "hello\nworld");
    REQUIRE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
}

// ========================================
// Mapping values
// ========================================

TEST_CASE("YamlParser parses single quoted mapping value",
          "[job_yaml][parser][scalar][mapping][single_quoted]")
{
    constexpr std::string_view source = "name: 'alpha'\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *name = node.member("name");

    REQUIRE(destination.complete());
    REQUIRE(name != nullptr);
    REQUIRE(name->isScalar());
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(name->borrowsScalar());
    REQUIRE_FALSE(name->ownsScalar());

    const std::size_t offset = source.find("'alpha'");

    REQUIRE(offset != std::string_view::npos);
    REQUIRE(name->scalar().data() == source.data() + offset + 1);
}

TEST_CASE("YamlParser parses transformed single quoted mapping value",
          "[job_yaml][parser][scalar][mapping][single_quoted][owned]")
{
    constexpr std::string_view source = "name: 'it''s fine'\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *name = node.member("name");

    REQUIRE(destination.complete());
    REQUIRE(name != nullptr);
    REQUIRE(name->scalar() == "it's fine");
    REQUIRE(name->ownsScalar());
    REQUIRE_FALSE(name->borrowsScalar());
}

TEST_CASE("YamlParser parses double quoted mapping value",
          "[job_yaml][parser][scalar][mapping][double_quoted]")
{
    constexpr std::string_view source = "name: \"alpha\"\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *name = node.member("name");

    REQUIRE(destination.complete());
    REQUIRE(name != nullptr);
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(name->borrowsScalar());

    const std::size_t offset = source.find("\"alpha\"");

    REQUIRE(offset != std::string_view::npos);
    REQUIRE(name->scalar().data() == source.data() + offset + 1);
}

TEST_CASE("YamlParser parses transformed double quoted mapping value",
          "[job_yaml][parser][scalar][mapping][double_quoted][owned]")
{
    constexpr std::string_view source = "name: \"hello\\tworld\"\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *name = node.member("name");

    REQUIRE(destination.complete());
    REQUIRE(name != nullptr);
    REQUIRE(name->scalar() == "hello\tworld");
    REQUIRE(name->ownsScalar());
    REQUIRE_FALSE(name->borrowsScalar());
}

// ========================================
// Mapping keys
// ========================================

TEST_CASE("YamlParser parses single quoted mapping key",
          "[job_yaml][parser][scalar][mapping][key][single_quoted]")
{
    constexpr std::string_view source = "'name': alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *name = node.member("name");

    REQUIRE(name != nullptr);
    REQUIRE(name->scalar() == "alpha");
}

TEST_CASE("YamlParser parses transformed single quoted mapping key",
          "[job_yaml][parser][scalar][mapping][key][single_quoted][owned]")
{
    constexpr std::string_view source = "'it''s': alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *value = node.member("it's");

    REQUIRE(value != nullptr);
    REQUIRE(value->scalar() == "alpha");
}

TEST_CASE("YamlParser parses double quoted mapping key",
          "[job_yaml][parser][scalar][mapping][key][double_quoted]")
{
    constexpr std::string_view source = "\"name\": alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *name = node.member("name");

    REQUIRE(name != nullptr);
    REQUIRE(name->scalar() == "alpha");
}

TEST_CASE("YamlParser parses transformed double quoted mapping key",
          "[job_yaml][parser][scalar][mapping][key][double_quoted][owned]")
{
    constexpr std::string_view source = "\"display\\tname\": alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *value = node.member("display\tname");

    REQUIRE(value != nullptr);
    REQUIRE(value->scalar() == "alpha");
}

// ========================================
// Sequences
// ========================================

TEST_CASE("YamlParser parses quoted scalars inside block sequence",
          "[job_yaml][parser][scalar][sequence]")
{
    constexpr std::string_view source =
        "- alpha\n"
        "- 'beta'\n"
        "- \"gamma\"\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isSequence());
    REQUIRE(result.sequence().size() == 3);

    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[0].borrowsScalar());

    REQUIRE(result.sequence()[1].scalar() == "beta");
    REQUIRE(result.sequence()[1].borrowsScalar());

    REQUIRE(result.sequence()[2].scalar() == "gamma");
    REQUIRE(result.sequence()[2].borrowsScalar());
}

TEST_CASE("YamlParser parses transformed quoted scalars inside block sequence",
          "[job_yaml][parser][scalar][sequence][owned]")
{
    constexpr std::string_view source =
        "- 'it''s'\n"
        "- \"hello\\nworld\"\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.sequence().size() == 2);

    REQUIRE(result.sequence()[0].scalar() == "it's");
    REQUIRE(result.sequence()[0].ownsScalar());

    REQUIRE(result.sequence()[1].scalar() == "hello\nworld");
    REQUIRE(result.sequence()[1].ownsScalar());
}

// ========================================
// Nested values
// ========================================

TEST_CASE("YamlParser parses quoted scalar in nested mapping",
          "[job_yaml][parser][scalar][mapping][nested]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  inner: 'alpha'\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *outer = node.member("outer");

    REQUIRE(outer != nullptr);

    const YamlNode *inner = outer->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->scalar() == "alpha");
    REQUIRE(inner->borrowsScalar());
}

TEST_CASE("YamlParser preserves transformed scalar through nested container moves",
          "[job_yaml][parser][scalar][mapping][nested][owned]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  inner: 'it''s'\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *outer = node.member("outer");

    REQUIRE(outer != nullptr);

    const YamlNode *inner = outer->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->scalar() == "it's");
    REQUIRE(inner->ownsScalar());
}

// ========================================
// Reflected destination
// ========================================

TEST_CASE("YamlParser decodes single quoted scalar into reflected string member",
          "[job_yaml][parser][scalar][object][single_quoted]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("name: 'alpha'\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlParser decodes transformed single quoted scalar into reflected string member",
          "[job_yaml][parser][scalar][object][single_quoted][transformed]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("name: 'it''s fine'\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "it's fine");
}

TEST_CASE("YamlParser decodes double quoted scalar into reflected string member",
          "[job_yaml][parser][scalar][object][double_quoted]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("name: \"alpha\"\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlParser decodes transformed double quoted scalar into reflected string member",
          "[job_yaml][parser][scalar][object][double_quoted][transformed]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("name: \"hello\\tworld\"\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "hello\tworld");
}

// ========================================
// Scalar conversions after decoding
// ========================================

TEST_CASE("YamlParser converts quoted integer into reflected integer member",
          "[job_yaml][parser][scalar][object][conversion]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("count: '42'\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser converts quoted boolean into reflected boolean member",
          "[job_yaml][parser][scalar][object][conversion]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("enabled: \"true\"\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.enabled);
}

TEST_CASE("YamlParser converts quoted floating point into reflected floating point member",
          "[job_yaml][parser][scalar][object][conversion]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("ratio: '1.5'\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.ratio == 1.5);
}

// ========================================
// Invalid quoted scalars
// ========================================

TEST_CASE("YamlParser rejects unterminated single quoted scalar",
          "[job_yaml][parser][scalar][single_quoted][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("'unterminated", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects unterminated double quoted scalar",
          "[job_yaml][parser][scalar][double_quoted][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("\"unterminated", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects invalid double quoted escape",
          "[job_yaml][parser][scalar][double_quoted][escape][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("\"\\q\"", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects malformed double quoted Unicode escape",
          "[job_yaml][parser][scalar][double_quoted][unicode][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("\"\\u12G4\"", destination));
    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Generated scalar corpus
// ========================================

TEST_CASE("YamlParser parses generated plain scalar fixtures",
          "[job_yaml][parser][scalar][plain][generated]")
{
    for (const YamlScalarFixture &fixture : PlainScalarFixtures) {
        INFO("fixture: " << fixture.name);
        INFO("source: " << fixture.source);

        YamlNode node;
        YamlNodeParserDestination destination{node};

        const bool parsed = YamlParser::parse(fixture.source, destination);

        REQUIRE(parsed == fixture.valid);

        if (!fixture.valid)
            continue;

        const YamlNode &result = node;

        REQUIRE(destination.complete());

        if (YamlScalarKernel::isNull(fixture.source)) {
            REQUIRE(result.isNull());
            continue;
        }

        REQUIRE(result.isScalar());
        REQUIRE(result.scalar() == fixture.expected);
        REQUIRE(result.ownsScalar() == fixture.transformed);
        REQUIRE(result.borrowsScalar() == !fixture.transformed);
    }
}

TEST_CASE("YamlParser parses generated single quoted scalar fixtures",
          "[job_yaml][parser][scalar][single_quoted][generated]")
{
    for (const YamlScalarFixture &fixture : SingleQuotedScalarFixtures) {
        INFO("fixture: " << fixture.name);
        INFO("source: " << fixture.source);

        YamlNode node;
        YamlNodeParserDestination destination{node};

        const bool parsed = YamlParser::parse(fixture.source, destination);

        REQUIRE(parsed == fixture.valid);

        if (!fixture.valid)
            continue;

        const YamlNode &result = node;

        REQUIRE(destination.complete());
        REQUIRE(result.isScalar());
        REQUIRE(result.scalar() == fixture.expected);
        REQUIRE(result.ownsScalar() == fixture.transformed);
        REQUIRE(result.borrowsScalar() == !fixture.transformed);
    }
}

TEST_CASE("YamlParser parses generated double quoted scalar fixtures",
          "[job_yaml][parser][scalar][double_quoted][generated]")
{
    for (const YamlScalarFixture &fixture : DoubleQuotedScalarFixtures) {
        INFO("fixture: " << fixture.name);
        INFO("source: " << fixture.source);

        YamlNode node;
        YamlNodeParserDestination destination{node};

        const bool parsed = YamlParser::parse(fixture.source, destination);

        REQUIRE(parsed == fixture.valid);

        if (!fixture.valid)
            continue;

        const YamlNode &result = node;

        REQUIRE(destination.complete());
        REQUIRE(result.isScalar());
        REQUIRE(result.scalar() == fixture.expected);
        REQUIRE(result.ownsScalar() == fixture.transformed);
        REQUIRE(result.borrowsScalar() == !fixture.transformed);
    }
}

} // namespace job::yaml::tests