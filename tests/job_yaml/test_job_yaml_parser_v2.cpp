#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include "test_job_yaml_fixtures.h"

#include <job_yaml_diagnostic.h>
#include <job_yaml_node.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>

namespace job::yaml::tests {

static void requireDiagnostic(const YamlDiagnostic &diagnostic,
                              YamlDiagnosticKind kind,
                              std::size_t offset,
                              std::size_t size,
                              std::size_t line,
                              std::size_t column)
{
    REQUIRE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == kind);
    REQUIRE(diagnostic.offset() == offset);
    REQUIRE(diagnostic.range().size == size);
    REQUIRE(diagnostic.line() == line);
    REQUIRE(diagnostic.column() == column);
}


// ========================================
// Explicit document start
// ========================================

TEST_CASE("YamlParser parses explicit document root scalar",
          "[job_yaml][parser][v2][document][start][scalar]")
{
    constexpr std::string_view source = "--- alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser parses explicit document mapping",
          "[job_yaml][parser][v2][document][start][mapping]")
{
    constexpr std::string_view source =
        "---\n"
        "name: alpha\n"
        "count: 42\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser parses explicit document block sequence",
          "[job_yaml][parser][v2][document][start][sequence]")
{
    constexpr std::string_view source =
        "---\n"
        "- alpha\n"
        "- beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 2);
    REQUIRE(node.sequence()[0].scalar() == "alpha");
    REQUIRE(node.sequence()[1].scalar() == "beta");
}

TEST_CASE("YamlParser parses explicit document flow sequence",
          "[job_yaml][parser][v2][document][start][flow]")
{
    constexpr std::string_view source = "--- [alpha, beta]\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 2);
    REQUIRE(node.sequence()[0].scalar() == "alpha");
    REQUIRE(node.sequence()[1].scalar() == "beta");
}

// ========================================
// Explicit document end
// ========================================

TEST_CASE("YamlParser accepts explicit document end after scalar",
          "[job_yaml][parser][v2][document][end][scalar]")
{
    constexpr std::string_view source =
        "alpha\n"
        "...\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser accepts comments and blank lines after document end",
          "[job_yaml][parser][v2][document][end][tail]")
{
    constexpr std::string_view source =
        "---\n"
        "alpha\n"
        "...\n"
        "\n"
        "# trailing comment\n"
        "\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser rejects content after document end",
          "[job_yaml][parser][v2][document][end][invalid]")
{
    constexpr std::string_view source =
        "alpha\n"
        "...\n"
        "beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find("beta");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      offset,
                      4,
                      3,
                      1);
}

TEST_CASE("YamlParser rejects duplicate document end",
          "[job_yaml][parser][v2][document][end][duplicate][invalid]")
{
    constexpr std::string_view source =
        "alpha\n"
        "...\n"
        "...\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.rfind("...");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      offset,
                      3,
                      3,
                      1);
}

TEST_CASE("YamlParser rejects second document start after document end",
          "[job_yaml][parser][v2][document][multiple][invalid]")
{
    constexpr std::string_view source =
        "alpha\n"
        "...\n"
        "---\n"
        "beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.rfind("---");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      offset,
                      3,
                      3,
                      1);
}

// ========================================
// Directives
// ========================================

TEST_CASE("YamlParser accepts directive before explicit document start",
          "[job_yaml][parser][v2][directive]")
{
    constexpr std::string_view source =
        "%YAML 1.2\n"
        "---\n"
        "alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser accepts multiple structural directives before explicit document start",
          "[job_yaml][parser][v2][directive]")
{
    constexpr std::string_view source =
        "%YAML 1.2\n"
        "%TAG !e! tag:example.com,2026:\n"
        "---\n"
        "alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser rejects directive without explicit document start",
          "[job_yaml][parser][v2][directive][invalid]")
{
    constexpr std::string_view source =
        "%YAML 1.2\n"
        "alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find("alpha");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDirective,
                      offset,
                      5,
                      2,
                      1);
}

TEST_CASE("YamlParser rejects directive after document content",
          "[job_yaml][parser][v2][directive][placement][invalid]")
{
    constexpr std::string_view source = "alpha\n" "%YAML 1.2\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find('%');
    requireDiagnostic(diagnostic, YamlDiagnosticKind::InvalidDirective, offset,
                      5, 2, 1);
}

// ========================================
// Document diagnostics
// ========================================

TEST_CASE("YamlParser reports diagnostic for empty source",
          "[job_yaml][parser][v2][diagnostic][document]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse("", destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      0,
                      0,
                      1,
                      1);
}

TEST_CASE("YamlParser reports diagnostic for comment-only source",
          "[job_yaml][parser][v2][diagnostic][document]")
{
    constexpr std::string_view source = "# nothing here\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      source.size(),
                      0,
                      2,
                      1);
}

