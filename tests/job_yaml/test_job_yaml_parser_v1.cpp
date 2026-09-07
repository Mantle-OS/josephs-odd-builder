#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include <job_yaml_node.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_parser.h>

#include "test_job_yaml_fixtures.h"

namespace job::yaml::tests {

// ========================================
// Block scalar root values
// ========================================

TEST_CASE("YamlParser parses root literal block scalar",
          "[job_yaml][parser][block_scalar][literal][root]")
{
    constexpr std::string_view source =
        "|\n"
        "  alpha\n"
        "  beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "alpha\nbeta\n");
    REQUIRE(node.ownsScalar());
    REQUIRE_FALSE(node.borrowsScalar());
}

TEST_CASE("YamlParser parses root folded block scalar",
          "[job_yaml][parser][block_scalar][folded][root]")
{
    constexpr std::string_view source =
        ">\n"
        "  alpha\n"
        "  beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "alpha beta\n");
    REQUIRE(node.ownsScalar());
    REQUIRE_FALSE(node.borrowsScalar());
}

// ========================================
// Block scalar mapping values
// ========================================

TEST_CASE("YamlParser parses literal block scalar mapping value",
          "[job_yaml][parser][block_scalar][literal][mapping]")
{
    constexpr std::string_view source =
        "text: |\n"
        "  alpha\n"
        "  beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *text = node.member("text");

    REQUIRE(text != nullptr);
    REQUIRE(text->isScalar());
    REQUIRE(text->scalar() == "alpha\nbeta\n");
    REQUIRE(text->ownsScalar());
}

TEST_CASE("YamlParser parses folded block scalar mapping value",
          "[job_yaml][parser][block_scalar][folded][mapping]")
{
    constexpr std::string_view source =
        "text: >\n"
        "  alpha\n"
        "  beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *text = node.member("text");

    REQUIRE(text != nullptr);
    REQUIRE(text->isScalar());
    REQUIRE(text->scalar() == "alpha beta\n");
    REQUIRE(text->ownsScalar());
}

TEST_CASE("YamlParser resumes mapping after literal block scalar",
          "[job_yaml][parser][block_scalar][literal][mapping][resume]")
{
    constexpr std::string_view source =
        "first: |\n"
        "  alpha\n"
        "  beta\n"
        "second: gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    REQUIRE(first->scalar() == "alpha\nbeta\n");
    REQUIRE(second->scalar() == "gamma");
}

TEST_CASE("YamlParser resumes mapping after folded block scalar",
          "[job_yaml][parser][block_scalar][folded][mapping][resume]")
{
    constexpr std::string_view source =
        "first: >\n"
        "  alpha\n"
        "  beta\n"
        "second: gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    REQUIRE(first->scalar() == "alpha beta\n");
    REQUIRE(second->scalar() == "gamma");
}

// ========================================
// Block scalar sequence entries
// ========================================

TEST_CASE("YamlParser parses literal block scalar sequence entry",
          "[job_yaml][parser][block_scalar][literal][sequence]")
{
    constexpr std::string_view source =
        "- |\n"
        "  alpha\n"
        "  beta\n"
        "- gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());

    const auto &sequence = node.sequence();

    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence[0].isScalar());
    REQUIRE(sequence[0].scalar() == "alpha\nbeta\n");
    REQUIRE(sequence[0].ownsScalar());
    REQUIRE(sequence[1].scalar() == "gamma");
}

TEST_CASE("YamlParser parses folded block scalar sequence entry",
          "[job_yaml][parser][block_scalar][folded][sequence]")
{
    constexpr std::string_view source =
        "- >\n"
        "  alpha\n"
        "  beta\n"
        "- gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const auto &sequence = node.sequence();

    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence[0].scalar() == "alpha beta\n");
    REQUIRE(sequence[0].ownsScalar());
    REQUIRE(sequence[1].scalar() == "gamma");
}

// ========================================
// Nested block scalar values
// ========================================

TEST_CASE("YamlParser parses nested literal block scalar",
          "[job_yaml][parser][block_scalar][literal][nested]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  text: |\n"
        "    alpha\n"
        "    beta\n"
        "  sibling: gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *outer = node.member("outer");

    REQUIRE(outer != nullptr);
    REQUIRE(outer->isMapping());

    const YamlNode *text = outer->member("text");
    const YamlNode *sibling = outer->member("sibling");

    REQUIRE(text != nullptr);
    REQUIRE(sibling != nullptr);

    REQUIRE(text->scalar() == "alpha\nbeta\n");
    REQUIRE(sibling->scalar() == "gamma");
}

