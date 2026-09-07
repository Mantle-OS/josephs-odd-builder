#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>

#include <job_yaml_diagnostic.h>

namespace job::yaml::tests {

// ========================================
// Default state
// ========================================

TEST_CASE("YamlDiagnostic default state is clear",
          "[job_yaml][diagnostic][default]")
{
    const YamlDiagnostic diagnostic;

    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::None);
    REQUIRE(diagnostic.range().offset == 0);
    REQUIRE(diagnostic.range().size == 0);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.line() == 0);
    REQUIRE(diagnostic.column() == 0);
}

TEST_CASE("YamlDiagnostic pointer factories create diagnostics",
          "[job_yaml][diagnostic][factory]")
{
    const YamlDiagnostic::Ptr shared = YamlDiagnostic::createShared();
    const YamlDiagnostic::UPtr unique = YamlDiagnostic::createUniq();

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);
    REQUIRE_FALSE(shared->valid());
    REQUIRE_FALSE(unique->valid());
}

// ========================================
// Set and clear
// ========================================

TEST_CASE("YamlDiagnostic set stores kind and source range",
          "[job_yaml][diagnostic][set]")
{
    constexpr std::string_view source = "alpha: beta\n";
    constexpr YamlSourceRange range{
        .offset = 7,
        .size = 4
    };

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::InvalidScalar, range, source);

    REQUIRE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::InvalidScalar);
    REQUIRE(diagnostic.range().offset == range.offset);
    REQUIRE(diagnostic.range().size == range.size);
    REQUIRE(diagnostic.offset() == range.offset);
    REQUIRE(diagnostic.line() == 1);
    REQUIRE(diagnostic.column() == 8);
}

TEST_CASE("YamlDiagnostic clear restores default state",
          "[job_yaml][diagnostic][clear]")
{
    constexpr std::string_view source = "alpha\nbeta\n";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = 6, .size = 4},
                   source);

    REQUIRE(diagnostic.valid());

    diagnostic.clear();

    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::None);
    REQUIRE(diagnostic.range().offset == 0);
    REQUIRE(diagnostic.range().size == 0);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.line() == 0);
    REQUIRE(diagnostic.column() == 0);
}

TEST_CASE("YamlDiagnostic clear is idempotent",
          "[job_yaml][diagnostic][clear]")
{
    YamlDiagnostic diagnostic;

    diagnostic.clear();
    diagnostic.clear();

    REQUIRE_FALSE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::None);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.line() == 0);
    REQUIRE(diagnostic.column() == 0);
}

TEST_CASE("YamlDiagnostic can be reused after clear",
          "[job_yaml][diagnostic][reuse]")
{
    constexpr std::string_view firstSource = "alpha\n";
    constexpr std::string_view secondSource = "one\ntwo\nthree";

    YamlDiagnostic diagnostic;

    diagnostic.set(YamlDiagnosticKind::InvalidScalar,
                   {.offset = 0, .size = 5},
                   firstSource);

    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::InvalidScalar);
    REQUIRE(diagnostic.line() == 1);
    REQUIRE(diagnostic.column() == 1);

    diagnostic.clear();

    diagnostic.set(YamlDiagnosticKind::InvalidDocument,
                   {.offset = 8, .size = 5},
                   secondSource);

    REQUIRE(diagnostic.valid());
    REQUIRE(diagnostic.kind() == YamlDiagnosticKind::InvalidDocument);
    REQUIRE(diagnostic.offset() == 8);
    REQUIRE(diagnostic.range().size == 5);
    REQUIRE(diagnostic.line() == 3);
    REQUIRE(diagnostic.column() == 1);
}

// ========================================
// Location calculation
// ========================================

TEST_CASE("YamlDiagnostic locates offset at source beginning",
          "[job_yaml][diagnostic][location]")
{
    constexpr std::string_view source = "alpha";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = 0, .size = 1},
                   source);

    REQUIRE(diagnostic.line() == 1);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic locates offset at source end",
          "[job_yaml][diagnostic][location][eof]")
{
    constexpr std::string_view source = "alpha";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedEnd,
                   {.offset = source.size(), .size = 0},
                   source);

    REQUIRE(diagnostic.offset() == source.size());
    REQUIRE(diagnostic.line() == 1);
    REQUIRE(diagnostic.column() == 6);
}

