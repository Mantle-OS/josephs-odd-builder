#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <string_view>

#include "test_job_yaml_diagnostic_fixtures_generated.h"

#include <job_yaml_node.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>

namespace job::yaml::tests {

template <std::size_t Size>
static void requireGeneratedDiagnosticFixtures(const std::array<YamlDiagnosticFixture, Size> &fixtures)
{
    for (const YamlDiagnosticFixture &fixture : fixtures) {
        INFO("fixture: " << fixture.name);
        INFO("production: " << fixture.production);
        INFO("source: " << fixture.source);

        if (fixture.name == "explicit_empty_document")
            continue;

        YamlNode node;
        YamlNodeParserDestination destination{node};
        YamlDiagnostic diagnostic;

        const bool parsed = YamlParser::parse(fixture.source, destination, diagnostic);

        REQUIRE(parsed == fixture.valid);

        if (fixture.valid) {
            REQUIRE_FALSE(diagnostic.valid());
            REQUIRE(destination.complete());
            continue;
        }

        REQUIRE(diagnostic.valid());
        REQUIRE(diagnostic.kind() == fixture.kind);
        REQUIRE(diagnostic.offset() == fixture.offset);
        REQUIRE(diagnostic.range().offset == fixture.offset);
        REQUIRE(diagnostic.range().size == fixture.rangeSize);
        REQUIRE(diagnostic.line() == fixture.line);
        REQUIRE(diagnostic.column() == fixture.column);
    }
}

// ========================================
// Generated document diagnostics
// ========================================

TEST_CASE("YamlParser matches generated document diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][document]")
{
    requireGeneratedDiagnosticFixtures(DocumentDiagnosticFixtures);
}

// ========================================
// Generated directive diagnostics
// ========================================

TEST_CASE("YamlParser matches generated directive diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][directive]")
{
    requireGeneratedDiagnosticFixtures(DirectiveDiagnosticFixtures);
}

// ========================================
// Generated property diagnostics
// ========================================

TEST_CASE("YamlParser matches generated property diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][property]")
{
    requireGeneratedDiagnosticFixtures(PropertyDiagnosticFixtures);
}

// ========================================
// Generated scalar diagnostics
// ========================================

TEST_CASE("YamlParser matches generated scalar diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][scalar]")
{
    requireGeneratedDiagnosticFixtures(ScalarDiagnosticFixtures);
}

// ========================================
// Generated flow diagnostics
// ========================================

TEST_CASE("YamlParser matches generated flow diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][flow]")
{
    requireGeneratedDiagnosticFixtures(FlowDiagnosticFixtures);
}

// ========================================
// Generated block scalar diagnostics
// ========================================

TEST_CASE("YamlParser matches generated block scalar diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][block_scalar]")
{
    requireGeneratedDiagnosticFixtures(BlockScalarDiagnosticFixtures);
}

// ========================================
// Generated indentation diagnostics
// ========================================

TEST_CASE("YamlParser matches generated indentation diagnostic fixtures",
          "[job_yaml][parser][diagnostic][generated][indentation]")
{
    requireGeneratedDiagnosticFixtures(IndentationDiagnosticFixtures);
}

// ========================================
// Fixture metadata sanity
// ========================================

template <std::size_t Size>
static void requireGeneratedFixtureMetadata(const std::array<YamlDiagnosticFixture, Size> &fixtures)
{
    for (const YamlDiagnosticFixture &fixture : fixtures) {
        INFO("fixture: " << fixture.name);
        INFO("production: " << fixture.production);

        REQUIRE_FALSE(fixture.name.empty());
        REQUIRE_FALSE(fixture.production.empty());

        if (fixture.valid) {
            REQUIRE(fixture.kind == YamlDiagnosticKind::None);
            REQUIRE(fixture.offset == 0);
            REQUIRE(fixture.line == 0);
            REQUIRE(fixture.column == 0);
            REQUIRE(fixture.rangeSize == 0);
        } else {
            REQUIRE(fixture.kind != YamlDiagnosticKind::None);
            REQUIRE(fixture.offset <= fixture.source.size());
            REQUIRE(fixture.line >= 1);
            REQUIRE(fixture.column >= 1);
        }
    }
}

TEST_CASE("Generated diagnostic fixtures have internally consistent metadata",
          "[job_yaml][diagnostic][generated][metadata]")
{
    requireGeneratedFixtureMetadata(DocumentDiagnosticFixtures);
    requireGeneratedFixtureMetadata(DirectiveDiagnosticFixtures);
    requireGeneratedFixtureMetadata(PropertyDiagnosticFixtures);
    requireGeneratedFixtureMetadata(ScalarDiagnosticFixtures);
    requireGeneratedFixtureMetadata(FlowDiagnosticFixtures);
    requireGeneratedFixtureMetadata(BlockScalarDiagnosticFixtures);
    requireGeneratedFixtureMetadata(IndentationDiagnosticFixtures);
}

} // namespace job::yaml::tests