TEST_CASE("YamlParser parses nested folded block scalar",
          "[job_yaml][parser][block_scalar][folded][nested]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  text: >\n"
        "    alpha\n"
        "    beta\n"
        "  sibling: gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *outer = node.member("outer");

    REQUIRE(outer != nullptr);

    const YamlNode *text = outer->member("text");
    const YamlNode *sibling = outer->member("sibling");

    REQUIRE(text != nullptr);
    REQUIRE(sibling != nullptr);

    REQUIRE(text->scalar() == "alpha beta\n");
    REQUIRE(sibling->scalar() == "gamma");
}

// ========================================
// Chomping
// ========================================

TEST_CASE("YamlParser applies block scalar chomping",
          "[job_yaml][parser][block_scalar][chomping]")
{
    SECTION("literal clip")
    {
        constexpr std::string_view source =
            "|\n"
            "  alpha\n"
            "\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha\n");
    }

    SECTION("literal strip")
    {
        constexpr std::string_view source =
            "|-\n"
            "  alpha\n"
            "\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha");
    }

    SECTION("literal keep")
    {
        constexpr std::string_view source =
            "|+\n"
            "  alpha\n"
            "\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha\n\n");
    }

    SECTION("folded clip")
    {
        constexpr std::string_view source =
            ">\n"
            "  alpha\n"
            "  beta\n"
            "\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha beta\n");
    }

    SECTION("folded strip")
    {
        constexpr std::string_view source =
            ">-\n"
            "  alpha\n"
            "  beta\n"
            "\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha beta");
    }

    SECTION("folded keep")
    {
        constexpr std::string_view source =
            ">+\n"
            "  alpha\n"
            "  beta\n"
            "\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha beta\n\n");
    }
}

// ========================================
// Explicit indentation
// ========================================

TEST_CASE("YamlParser applies explicit block scalar indentation",
          "[job_yaml][parser][block_scalar][indent]")
{
    SECTION("literal")
    {
        constexpr std::string_view source =
            "|2\n"
            "  alpha\n"
            "  beta\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha\nbeta\n");
    }

    SECTION("folded")
    {
        constexpr std::string_view source =
            ">2\n"
            "  alpha\n"
            "  beta\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha beta\n");
    }

    SECTION("indentation then chomping")
    {
        constexpr std::string_view source =
            "|2-\n"
            "  alpha\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha");
    }

    SECTION("chomping then indentation")
    {
        constexpr std::string_view source =
            "|-2\n"
            "  alpha\n";

        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse(source, destination));
        REQUIRE(node.scalar() == "alpha");
    }
}

// ========================================
// Header comments
// ========================================

TEST_CASE("YamlParser accepts comment after block scalar header",
          "[job_yaml][parser][block_scalar][comment]")
{
    constexpr std::string_view source =
        "text: |2- # literal text\n"
        "  alpha\n"
        "next: beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *text = node.member("text");
    const YamlNode *next = node.member("next");

    REQUIRE(text != nullptr);
    REQUIRE(next != nullptr);

    REQUIRE(text->scalar() == "alpha");
    REQUIRE(next->scalar() == "beta");
}

// ========================================
// Reflected object destination
// ========================================

TEST_CASE("YamlParser assigns literal block scalar to reflected string member",
          "[job_yaml][parser][block_scalar][literal][object]")
{
    constexpr std::string_view source =
        "name: |\n"
        "  alpha\n"
        "  beta\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha\nbeta\n");
}

TEST_CASE("YamlParser assigns folded block scalar to reflected string member",
          "[job_yaml][parser][block_scalar][folded][object]")
{
    constexpr std::string_view source =
        "name: >\n"
        "  alpha\n"
        "  beta\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha beta\n");
}

// ========================================
// Invalid block scalar structure
// ========================================

TEST_CASE("YamlParser rejects malformed block scalar header",
          "[job_yaml][parser][block_scalar][invalid]")
{
    SECTION("zero indentation")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse(
            "|0\n"
            "  alpha\n",
            destination));
    }

    SECTION("duplicate indentation")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse(
            "|22\n"
            "  alpha\n",
            destination));
    }

    SECTION("duplicate chomping")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse(
            ">++\n"
            "  alpha\n",
            destination));
    }

    SECTION("invalid header character")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse(
            "|x\n"
            "  alpha\n",
            destination));
    }
}

TEST_CASE("YamlParser rejects insufficient explicit block scalar indentation",
          "[job_yaml][parser][block_scalar][indent][invalid]")
{
    SECTION("literal")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse(
            "|3\n"
            "  alpha\n",
            destination));
    }

    SECTION("folded")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse(
            ">3\n"
            "  alpha\n",
            destination));
    }
}

} // namespace job::yaml::tests