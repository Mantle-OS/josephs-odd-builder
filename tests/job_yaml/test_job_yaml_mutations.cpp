#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>

#include "test_job_yaml_mutation_fixtures_generated.h"

#include <job_yaml_node.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>

namespace job::yaml::tests {

template <std::size_t Size>
static void requireMutationFixtures(const std::array<YamlMutationFixture, Size> &fixtures)
{
    for (const YamlMutationFixture &fixture : fixtures) {
        INFO("fixture: " << fixture.name);
        INFO("production: " << fixture.production);
        INFO("mutation: " << fixture.mutation);
        INFO("source: " << fixture.source);

        YamlNode node;
        YamlNodeParserDestination destination{node};
        YamlDiagnostic diagnostic;

        REQUIRE_FALSE(YamlParser::parse(fixture.source, destination, diagnostic));
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
// Generated token deletion mutations
// ========================================

TEST_CASE("YamlParser rejects generated token deletion mutations",
          "[job_yaml][parser][mutation][generated][delete_token]")
{
    requireMutationFixtures(DeleteTokenMutationFixtures);
}

// ========================================
// Generated token duplication mutations
// ========================================

TEST_CASE("YamlParser rejects generated token duplication mutations",
          "[job_yaml][parser][mutation][generated][duplicate_token]")
{
    requireMutationFixtures(DuplicateTokenMutationFixtures);
}

// ========================================
// Generated delimiter replacement mutations
// ========================================

TEST_CASE("YamlParser rejects generated delimiter replacement mutations",
          "[job_yaml][parser][mutation][generated][replace_delimiter]")
{
    requireMutationFixtures(ReplaceDelimiterMutationFixtures);
}

// ========================================
// Generated truncation mutations
// ========================================

TEST_CASE("YamlParser rejects generated truncation mutations",
          "[job_yaml][parser][mutation][generated][truncate]")
{
    requireMutationFixtures(TruncateMutationFixtures);
}

// ========================================
// Generated indentation mutations
// ========================================

TEST_CASE("YamlParser rejects generated indentation mutations",
          "[job_yaml][parser][mutation][generated][indentation]")
{
    requireMutationFixtures(IndentationMutationFixtures);
}

// ========================================
// Generated closer removal mutations
// ========================================

TEST_CASE("YamlParser rejects generated closer removal mutations",
          "[job_yaml][parser][mutation][generated][remove_closer]")
{
    requireMutationFixtures(RemoveCloserMutationFixtures);
}

// ========================================
// Generated anchor and alias mutations
// ========================================

TEST_CASE("YamlParser rejects generated anchor and alias mutations",
          "[job_yaml][parser][mutation][generated][anchor_alias]")
{
    requireMutationFixtures(AnchorAliasMutationFixtures);
}

// ========================================
// Generated document marker mutations
// ========================================

TEST_CASE("YamlParser rejects generated document marker mutations",
          "[job_yaml][parser][mutation][generated][document_marker]")
{
    requireMutationFixtures(DocumentMarkerMutationFixtures);
}

// ========================================
// Fixture metadata sanity
// ========================================

template <std::size_t Size>
static void requireMutationFixtureMetadata(const std::array<YamlMutationFixture, Size> &fixtures)
{
    for (const YamlMutationFixture &fixture : fixtures) {
        INFO("fixture: " << fixture.name);
        INFO("production: " << fixture.production);
        INFO("mutation: " << fixture.mutation);

        REQUIRE_FALSE(fixture.name.empty());
        REQUIRE_FALSE(fixture.production.empty());
        REQUIRE_FALSE(fixture.mutation.empty());
        REQUIRE_FALSE(fixture.source.empty());
        REQUIRE(fixture.kind != YamlDiagnosticKind::None);
        REQUIRE(fixture.offset <= fixture.source.size());
        REQUIRE(fixture.line >= 1);
        REQUIRE(fixture.column >= 1);

        if (fixture.rangeSize > 0)
            REQUIRE(fixture.offset + fixture.rangeSize <= fixture.source.size());
    }
}

TEST_CASE("Generated mutation fixtures have internally consistent metadata",
          "[job_yaml][mutation][generated][metadata]")
{
    requireMutationFixtureMetadata(DeleteTokenMutationFixtures);
    requireMutationFixtureMetadata(DuplicateTokenMutationFixtures);
    requireMutationFixtureMetadata(ReplaceDelimiterMutationFixtures);
    requireMutationFixtureMetadata(TruncateMutationFixtures);
    requireMutationFixtureMetadata(IndentationMutationFixtures);
    requireMutationFixtureMetadata(RemoveCloserMutationFixtures);
    requireMutationFixtureMetadata(AnchorAliasMutationFixtures);
    requireMutationFixtureMetadata(DocumentMarkerMutationFixtures);
}

} // namespace job::yaml::tests
