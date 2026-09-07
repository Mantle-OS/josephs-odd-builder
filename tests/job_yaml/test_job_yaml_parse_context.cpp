#include <catch2/catch_test_macros.hpp>

#include <job_yaml_parse_context.h>

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Default state
// ========================================

TEST_CASE("YamlParseContext default constructs empty", "[job_yaml][parse_context]")
{
    const YamlParseContext context;

    REQUIRE(context.source.empty());
    REQUIRE_FALSE(context.hasSource());

    REQUIRE(context.anchors.empty());
    REQUIRE(context.anchors.size() == 0);

    REQUIRE(context.aliases.empty());
    REQUIRE(context.aliases.size() == 0);
}

// ========================================
// Source
// ========================================

TEST_CASE("YamlParseContext stores source view", "[job_yaml][parse_context]")
{
    constexpr std::string_view source = "alpha: beta"sv;

    YamlParseContext context;
    context.source = source;

    REQUIRE(context.hasSource());
    REQUIRE(context.source == source);
}

TEST_CASE("YamlParseContext source is borrowed", "[job_yaml][parse_context]")
{
    constexpr std::string_view source = "alpha: beta"sv;

    YamlParseContext context;
    context.source = source;

    REQUIRE(context.source.data() == source.data());
    REQUIRE(context.source.size() == source.size());
}

TEST_CASE("YamlParseContext empty source reports no source", "[job_yaml][parse_context]")
{
    YamlParseContext context;
    context.source = {};

    REQUIRE(context.source.empty());
    REQUIRE_FALSE(context.hasSource());
}

// ========================================
// Document state
// ========================================

TEST_CASE("YamlParseContext exposes anchor table", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    REQUIRE(context.anchors.reserve("alpha"));

    REQUIRE(context.anchors.size() == 1);
    REQUIRE(context.anchors.contains("alpha"));
}

TEST_CASE("YamlParseContext exposes alias stack", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    REQUIRE(context.aliases.push("alpha"));

    REQUIRE(context.aliases.size() == 1);
    REQUIRE(context.aliases.contains("alpha"));
}

TEST_CASE("YamlParseContext anchor and alias state are independent", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    REQUIRE(context.anchors.reserve("alpha"));
    REQUIRE(context.aliases.push("beta"));

    REQUIRE(context.anchors.contains("alpha"));
    REQUIRE_FALSE(context.anchors.contains("beta"));

    REQUIRE(context.aliases.contains("beta"));
    REQUIRE_FALSE(context.aliases.contains("alpha"));
}

// ========================================
// Clear
// ========================================

TEST_CASE("YamlParseContext clear resets complete document state", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.source = "alpha: beta"sv;

    REQUIRE(context.anchors.reserve("anchor"));
    REQUIRE(context.anchors.complete("anchor", {
                                                   .offset = 0,
                                                   .size = 5
                                               }, 2));

    REQUIRE(context.aliases.push("alias"));

    context.clear();

    REQUIRE(context.source.empty());
    REQUIRE_FALSE(context.hasSource());

    REQUIRE(context.anchors.empty());
    REQUIRE(context.anchors.size() == 0);
    REQUIRE_FALSE(context.anchors.contains("anchor"));

    REQUIRE(context.aliases.empty());
    REQUIRE(context.aliases.size() == 0);
    REQUIRE_FALSE(context.aliases.contains("alias"));
}

TEST_CASE("YamlParseContext can be reused after clear", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.source = "first"sv;

    REQUIRE(context.anchors.reserve("alpha"));
    REQUIRE(context.aliases.push("beta"));

    context.clear();

    context.source = "second"sv;

    REQUIRE(context.anchors.reserve("gamma"));
    REQUIRE(context.aliases.push("delta"));

    REQUIRE(context.source == "second");
    REQUIRE(context.anchors.contains("gamma"));
    REQUIRE(context.aliases.contains("delta"));
}

// ========================================
// Reset
// ========================================

TEST_CASE("YamlParseContext reset installs new source", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.reset("alpha: beta"sv);

    REQUIRE(context.hasSource());
    REQUIRE(context.source == "alpha: beta");
    REQUIRE(context.anchors.empty());
    REQUIRE(context.aliases.empty());
}

TEST_CASE("YamlParseContext reset clears previous anchor state", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.reset("first"sv);

    REQUIRE(context.anchors.reserve("alpha"));
    REQUIRE(context.anchors.complete("alpha", {
                                                  .offset = 0,
                                                  .size = 5
                                              }, 0));

    context.reset("second"sv);

    REQUIRE(context.source == "second");
    REQUIRE(context.anchors.empty());
    REQUIRE_FALSE(context.anchors.contains("alpha"));
}

TEST_CASE("YamlParseContext reset clears previous alias state", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.reset("first"sv);

    REQUIRE(context.aliases.push("alpha"));

    context.reset("second"sv);

    REQUIRE(context.source == "second");
    REQUIRE(context.aliases.empty());
    REQUIRE_FALSE(context.aliases.contains("alpha"));
}

TEST_CASE("YamlParseContext reset replaces complete document state", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.reset("first: document"sv);

    REQUIRE(context.anchors.reserve("alpha"));
    REQUIRE(context.anchors.complete("alpha", {
                                                  .offset = 7,
                                                  .size = 8
                                              }, 2));

    REQUIRE(context.aliases.push("beta"));

    context.reset("second: document"sv);

    REQUIRE(context.source == "second: document");

    REQUIRE(context.anchors.empty());
    REQUIRE(context.anchors.size() == 0);

    REQUIRE(context.aliases.empty());
    REQUIRE(context.aliases.size() == 0);
}

