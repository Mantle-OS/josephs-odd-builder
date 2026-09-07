#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "job_yaml_diagnostic.h"

namespace job::yaml::tests {

struct YamlDiagnosticFixture
{
    std::string_view name{};
    std::string_view production{};
    std::string_view source{};
    YamlDiagnosticKind kind{YamlDiagnosticKind::None};
    std::size_t offset{};
    std::size_t line{};
    std::size_t column{};
    std::size_t rangeSize{};
    bool valid{};
};

inline constexpr std::array<YamlDiagnosticFixture, 13> DocumentDiagnosticFixtures = {{
    {
        .name = std::string_view{"bare_document", 13},
        .production = std::string_view{"l-bare-document", 15},
        .source = std::string_view{"alpha\n", 6},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"explicit_document", 17},
        .production = std::string_view{"l-explicit-document", 19},
        .source = std::string_view{"---\nalpha\n", 10},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"explicit_empty_document", 23},
        .production = std::string_view{"l-explicit-document", 19},
        .source = std::string_view{"---\n", 4},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"document_prefix_comments", 24},
        .production = std::string_view{"l-document-prefix", 17},
        .source = std::string_view{"# before\n# document\n---\nalpha\n", 30},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"document_suffix", 15},
        .production = std::string_view{"l-document-suffix", 17},
        .source = std::string_view{"---\nalpha\n...\n", 14},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"document_suffix_comment", 23},
        .production = std::string_view{"l-document-suffix", 17},
        .source = std::string_view{"---\nalpha\n... # done\n", 21},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"directive_document_structural", 29},
        .production = std::string_view{"l-directive-document", 20},
        .source = std::string_view{"%YAML 1.2\n---\nalpha\n", 20},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"directives_end_same_line_content", 32},
        .production = std::string_view{"c-directives-end", 16},
        .source = std::string_view{"--- alpha\n", 10},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"marker_lookalike_directives_end", 31},
        .production = std::string_view{"c-forbidden", 11},
        .source = std::string_view{"---alpha\n", 9},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"marker_lookalike_document_end", 29},
        .production = std::string_view{"c-forbidden", 11},
        .source = std::string_view{"...alpha\n", 9},
        .kind = YamlDiagnosticKind::None,
        .offset = 0,
        .line = 0,
        .column = 0,
        .rangeSize = 0,
        .valid = true
    },
    {
        .name = std::string_view{"second_explicit_document", 24},
        .production = std::string_view{"c-directives-end", 16},
        .source = std::string_view{"---\nalpha\n---\nbeta\n", 19},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 10,
        .line = 3,
        .column = 1,
        .rangeSize = 3,
        .valid = false
    },
    {
        .name = std::string_view{"content_after_document_end", 26},
        .production = std::string_view{"c-document-end", 14},
        .source = std::string_view{"---\nalpha\n...\nbeta\n", 19},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 14,
        .line = 4,
        .column = 1,
        .rangeSize = 4,
        .valid = false
    },
    {
        .name = std::string_view{"second_document_after_suffix", 28},
        .production = std::string_view{"c-document-end", 14},
        .source = std::string_view{"---\nalpha\n...\n---\nbeta\n", 23},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 14,
        .line = 4,
        .column = 1,
        .rangeSize = 3,
        .valid = false
    },
}};

inline constexpr std::array<YamlDiagnosticFixture, 3> DirectiveDiagnosticFixtures = {{
    {
        .name = std::string_view{"directive_without_explicit_document", 35},
        .production = std::string_view{"l-directive-document", 20},
        .source = std::string_view{"%YAML 1.2\nalpha\n", 16},
        .kind = YamlDiagnosticKind::InvalidDirective,
        .offset = 10,
        .line = 2,
        .column = 1,
        .rangeSize = 5,
        .valid = false
    },
    {
        .name = std::string_view{"directive_after_document_content", 32},
        .production = std::string_view{"l-directive", 11},
        .source = std::string_view{"alpha\n%YAML 1.2\n---\nbeta\n", 25},
        .kind = YamlDiagnosticKind::InvalidDirective,
        .offset = 6,
        .line = 2,
        .column = 1,
        .rangeSize = 5,
        .valid = false
    },
    {
        .name = std::string_view{"directive_after_document_end", 28},
        .production = std::string_view{"l-directive", 11},
        .source = std::string_view{"---\nalpha\n...\n%YAML 1.2\n---\nbeta\n", 33},
        .kind = YamlDiagnosticKind::InvalidDocument,
        .offset = 14,
        .line = 4,
        .column = 1,
        .rangeSize = 5,
        .valid = false
    },
}};

