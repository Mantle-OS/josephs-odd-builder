#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "job_yaml_diagnostic.h"

namespace job::yaml::tests {

struct YamlMutationFixture
{
    std::string_view name{};
    std::string_view production{};
    std::string_view mutation{};
    std::string_view source{};
    YamlDiagnosticKind kind{YamlDiagnosticKind::None};
    std::size_t offset{};
    std::size_t line{};
    std::size_t column{};
    std::size_t rangeSize{};
};

inline constexpr std::array<YamlMutationFixture, 4> DeleteTokenMutationFixtures = {{
    {
        .name = std::string_view{"delete_flow_sequence_closer", 27},
        .production = std::string_view{"c-flow-sequence", 15},
        .mutation = std::string_view{"DeleteToken", 11},
        .source = std::string_view{"[alpha, beta", 12},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 0
    },
    {
        .name = std::string_view{"delete_flow_mapping_closer", 26},
        .production = std::string_view{"c-flow-mapping", 14},
        .mutation = std::string_view{"DeleteToken", 11},
        .source = std::string_view{"{name: alpha", 12},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 0
    },
    {
        .name = std::string_view{"delete_anchor_name", 18},
        .production = std::string_view{"c-ns-anchor-property", 20},
        .mutation = std::string_view{"DeleteToken", 11},
        .source = std::string_view{"base: &\n", 8},
        .kind = YamlDiagnosticKind::InvalidAnchor,
        .offset = 6,
        .line = 1,
        .column = 7,
        .rangeSize = 1
    },
    {
        .name = std::string_view{"delete_alias_name", 17},
        .production = std::string_view{"c-ns-alias-node", 15},
        .mutation = std::string_view{"DeleteToken", 11},
        .source = std::string_view{"copy: *\n", 8},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 6,
        .line = 1,
        .column = 7,
        .rangeSize = 1
    },
}};

inline constexpr std::array<YamlMutationFixture, 2> DuplicateTokenMutationFixtures = {{
    {
        .name = std::string_view{"duplicate_flow_sequence_comma", 29},
        .production = std::string_view{"c-flow-sequence", 15},
        .mutation = std::string_view{"DuplicateToken", 14},
        .source = std::string_view{"[alpha,, beta]\n", 15},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 1
    },
    {
        .name = std::string_view{"duplicate_flow_mapping_comma", 28},
        .production = std::string_view{"c-flow-mapping", 14},
        .mutation = std::string_view{"DuplicateToken", 14},
        .source = std::string_view{"{name: alpha,, count: 42}\n", 26},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 13,
        .line = 1,
        .column = 14,
        .rangeSize = 1
    },
}};

inline constexpr std::array<YamlMutationFixture, 2> ReplaceDelimiterMutationFixtures = {{
    {
        .name = std::string_view{"replace_flow_sequence_closer", 28},
        .production = std::string_view{"c-flow-sequence", 15},
        .mutation = std::string_view{"ReplaceDelimiter", 16},
        .source = std::string_view{"[alpha, beta}\n", 14},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 1
    },
    {
        .name = std::string_view{"replace_flow_mapping_closer", 27},
        .production = std::string_view{"c-flow-mapping", 14},
        .mutation = std::string_view{"ReplaceDelimiter", 16},
        .source = std::string_view{"{name: alpha]\n", 14},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 1
    },
}};

inline constexpr std::array<YamlMutationFixture, 3> TruncateMutationFixtures = {{
    {
        .name = std::string_view{"truncate_flow_sequence", 22},
        .production = std::string_view{"c-flow-sequence", 15},
        .mutation = std::string_view{"Truncate", 8},
        .source = std::string_view{"[alpha, beta", 12},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 0
    },
    {
        .name = std::string_view{"truncate_flow_mapping", 21},
        .production = std::string_view{"c-flow-mapping", 14},
        .mutation = std::string_view{"Truncate", 8},
        .source = std::string_view{"{name: alpha", 12},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 0
    },
    {
        .name = std::string_view{"truncate_double_quoted_scalar", 29},
        .production = std::string_view{"c-double-quoted", 15},
        .mutation = std::string_view{"Truncate", 8},
        .source = std::string_view{"value: \"alpha", 13},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 6
    },
}};

inline constexpr std::array<YamlMutationFixture, 4> IndentationMutationFixtures = {{
    {
        .name = std::string_view{"illegal_mapping_deeper_indent", 29},
        .production = std::string_view{"l+block-mapping", 15},
        .mutation = std::string_view{"IllegalIndentation", 18},
        .source = std::string_view{"first: alpha\n  second: beta\n", 28},
        .kind = YamlDiagnosticKind::InvalidIndentation,
        .offset = 15,
        .line = 2,
        .column = 3,
        .rangeSize = 6
    },
    {
        .name = std::string_view{"illegal_mapping_dedent", 22},
        .production = std::string_view{"l+block-mapping", 15},
        .mutation = std::string_view{"IllegalIndentation", 18},
        .source = std::string_view{"root:\n  first: alpha\n second: beta\n", 35},
        .kind = YamlDiagnosticKind::InvalidIndentation,
        .offset = 22,
        .line = 3,
        .column = 2,
        .rangeSize = 6
    },
    {
        .name = std::string_view{"illegal_sequence_deeper_indent", 30},
        .production = std::string_view{"l+block-sequence", 16},
        .mutation = std::string_view{"IllegalIndentation", 18},
        .source = std::string_view{"- alpha\n  - beta\n", 17},
        .kind = YamlDiagnosticKind::InvalidIndentation,
        .offset = 10,
        .line = 2,
        .column = 3,
        .rangeSize = 1
    },
    {
        .name = std::string_view{"illegal_block_scalar_tab_indent", 31},
        .production = std::string_view{"c-l+literal", 11},
        .mutation = std::string_view{"IllegalIndentation", 18},
        .source = std::string_view{"value: |\n\talpha\n", 16},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 9,
        .line = 2,
        .column = 1,
        .rangeSize = 1
    },
}};

inline constexpr std::array<YamlMutationFixture, 4> RemoveCloserMutationFixtures = {{
    {
        .name = std::string_view{"remove_single_quote_closer", 26},
        .production = std::string_view{"c-single-quoted", 15},
        .mutation = std::string_view{"RemoveCloser", 12},
        .source = std::string_view{"value: 'alpha", 13},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 6
    },
    {
        .name = std::string_view{"remove_double_quote_closer", 26},
        .production = std::string_view{"c-double-quoted", 15},
        .mutation = std::string_view{"RemoveCloser", 12},
        .source = std::string_view{"value: \"alpha", 13},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 6
    },
    {
        .name = std::string_view{"remove_flow_sequence_closer", 27},
        .production = std::string_view{"c-flow-sequence", 15},
        .mutation = std::string_view{"RemoveCloser", 12},
        .source = std::string_view{"[alpha, beta", 12},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 0
    },
    {
        .name = std::string_view{"remove_flow_mapping_closer", 26},
        .production = std::string_view{"c-flow-mapping", 14},
        .mutation = std::string_view{"RemoveCloser", 12},
        .source = std::string_view{"{name: alpha", 12},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 12,
        .line = 1,
        .column = 13,
        .rangeSize = 0
    },
}};

inline constexpr std::array<YamlMutationFixture, 5> AnchorAliasMutationFixtures = {{
    {
        .name = std::string_view{"break_anchor_empty_name", 23},
        .production = std::string_view{"c-ns-anchor-property", 20},
        .mutation = std::string_view{"BreakAnchor", 11},
        .source = std::string_view{"base: &\n", 8},
        .kind = YamlDiagnosticKind::InvalidAnchor,
        .offset = 6,
        .line = 1,
        .column = 7,
        .rangeSize = 1
    },
    {
        .name = std::string_view{"break_alias_empty_name", 22},
        .production = std::string_view{"c-ns-alias-node", 15},
        .mutation = std::string_view{"BreakAlias", 10},
        .source = std::string_view{"copy: *\n", 8},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 6,
        .line = 1,
        .column = 7,
        .rangeSize = 1
    },
    {
        .name = std::string_view{"break_alias_unknown_name", 24},
        .production = std::string_view{"c-ns-alias-node", 15},
        .mutation = std::string_view{"BreakAlias", 10},
        .source = std::string_view{"base: &shared alpha\ncopy: *missing\n", 35},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 26,
        .line = 2,
        .column = 7,
        .rangeSize = 8
    },
    {
        .name = std::string_view{"duplicate_anchor_name", 21},
        .production = std::string_view{"c-ns-anchor-property", 20},
        .mutation = std::string_view{"DuplicateAnchor", 15},
        .source = std::string_view{"first: &shared alpha\nsecond: &shared beta\n", 42},
        .kind = YamlDiagnosticKind::InvalidAnchor,
        .offset = 29,
        .line = 2,
        .column = 9,
        .rangeSize = 7
    },
    {
        .name = std::string_view{"break_alias_recursive_reference", 31},
        .production = std::string_view{"c-ns-alias-node", 15},
        .mutation = std::string_view{"BreakAlias", 10},
        .source = std::string_view{"value: &self *self\n", 19},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 13,
        .line = 1,
        .column = 14,
        .rangeSize = 5
    },
}};

inline constexpr std::array<YamlMutationFixture, 4> DocumentMarkerMutationFixtures = {{
    {
        .name = std::string_view{"duplicate_document_start", 24},
        .production = std::string_view{"c-directives-end", 16},
        .mutation = std::string_view{"DuplicateDocumentMarker", 23},
        .source = std::string_view{"---\nalpha\n---\nbeta\n", 19},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 10,
        .line = 3,
        .column = 1,
        .rangeSize = 3
    },
    {
        .name = std::string_view{"move_document_start_after_content", 33},
        .production = std::string_view{"c-directives-end", 16},
        .mutation = std::string_view{"MoveDocumentMarker", 18},
        .source = std::string_view{"alpha\n---\nbeta\n", 15},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 6,
        .line = 2,
        .column = 1,
        .rangeSize = 3
    },
    {
        .name = std::string_view{"content_after_document_end", 26},
        .production = std::string_view{"c-document-end", 14},
        .mutation = std::string_view{"MoveDocumentMarker", 18},
        .source = std::string_view{"---\nalpha\n...\nbeta\n", 19},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 14,
        .line = 4,
        .column = 1,
        .rangeSize = 4
    },
    {
        .name = std::string_view{"directive_after_content", 23},
        .production = std::string_view{"l-directive", 11},
        .mutation = std::string_view{"MoveDirective", 13},
        .source = std::string_view{"alpha\n%YAML 1.2\n---\nbeta\n", 25},
        .kind = YamlDiagnosticKind::InvalidDirective,
        .offset = 6,
        .line = 2,
        .column = 1,
        .rangeSize = 5
    },
}};

} // namespace job::yaml::tests
