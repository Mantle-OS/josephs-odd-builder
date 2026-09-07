#include <catch2/catch_test_macros.hpp>

#include <job_yaml_anchor_table.h>

#include <cstddef>
#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Default state
// ========================================

TEST_CASE("YamlAnchorTable default constructs empty", "[job_yaml][anchor_table]")
{
    const YamlAnchorTable table;

    REQUIRE(table.empty());
    REQUIRE(table.size() == 0);
    REQUIRE_FALSE(table.contains("alpha"));
    REQUIRE(table.find("alpha") == nullptr);
}

// ========================================
// Reserve
// ========================================

TEST_CASE("YamlAnchorTable reserves anchor", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));

    REQUIRE_FALSE(table.empty());
    REQUIRE(table.size() == 1);
    REQUIRE(table.contains("alpha"));

    const auto *entry = table.find("alpha");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->name == "alpha");
    REQUIRE(entry->range.offset == 0);
    REQUIRE(entry->range.size == 0);
    REQUIRE(entry->indent == 0);
    REQUIRE_FALSE(entry->isComplete());
}

TEST_CASE("YamlAnchorTable rejects empty anchor name", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE_FALSE(table.reserve({}));

    REQUIRE(table.empty());
    REQUIRE(table.size() == 0);
}

TEST_CASE("YamlAnchorTable rejects duplicate anchor", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));
    REQUIRE_FALSE(table.reserve("alpha"));

    REQUIRE(table.size() == 1);
}

TEST_CASE("YamlAnchorTable stores multiple anchors", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));
    REQUIRE(table.reserve("beta"));
    REQUIRE(table.reserve("gamma"));

    REQUIRE(table.size() == 3);
    REQUIRE(table.contains("alpha"));
    REQUIRE(table.contains("beta"));
    REQUIRE(table.contains("gamma"));
}

TEST_CASE("YamlAnchorTable anchor name is a borrowed view", "[job_yaml][anchor_table]")
{
    constexpr std::string_view source = "alpha"sv;

    YamlAnchorTable table;

    REQUIRE(table.reserve(source));

    const auto *entry = table.find("alpha");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->name.data() == source.data());
    REQUIRE(entry->name.size() == source.size());
}

// ========================================
// Complete
// ========================================

TEST_CASE("YamlAnchorTable completes reserved anchor", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));

    const YamlSourceRange range{
        .offset = 12,
        .size = 24
    };

    REQUIRE(table.complete("alpha", range, 4));

    const auto *entry = table.find("alpha");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->isComplete());
    REQUIRE(entry->range.offset == 12);
    REQUIRE(entry->range.size == 24);
    REQUIRE(entry->indent == 4);
}

TEST_CASE("YamlAnchorTable preserves completed node range", "[job_yaml][anchor_table]")
{
    constexpr std::string_view source = "prefix-alpha-node-suffix"sv;

    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));
    REQUIRE(table.complete("alpha", {
                                        .offset = 7,
                                        .size = 10
                                    }, 2));

    const auto *entry = table.find("alpha");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->range.view(source) == "alpha-node");
}

TEST_CASE("YamlAnchorTable rejects completion of unknown anchor", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE_FALSE(table.complete("alpha", {
                                              .offset = 8,
                                              .size = 16
                                          }, 2));

    REQUIRE(table.empty());
}

TEST_CASE("YamlAnchorTable rejects second completion", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));

    REQUIRE(table.complete("alpha", {
                                        .offset = 8,
                                        .size = 16
                                    }, 2));

    REQUIRE_FALSE(table.complete("alpha", {
                                              .offset = 32,
                                              .size = 64
                                          }, 8));

    const auto *entry = table.find("alpha");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->range.offset == 8);
    REQUIRE(entry->range.size == 16);
    REQUIRE(entry->indent == 2);
}

TEST_CASE("YamlAnchorTable completes anchors independently", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));
    REQUIRE(table.reserve("beta"));

    REQUIRE(table.complete("alpha", {
                                        .offset = 4,
                                        .size = 8
                                    }, 2));

    const auto *alpha = table.find("alpha");
    const auto *beta = table.find("beta");

    REQUIRE(alpha != nullptr);
    REQUIRE(beta != nullptr);

    REQUIRE(alpha->isComplete());
    REQUIRE(alpha->range.offset == 4);
    REQUIRE(alpha->range.size == 8);
    REQUIRE(alpha->indent == 2);

    REQUIRE_FALSE(beta->isComplete());
    REQUIRE(beta->range.offset == 0);
    REQUIRE(beta->range.size == 0);
    REQUIRE(beta->indent == 0);
}

// ========================================
// Lookup
// ========================================

TEST_CASE("YamlAnchorTable find returns matching entry", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));
    REQUIRE(table.reserve("beta"));

    const auto *entry = table.find("beta");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->name == "beta");
}

TEST_CASE("YamlAnchorTable find returns null for missing anchor", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));

    REQUIRE(table.find("beta") == nullptr);
}

TEST_CASE("YamlAnchorTable contains reports reserved anchors", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));

    REQUIRE(table.contains("alpha"));
    REQUIRE_FALSE(table.contains("beta"));
}

// ========================================
// Clear
// ========================================

TEST_CASE("YamlAnchorTable clear removes all anchors", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));
    REQUIRE(table.reserve("beta"));

    REQUIRE(table.complete("alpha", {
                                        .offset = 4,
                                        .size = 8
                                    }, 2));

    table.clear();

    REQUIRE(table.empty());
    REQUIRE(table.size() == 0);
    REQUIRE_FALSE(table.contains("alpha"));
    REQUIRE_FALSE(table.contains("beta"));
    REQUIRE(table.find("alpha") == nullptr);
    REQUIRE(table.find("beta") == nullptr);
}

TEST_CASE("YamlAnchorTable can be reused after clear", "[job_yaml][anchor_table]")
{
    YamlAnchorTable table;

    REQUIRE(table.reserve("alpha"));

    table.clear();

    REQUIRE(table.reserve("alpha"));
    REQUIRE(table.size() == 1);
    REQUIRE(table.contains("alpha"));
}

// ========================================
// Entry state
// ========================================

TEST_CASE("YamlAnchorEntry reports completion state", "[job_yaml][anchor_table]")
{
    constexpr YamlAnchorEntry incomplete{
        .name = "alpha"sv,
        .range = {},
        .indent = 0,
        .complete = false
    };

    constexpr YamlAnchorEntry complete{
        .name = "beta"sv,
        .range = {
            .offset = 4,
            .size = 8
        },
        .indent = 2,
        .complete = true
    };

    STATIC_REQUIRE_FALSE(incomplete.isComplete());
    STATIC_REQUIRE(complete.isComplete());
}

} // namespace job::yaml::tests