inline constexpr std::array<YamlDiagnosticFixture, 8> PropertyDiagnosticFixtures = {{
    {
        .name = std::string_view{"empty_anchor_name", 17},
        .production = std::string_view{"c-ns-anchor-property", 20},
        .source = std::string_view{"value: &\n", 9},
        .kind = YamlDiagnosticKind::InvalidAnchor,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"duplicate_anchor_property", 25},
        .production = std::string_view{"c-ns-properties", 15},
        .source = std::string_view{"value: &first &second alpha\n", 28},
        .kind = YamlDiagnosticKind::InvalidAnchor,
        .offset = 14,
        .line = 1,
        .column = 15,
        .rangeSize = 7,
        .valid = false
    },
    {
        .name = std::string_view{"duplicate_anchor_name", 21},
        .production = std::string_view{"c-ns-anchor-property", 20},
        .source = std::string_view{"first: &shared alpha\nsecond: &shared beta\n", 42},
        .kind = YamlDiagnosticKind::InvalidAnchor,
        .offset = 29,
        .line = 2,
        .column = 9,
        .rangeSize = 7,
        .valid = false
    },
    {
        .name = std::string_view{"empty_alias_name", 16},
        .production = std::string_view{"c-ns-alias-node", 15},
        .source = std::string_view{"value: *\n", 9},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"unknown_alias", 13},
        .production = std::string_view{"c-ns-alias-node", 15},
        .source = std::string_view{"value: *missing\n", 16},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 8,
        .valid = false
    },
    {
        .name = std::string_view{"recursive_alias", 15},
        .production = std::string_view{"c-ns-alias-node", 15},
        .source = std::string_view{"value: &self *self\n", 19},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 13,
        .line = 1,
        .column = 14,
        .rangeSize = 5,
        .valid = false
    },
    {
        .name = std::string_view{"duplicate_tag_property", 22},
        .production = std::string_view{"c-ns-properties", 15},
        .source = std::string_view{"value: !first !second alpha\n", 28},
        .kind = YamlDiagnosticKind::InvalidTag,
        .offset = 14,
        .line = 1,
        .column = 15,
        .rangeSize = 7,
        .valid = false
    },
    {
        .name = std::string_view{"properties_on_alias", 19},
        .production = std::string_view{"c-ns-properties", 15},
        .source = std::string_view{"base: &base alpha\nvalue: !tag *base\n", 36},
        .kind = YamlDiagnosticKind::InvalidAlias,
        .offset = 30,
        .line = 2,
        .column = 13,
        .rangeSize = 5,
        .valid = false
    },
}};

inline constexpr std::array<YamlDiagnosticFixture, 7> ScalarDiagnosticFixtures = {{
    {
        .name = std::string_view{"single_quoted_unterminated", 26},
        .production = std::string_view{"c-single-quoted", 15},
        .source = std::string_view{"value: 'alpha\n", 14},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 6,
        .valid = false
    },
    {
        .name = std::string_view{"double_quoted_unterminated", 26},
        .production = std::string_view{"c-double-quoted", 15},
        .source = std::string_view{"value: \"alpha\n", 14},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 6,
        .valid = false
    },
    {
        .name = std::string_view{"double_quoted_invalid_escape", 28},
        .production = std::string_view{"c-ns-esc-char", 13},
        .source = std::string_view{"value: \"\\q\"\n", 12},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 4,
        .valid = false
    },
    {
        .name = std::string_view{"double_quoted_invalid_unicode_short", 35},
        .production = std::string_view{"ns-esc-16-bit", 13},
        .source = std::string_view{"value: \"\\u123\"\n", 15},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 7,
        .valid = false
    },
    {
        .name = std::string_view{"double_quoted_invalid_unicode_digit", 35},
        .production = std::string_view{"ns-esc-16-bit", 13},
        .source = std::string_view{"value: \"\\u12G4\"\n", 16},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 8,
        .valid = false
    },
    {
        .name = std::string_view{"double_quoted_invalid_unicode_surrogate", 39},
        .production = std::string_view{"ns-esc-16-bit", 13},
        .source = std::string_view{"value: \"\\uD800\"\n", 16},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 8,
        .valid = false
    },
    {
        .name = std::string_view{"double_quoted_invalid_unicode_range", 35},
        .production = std::string_view{"ns-esc-32-bit", 13},
        .source = std::string_view{"value: \"\\U00110000\"\n", 20},
        .kind = YamlDiagnosticKind::InvalidScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 12,
        .valid = false
    },
}};