TEST_CASE("YamlParser rejects leading document end",
          "[job_yaml][parser][v2][diagnostic][document]")
{
    constexpr std::string_view source = "...\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      0,
                      3,
                      1,
                      1);
}

TEST_CASE("YamlParser currently rejects explicit empty document",
          "[job_yaml][parser][v2][document][empty][current_subset]")
{
    constexpr std::string_view source = "---\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::InvalidDocument);
}

// ========================================
// Syntax diagnostics
// ========================================

TEST_CASE("YamlParser reports invalid scalar diagnostic",
          "[job_yaml][parser][v2][diagnostic][scalar]")
{
    constexpr std::string_view source = "\"\\q\"";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidScalar,
                      0,
                      source.size(),
                      1,
                      1);
}

TEST_CASE("YamlParser reports invalid anchor diagnostic",
          "[job_yaml][parser][v2][diagnostic][anchor]")
{
    constexpr std::string_view source = "&";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidAnchor,
                      0,
                      1,
                      1,
                      1);
}

TEST_CASE("YamlParser reports invalid alias diagnostic",
          "[job_yaml][parser][v2][diagnostic][alias]")
{
    constexpr std::string_view source = "*missing";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidAlias,
                      0,
                      source.size(),
                      1,
                      1);
}

TEST_CASE("YamlParser reports duplicate tag diagnostic",
          "[job_yaml][parser][v2][diagnostic][tag]")
{
    constexpr std::string_view source = "!one !two alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find("!two");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidTag,
                      offset,
                      4,
                      1,
                      offset + 1);
}

TEST_CASE("YamlParser reports unexpected end for unclosed flow sequence",
          "[job_yaml][parser][v2][diagnostic][flow]")
{
    constexpr std::string_view source = "[alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::UnexpectedEnd,
                      source.size(),
                      0,
                      1,
                      source.size() + 1);
}

TEST_CASE("YamlParser reports invalid flow collection for duplicate separator",
          "[job_yaml][parser][v2][diagnostic][flow]")
{
    constexpr std::string_view source = "[alpha,,beta]";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find(",,") + 1;
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidFlowCollection,
                      offset,
                      1,
                      1,
                      offset + 1);
}

TEST_CASE("YamlParser reports invalid block scalar diagnostic",
          "[job_yaml][parser][v2][diagnostic][block_scalar]")
{
    constexpr std::string_view source =
        "|0\n"
        "  alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::InvalidBlockScalar);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.line() == 1);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlParser reports invalid indentation diagnostic",
          "[job_yaml][parser][v2][diagnostic][indentation]")
{
    constexpr std::string_view source = "  alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidIndentation,
                      2,
                      5,
                      1,
                      3);
}

// ========================================
// Destination diagnostics
// ========================================

TEST_CASE("YamlParser reports destination rejected diagnostic",
          "[job_yaml][parser][v2][diagnostic][destination]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("alpha", destination));
    REQUIRE(destination.complete());

    YamlDiagnostic diagnostic;
    REQUIRE_FALSE(YamlParser::parse("beta", destination, diagnostic));

    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::DestinationRejected,
                      0,
                      4,
                      1,
                      1);
}

// ========================================
// Alias replay diagnostics
// ========================================

TEST_CASE("YamlParser normalizes alias replay failure to alias occurrence",
          "[job_yaml][parser][v2][diagnostic][alias][replay]")
{
    constexpr std::string_view source =
        "name: &value alpha\n"
        "count: *value\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find("*value");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidAlias,
                      offset,
                      6,
                      2,
                      8);

    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 0);
}

// ========================================
// Diagnostic source locations
// ========================================

TEST_CASE("YamlParser diagnostic location handles CRLF as one line break",
          "[job_yaml][parser][v2][diagnostic][location][crlf]")
{
    constexpr std::string_view source =
        "# prefix\r\n"
        "---\r\n"
        "alpha\r\n"
        "...\r\n"
        "beta";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    REQUIRE_FALSE(YamlParser::parse(source, destination, diagnostic));

    constexpr std::size_t offset = source.find("beta");
    requireDiagnostic(diagnostic,
                      YamlDiagnosticKind::InvalidDocument,
                      offset,
                      4,
                      5,
                      1);
}

TEST_CASE("YamlParser successful diagnostic overload leaves diagnostic clear",
          "[job_yaml][parser][v2][diagnostic][success]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};
    YamlDiagnostic diagnostic;

    diagnostic.set(YamlDiagnosticKind::InvalidDocument, {.offset = 0, .size = 0}, source);
    REQUIRE(diagnostic.valid());

    REQUIRE(YamlParser::parse(source, destination, diagnostic));
    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(destination.complete());
    REQUIRE(node.scalar() == "alpha");
}

} // namespace job::yaml::tests
