#pragma once

#include <array>
#include <string_view>

namespace job::yaml::tests {

enum class YamlFlowFixtureKind {
    Sequence,
    Mapping
};

struct YamlFlowFixture
{
    std::string_view name;
    std::string_view source;
    std::string_view production;
    YamlFlowFixtureKind kind;
    bool valid;
};

inline constexpr std::array<YamlFlowFixture, 49> FlowFixtures{
    {
     {
         .name = "empty_flow_sequence",
         .source = "[]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "single_plain_flow_sequence",
         .source = "[alpha]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "multiple_plain_flow_sequence",
         .source = "[alpha, beta, gamma]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "single_quoted_flow_sequence",
         .source = "['alpha', 'beta']",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "double_quoted_flow_sequence",
         .source = "[\"alpha\", \"beta\"]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "mixed_scalar_flow_sequence",
         .source = "[alpha, 'beta', \"gamma\"]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "flow_sequence_with_spaces",
         .source = "[ alpha, beta, gamma ]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "flow_sequence_with_trailing_comma",
         .source = "[alpha, beta,]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "multiline_flow_sequence",
         .source = "[\n  alpha,\n  beta,\n  gamma\n]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
         .name = "flow_sequence_with_comments",
         .source = "[alpha, # first\n beta, # second\n gamma]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
      .name = "empty_flow_mapping",
      .source = "{}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "single_flow_mapping_entry",
      .source = "{name: alpha}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "multiple_flow_mapping_entries",
      .source = "{name: alpha, count: 42}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "single_quoted_flow_mapping",
      .source = "{'name': 'alpha'}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "double_quoted_flow_mapping",
      .source = "{\"name\": \"alpha\"}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "mixed_scalar_flow_mapping",
      .source = "{plain: alpha, single: 'beta', double: \"gamma\"}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "flow_mapping_with_spaces",
      .source = "{ name: alpha, count: 42 }",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "flow_mapping_with_trailing_comma",
      .source = "{name: alpha, count: 42,}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "multiline_flow_mapping",
      .source = "{\n  name: alpha,\n  count: 42\n}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "flow_mapping_with_comments",
      .source = "{name: alpha, # name\n count: 42}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
         .name = "recursive_sequence_depth_1",
         .source = "[alpha, 'beta']",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
      .name = "recursive_mapping_depth_1",
      .source = "{left: alpha, right: 'beta'}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "recursive_sequence_mapping_depth_1",
      .source = "[[alpha, 'beta'], {left: alpha, right: 'beta'}]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "recursive_mapping_sequence_depth_1",
      .source = "{sequence: [alpha, 'beta'], mapping: {left: alpha, right: 'beta'}}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "recursive_sequence_depth_2",
      .source = "[{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "recursive_mapping_depth_2",
      .source = "{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "recursive_sequence_mapping_depth_2",
      .source = "[[{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]], {left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "recursive_mapping_sequence_depth_2",
      .source = "{sequence: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]], mapping: {left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "recursive_sequence_depth_3",
      .source = "[[{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}, [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]], {nested: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]}]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "recursive_mapping_depth_3",
      .source = "{left: [{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}, [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]], right: {nested: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]}}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "recursive_sequence_mapping_depth_3",
      .source = "[[[{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}, [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]], {nested: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]}], {left: [{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}, [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]], right: {nested: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]}}]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "recursive_mapping_sequence_depth_3",
      .source = "{sequence: [[{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}, [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]], {nested: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]}], mapping: {left: [{left: {left: alpha, right: 'beta'}, right: [{left: alpha, right: 'beta'}, [alpha, 'beta']]}, [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]], right: {nested: [{left: alpha, right: 'beta'}, [{left: alpha, right: 'beta'}, [alpha, 'beta']]]}}}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "sequence_containing_mapping",
      .source = "[alpha, {name: beta}]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "mapping_containing_sequence",
      .source = "{items: [alpha, beta]}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
         .name = "sequence_containing_sequences",
         .source = "[[alpha, beta], [gamma, delta]]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = true,
     },
     {
      .name = "mapping_containing_mappings",
      .source = "{outer: {inner: alpha}}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
      .name = "deep_mixed_flow_sequence",
      .source = "[alpha, {items: [beta, {value: gamma}]}]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = true,
      },
     {
      .name = "deep_mixed_flow_mapping",
      .source = "{outer: [alpha, {inner: [beta, gamma]}]}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = true,
      },
     {
         .name = "unterminated_flow_sequence",
         .source = "[alpha, beta",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = false,
     },
     {
      .name = "unterminated_flow_mapping",
      .source = "{name: alpha",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = false,
      },
     {
         .name = "flow_sequence_closed_by_mapping_delimiter",
         .source = "[alpha, beta}",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = false,
     },
     {
      .name = "flow_mapping_closed_by_sequence_delimiter",
      .source = "{name: alpha]",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = false,
      },
     {
         .name = "duplicate_flow_sequence_separator",
         .source = "[alpha,, beta]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = false,
     },
     {
      .name = "duplicate_flow_mapping_separator",
      .source = "{name: alpha,, count: 42}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = false,
      },
     {
      .name = "missing_flow_mapping_separator",
      .source = "{name alpha}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = false,
      },
     {
      .name = "missing_flow_mapping_value",
      .source = "{name:}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = false,
      },
     {
      .name = "nested_unterminated_flow_sequence",
      .source = "{items: [alpha, beta}",
      .production = "c-flow-mapping",
      .kind = YamlFlowFixtureKind::Mapping,
      .valid = false,
      },
     {
      .name = "nested_unterminated_flow_mapping",
      .source = "[alpha, {name: beta]",
      .production = "c-flow-sequence",
      .kind = YamlFlowFixtureKind::Sequence,
      .valid = false,
      },
     {
         .name = "nested_mismatched_flow_delimiter",
         .source = "[alpha, {name: beta]]",
         .production = "c-flow-sequence",
         .kind = YamlFlowFixtureKind::Sequence,
         .valid = false,
         },
     }
};
} // namespace job::yaml::tests