inline constexpr std::array<YamlDiagnosticFixture, 9> FlowDiagnosticFixtures = {{
    {
        .name = std::string_view{"unterminated_flow_sequence", 26},
        .production = std::string_view{"c-flow-sequence", 15},
        .source = std::string_view{"value: [alpha, beta", 19},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 19,
        .line = 1,
        .column = 20,
        .rangeSize = 0,
        .valid = false
    },
    {
        .name = std::string_view{"unterminated_flow_mapping", 25},
        .production = std::string_view{"c-flow-mapping", 14},
        .source = std::string_view{"value: {name: alpha", 19},
        .kind = YamlDiagnosticKind::UnexpectedEnd,
        .offset = 19,
        .line = 1,
        .column = 20,
        .rangeSize = 0,
        .valid = false
    },
    {
        .name = std::string_view{"flow_sequence_closed_by_mapping_delimiter", 41},
        .production = std::string_view{"c-flow-sequence", 15},
        .source = std::string_view{"value: [alpha, beta}\n", 21},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 19,
        .line = 1,
        .column = 20,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"flow_mapping_closed_by_sequence_delimiter", 41},
        .production = std::string_view{"c-flow-mapping", 14},
        .source = std::string_view{"value: {name: alpha]\n", 21},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 19,
        .line = 1,
        .column = 20,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"duplicate_flow_sequence_separator", 33},
        .production = std::string_view{"c-flow-sequence", 15},
        .source = std::string_view{"value: [alpha,, beta]\n", 22},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 14,
        .line = 1,
        .column = 15,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"duplicate_flow_mapping_separator", 32},
        .production = std::string_view{"c-flow-mapping", 14},
        .source = std::string_view{"value: {name: alpha,, count: 42}\n", 33},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 20,
        .line = 1,
        .column = 21,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"missing_flow_mapping_separator", 30},
        .production = std::string_view{"c-flow-mapping", 14},
        .source = std::string_view{"value: {name alpha}\n", 20},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 13,
        .line = 1,
        .column = 14,
        .rangeSize = 5,
        .valid = false
    },
    {
        .name = std::string_view{"nested_unterminated_flow_sequence", 33},
        .production = std::string_view{"c-flow-mapping", 14},
        .source = std::string_view{"value: {items: [alpha, beta}\n", 29},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 27,
        .line = 1,
        .column = 28,
        .rangeSize = 1,
        .valid = false
    },
    {
        .name = std::string_view{"nested_unterminated_flow_mapping", 32},
        .production = std::string_view{"c-flow-sequence", 15},
        .source = std::string_view{"value: [alpha, {name: beta]\n", 28},
        .kind = YamlDiagnosticKind::InvalidFlowCollection,
        .offset = 26,
        .line = 1,
        .column = 27,
        .rangeSize = 1,
        .valid = false
    },
}};

inline constexpr std::array<YamlDiagnosticFixture, 9> BlockScalarDiagnosticFixtures = {{
    {
        .name = std::string_view{"literal_invalid_indent_zero", 27},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"value: |0\n  alpha\n", 18},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 2,
        .valid = false
    },
    {
        .name = std::string_view{"folded_invalid_indent_zero", 26},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"value: >0\n  alpha\n", 18},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 2,
        .valid = false
    },
    {
        .name = std::string_view{"literal_duplicate_indent", 24},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"value: |22\n  alpha\n", 19},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 3,
        .valid = false
    },
    {
        .name = std::string_view{"literal_duplicate_chomping", 26},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"value: |++\n  alpha\n", 19},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 3,
        .valid = false
    },
    {
        .name = std::string_view{"folded_duplicate_chomping", 25},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"value: >--\n  alpha\n", 19},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 3,
        .valid = false
    },
    {
        .name = std::string_view{"literal_invalid_header_character", 32},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"value: |x\n  alpha\n", 18},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 2,
        .valid = false
    },
    {
        .name = std::string_view{"folded_invalid_header_character", 31},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"value: >x\n  alpha\n", 18},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 2,
        .valid = false
    },
    {
        .name = std::string_view{"literal_insufficient_explicit_indent", 36},
        .production = std::string_view{"l-literal-content", 17},
        .source = std::string_view{"value: |3\n  alpha\n", 18},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 2,
        .valid = false
    },
    {
        .name = std::string_view{"folded_insufficient_explicit_indent", 35},
        .production = std::string_view{"l-folded-content", 16},
        .source = std::string_view{"value: >3\n  alpha\n", 18},
        .kind = YamlDiagnosticKind::InvalidBlockScalar,
        .offset = 7,
        .line = 1,
        .column = 8,
        .rangeSize = 2,
        .valid = false
    },
}};

inline constexpr std::array<YamlDiagnosticFixture, 3> IndentationDiagnosticFixtures = {{
    {
        .name = std::string_view{"mapping_unexpected_deeper_indent", 32},
        .production = std::string_view{"l+block-mapping", 15},
        .source = std::string_view{"first: alpha\n  second: beta\n", 28},
        .kind = YamlDiagnosticKind::InvalidIndentation,
        .offset = 15,
        .line = 2,
        .column = 3,
        .rangeSize = 6,
        .valid = false
    },
    {
        .name = std::string_view{"mapping_inconsistent_dedent", 27},
        .production = std::string_view{"l+block-mapping", 15},
        .source = std::string_view{"root:\n  first: alpha\n second: beta\n", 35},
        .kind = YamlDiagnosticKind::InvalidIndentation,
        .offset = 22,
        .line = 3,
        .column = 2,
        .rangeSize = 6,
        .valid = false
    },
    {
        .name = std::string_view{"sequence_unexpected_deeper_indent", 33},
        .production = std::string_view{"l+block-sequence", 16},
        .source = std::string_view{"- alpha\n  - beta\n", 17},
        .kind = YamlDiagnosticKind::InvalidIndentation,
        .offset = 10,
        .line = 2,
        .column = 3,
        .rangeSize = 1,
        .valid = false
    },
}};

} // namespace job::yaml::tests