TEST_CASE("YamlDiagnostic locates LF line breaks",
          "[job_yaml][diagnostic][location][lf]")
{
    constexpr std::string_view source =
        "alpha\n"
        "beta\n"
        "gamma";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = 11, .size = 5},
                   source);

    REQUIRE(diagnostic.line() == 3);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic locates CR line breaks",
          "[job_yaml][diagnostic][location][cr]")
{
    constexpr std::string_view source = "alpha\rbeta\rgamma";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = 11, .size = 5},
                   source);

    REQUIRE(diagnostic.line() == 3);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic locates CRLF line breaks as one break",
          "[job_yaml][diagnostic][location][crlf]")
{
    constexpr std::string_view source =
        "alpha\r\n"
        "beta\r\n"
        "gamma";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = 13, .size = 5},
                   source);

    REQUIRE(diagnostic.line() == 3);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic locates mixed YAML line breaks", "[job_yaml][diagnostic][location][mixed]")
{
    constexpr std::string_view source = "one\r\ntwo\rthree\nfour";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   { .offset = 15, .size = 4 },
                   source);

    REQUIRE(diagnostic.line() == 4);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic locates columns after line break",
          "[job_yaml][diagnostic][location][column]")
{
    constexpr std::string_view source =
        "alpha\n"
        "beta gamma";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = 11, .size = 5},
                   source);

    REQUIRE(diagnostic.line() == 2);
    REQUIRE(diagnostic.column() == 6);
}

TEST_CASE("YamlDiagnostic keeps EOF after trailing LF on next line", "[job_yaml][diagnostic][location][lf][eof]")
{
    constexpr std::string_view source = "alpha\n";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedEnd,
                   {
                    .offset = source.size(),
                    .size = 0
                   },
                   source);

    REQUIRE(diagnostic.line() == 2);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic keeps EOF after trailing CR on next line",
          "[job_yaml][diagnostic][location][cr][eof]")
{
    constexpr std::string_view source = "alpha\r";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedEnd,
                   {.offset = source.size(), .size = 0},
                   source);

    REQUIRE(diagnostic.line() == 2);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic keeps EOF after trailing CRLF on next line",
          "[job_yaml][diagnostic][location][crlf][eof]")
{
    constexpr std::string_view source = "alpha\r\n";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedEnd,
                   {.offset = source.size(), .size = 0},
                   source);

    REQUIRE(diagnostic.line() == 2);
    REQUIRE(diagnostic.column() == 1);
}

TEST_CASE("YamlDiagnostic reports zero location for offset beyond source",
          "[job_yaml][diagnostic][location][out_of_range]")
{
    constexpr std::string_view source = "alpha";

    YamlDiagnostic diagnostic;
    diagnostic.set(YamlDiagnosticKind::UnexpectedToken,
                   {.offset = source.size() + 1, .size = 0},
                   source);

    REQUIRE(diagnostic.valid());
    REQUIRE(diagnostic.offset() == source.size() + 1);
    REQUIRE(diagnostic.line() == 0);
    REQUIRE(diagnostic.column() == 0);
}

// ========================================
// Diagnostic kinds
// ========================================

TEST_CASE("YamlDiagnostic treats every non-None diagnostic kind as valid",
          "[job_yaml][diagnostic][kind]")
{
    constexpr std::string_view source = "x";

    constexpr YamlDiagnosticKind kinds[] = {
        YamlDiagnosticKind::UnexpectedToken,
        YamlDiagnosticKind::UnexpectedEnd,
        YamlDiagnosticKind::InvalidIndentation,
        YamlDiagnosticKind::InvalidAnchor,
        YamlDiagnosticKind::InvalidAlias,
        YamlDiagnosticKind::InvalidTag,
        YamlDiagnosticKind::InvalidScalar,
        YamlDiagnosticKind::InvalidFlowCollection,
        YamlDiagnosticKind::InvalidBlockScalar,
        YamlDiagnosticKind::InvalidDirective,
        YamlDiagnosticKind::InvalidDocument,
        YamlDiagnosticKind::DestinationRejected
    };

    for (const YamlDiagnosticKind kind : kinds) {
        YamlDiagnostic diagnostic;
        diagnostic.set(kind, {.offset = 0, .size = 1}, source);

        REQUIRE(diagnostic.valid());
        REQUIRE(diagnostic.kind() == kind);
    }
}

} // namespace job::yaml::tests