TEST_CASE("YamlParseContext reset with empty source clears state", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.reset("alpha"sv);

    REQUIRE(context.anchors.reserve("anchor"));
    REQUIRE(context.aliases.push("alias"));

    context.reset({});

    REQUIRE(context.source.empty());
    REQUIRE_FALSE(context.hasSource());
    REQUIRE(context.anchors.empty());
    REQUIRE(context.aliases.empty());
}

// ========================================
// Reuse across documents
// ========================================

TEST_CASE("YamlParseContext does not leak document symbols across reset", "[job_yaml][parse_context]")
{
    YamlParseContext context;

    context.reset("first"sv);

    REQUIRE(context.anchors.reserve("alpha"));
    REQUIRE(context.aliases.push("beta"));

    context.reset("second"sv);

    REQUIRE_FALSE(context.anchors.contains("alpha"));
    REQUIRE_FALSE(context.aliases.contains("beta"));

    REQUIRE(context.anchors.reserve("alpha"));
    REQUIRE(context.aliases.push("beta"));
}

///////
/// DIAGONISTIC

TEST_CASE("YamlParseContext default diagnostic is empty", "[job_yaml][parse_context][diagnostic]")
{
    const YamlParseContext context;

    REQUIRE_FALSE(context.diagnostic.valid());
    REQUIRE(context.diagnostic.kind() == YamlDiagnosticKind::None);
}

TEST_CASE("YamlParseContext exposes diagnostic state", "[job_yaml][parse_context][diagnostic]")
{
    YamlParseContext context;

    context.reset("alpha: beta"sv);
    context.diagnostic.set(
        YamlDiagnosticKind::UnexpectedToken,
        {
            .offset = 7,
            .size = 4
        },
        context.source);

    REQUIRE(context.diagnostic.valid());
    REQUIRE(context.diagnostic.kind() == YamlDiagnosticKind::UnexpectedToken);
    REQUIRE(context.diagnostic.offset() == 7);
    REQUIRE(context.diagnostic.range().size == 4);
}

TEST_CASE("YamlParseContext clearDocument preserves source and diagnostic",
          "[job_yaml][parse_context][document][diagnostic]")
{
    YamlParseContext context;

    context.reset("alpha: beta"sv);

    REQUIRE(context.anchors.reserve("anchor"));
    REQUIRE(context.aliases.push("alias"));

    context.diagnostic.set(
        YamlDiagnosticKind::InvalidAlias,
        {
            .offset = 7,
            .size = 4
        },
        context.source);

    context.clearDocument();

    REQUIRE(context.source == "alpha: beta");
    REQUIRE(context.hasSource());

    REQUIRE(context.anchors.empty());
    REQUIRE_FALSE(context.anchors.contains("anchor"));

    REQUIRE(context.aliases.empty());
    REQUIRE_FALSE(context.aliases.contains("alias"));

    REQUIRE(context.diagnostic.valid());
    REQUIRE(context.diagnostic.kind() == YamlDiagnosticKind::InvalidAlias);
    REQUIRE(context.diagnostic.offset() == 7);
    REQUIRE(context.diagnostic.range().size == 4);
}

TEST_CASE("YamlParseContext clear resets diagnostic state",
          "[job_yaml][parse_context][diagnostic]")
{
    YamlParseContext context;

    context.reset("alpha: beta"sv);

    context.diagnostic.set(
        YamlDiagnosticKind::InvalidDocument,
        {
            .offset = 0,
            .size = 5
        },
        context.source);

    REQUIRE(context.diagnostic.valid());

    context.clear();

    REQUIRE_FALSE(context.diagnostic.valid());
    REQUIRE(context.diagnostic.kind() == YamlDiagnosticKind::None);
    REQUIRE(context.diagnostic.range().empty());
    REQUIRE(context.diagnostic.line() == 0);
    REQUIRE(context.diagnostic.column() == 0);
}

TEST_CASE("YamlParseContext reset clears previous diagnostic state",
          "[job_yaml][parse_context][diagnostic]")
{
    YamlParseContext context;

    context.reset("first"sv);

    context.diagnostic.set(
        YamlDiagnosticKind::InvalidScalar,
        {
            .offset = 0,
            .size = 5
        },
        context.source);

    REQUIRE(context.diagnostic.valid());

    context.reset("second"sv);

    REQUIRE(context.source == "second");
    REQUIRE_FALSE(context.diagnostic.valid());
    REQUIRE(context.diagnostic.kind() == YamlDiagnosticKind::None);
}

TEST_CASE("YamlParseContext clearDocument can begin a new document without clearing diagnostic",
          "[job_yaml][parse_context][document][diagnostic]")
{
    YamlParseContext context;

    context.reset("alpha\nbeta"sv);

    REQUIRE(context.anchors.reserve("first"));
    REQUIRE(context.aliases.push("first"));

    context.diagnostic.set(
        YamlDiagnosticKind::UnexpectedToken,
        {
            .offset = 6,
            .size = 4
        },
        context.source);

    context.clearDocument();

    REQUIRE(context.anchors.empty());
    REQUIRE(context.aliases.empty());

    REQUIRE(context.diagnostic.valid());
    REQUIRE(context.diagnostic.kind() == YamlDiagnosticKind::UnexpectedToken);

    REQUIRE(context.anchors.reserve("second"));
    REQUIRE(context.aliases.push("second"));

    REQUIRE(context.anchors.contains("second"));
    REQUIRE(context.aliases.contains("second"));
}




} // namespace job::yaml::tests