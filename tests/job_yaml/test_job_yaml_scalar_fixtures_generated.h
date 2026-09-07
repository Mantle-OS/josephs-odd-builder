#pragma once

#include <array>
#include <string_view>

#include "job_yaml_scalar_style.h"

namespace job::yaml::tests {

struct YamlScalarFixture
{
    std::string_view name{};
    std::string_view production{};
    std::string_view source{};
    std::string_view expected{};
    YamlScalarStyle style{YamlScalarStyle::None};
    bool valid{};
    bool transformed{};
};

inline constexpr std::array<YamlScalarFixture, 7> PlainScalarFixtures =
    {
    {
                                                                          {
                                                                              .name = std::string_view{"plain_simple", 12},
                                                                              .production = std::string_view{"ns-plain-one-line", 17},
                                                                              .source = std::string_view{"alpha", 5},
                                                                              .expected = std::string_view{"alpha", 5},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          {
                                                                              .name = std::string_view{"plain_with_space", 16},
                                                                              .production = std::string_view{"ns-plain-one-line", 17},
                                                                              .source = std::string_view{"alpha beta", 10},
                                                                              .expected = std::string_view{"alpha beta", 10},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          {
                                                                              .name = std::string_view{"plain_colon_without_boundary", 28},
                                                                              .production = std::string_view{"ns-plain-char", 13},
                                                                              .source = std::string_view{"alpha:beta", 10},
                                                                              .expected = std::string_view{"alpha:beta", 10},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          {
                                                                              .name = std::string_view{"plain_hash_without_comment_boundary", 35},
                                                                              .production = std::string_view{"ns-plain-char", 13},
                                                                              .source = std::string_view{"alpha#beta", 10},
                                                                              .expected = std::string_view{"alpha#beta", 10},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          {
                                                                              .name = std::string_view{"plain_numeric_spelling", 22},
                                                                              .production = std::string_view{"ns-plain-one-line", 17},
                                                                              .source = std::string_view{"12345", 5},
                                                                              .expected = std::string_view{"12345", 5},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          {
                                                                              .name = std::string_view{"plain_boolean_spelling", 22},
                                                                              .production = std::string_view{"ns-plain-one-line", 17},
                                                                              .source = std::string_view{"true", 4},
                                                                              .expected = std::string_view{"true", 4},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          {
                                                                              .name = std::string_view{"plain_null_spelling", 19},
                                                                              .production = std::string_view{"ns-plain-one-line", 17},
                                                                              .source = std::string_view{"null", 4},
                                                                              .expected = std::string_view{"null", 4},
                                                                              .style = YamlScalarStyle::Plain,
                                                                              .valid = true,
                                                                              .transformed = false
                                                                          },
                                                                          }};

inline constexpr std::array<YamlScalarFixture, 11> SingleQuotedScalarFixtures = {{
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_simple", 20},
                                                                                      .production = std::string_view{"c-single-quoted", 15},
                                                                                      .source = std::string_view{"'alpha'", 7},
                                                                                      .expected = std::string_view{"alpha", 5},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_spaces", 20},
                                                                                      .production = std::string_view{"c-single-quoted", 15},
                                                                                      .source = std::string_view{"'alpha beta'", 12},
                                                                                      .expected = std::string_view{"alpha beta", 10},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_empty", 19},
                                                                                      .production = std::string_view{"c-single-quoted", 15},
                                                                                      .source = std::string_view{"''", 2},
                                                                                      .expected = std::string_view{"", 0},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_colon", 19},
                                                                                      .production = std::string_view{"nb-single-char", 14},
                                                                                      .source = std::string_view{"'alpha: beta'", 13},
                                                                                      .expected = std::string_view{"alpha: beta", 11},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_hash", 18},
                                                                                      .production = std::string_view{"nb-single-char", 14},
                                                                                      .source = std::string_view{"'alpha # beta'", 14},
                                                                                      .expected = std::string_view{"alpha # beta", 12},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_double_quote", 26},
                                                                                      .production = std::string_view{"nb-single-char", 14},
                                                                                      .source = std::string_view{"'alpha\"beta'", 12},
                                                                                      .expected = std::string_view{"alpha\"beta", 10},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_doubled_quote", 27},
                                                                                      .production = std::string_view{"c-quoted-quote", 14},
                                                                                      .source = std::string_view{"'it''s'", 7},
                                                                                      .expected = std::string_view{"it's", 4},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = true
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_multiple_doubled_quotes", 37},
                                                                                      .production = std::string_view{"c-quoted-quote", 14},
                                                                                      .source = std::string_view{"'one''two''three'", 17},
                                                                                      .expected = std::string_view{"one'two'three", 13},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = true
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_backslash_literal", 31},
                                                                                      .production = std::string_view{"nb-single-char", 14},
                                                                                      .source = std::string_view{"'alpha\\nbeta'", 13},
                                                                                      .expected = std::string_view{"alpha\\nbeta", 11},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = true,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_unterminated", 26},
                                                                                      .production = std::string_view{"c-single-quoted", 15},
                                                                                      .source = std::string_view{"'alpha", 6},
                                                                                      .expected = std::string_view{"", 0},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = false,
                                                                                      .transformed = false
                                                                                  },
                                                                                  {
                                                                                      .name = std::string_view{"single_quoted_invalid_single_quote", 34},
                                                                                      .production = std::string_view{"c-single-quoted", 15},
                                                                                      .source = std::string_view{"'alpha'beta'", 12},
                                                                                      .expected = std::string_view{"", 0},
                                                                                      .style = YamlScalarStyle::SingleQuoted,
                                                                                      .valid = false,
                                                                                      .transformed = false
                                                                                  },
                                                                                  }};

inline constexpr std::array<YamlScalarFixture, 32> DoubleQuotedScalarFixtures = {{
     {
         .name = std::string_view{"double_quoted_simple", 20},
         .production = std::string_view{"c-double-quoted", 15},
         .source = std::string_view{"\"alpha\"", 7},
         .expected = std::string_view{"alpha", 5},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_spaces", 20},
         .production = std::string_view{"c-double-quoted", 15},
         .source = std::string_view{"\"alpha beta\"", 12},
         .expected = std::string_view{"alpha beta", 10},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_empty", 19},
         .production = std::string_view{"c-double-quoted", 15},
         .source = std::string_view{"\"\"", 2},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_single_quote", 26},
         .production = std::string_view{"nb-double-char", 14},
         .source = std::string_view{"\"alpha'beta\"", 12},
         .expected = std::string_view{"alpha'beta", 10},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_colon", 19},
         .production = std::string_view{"nb-double-char", 14},
         .source = std::string_view{"\"alpha: beta\"", 13},
         .expected = std::string_view{"alpha: beta", 11},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_hash", 18},
         .production = std::string_view{"nb-double-char", 14},
         .source = std::string_view{"\"alpha # beta\"", 14},
         .expected = std::string_view{"alpha # beta", 12},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_null", 25},
         .production = std::string_view{"ns-esc-null", 11},
         .source = std::string_view{"\"\\0\"", 4},
         .expected = std::string_view{"\0", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_bell", 25},
         .production = std::string_view{"ns-esc-bell", 11},
         .source = std::string_view{"\"\\a\"", 4},
         .expected = std::string_view{"\a", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_backspace", 30},
         .production = std::string_view{"ns-esc-backspace", 16},
         .source = std::string_view{"\"\\b\"", 4},
         .expected = std::string_view{"\b", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_horizontal_tab", 35},
         .production = std::string_view{"ns-esc-horizontal-tab", 21},
         .source = std::string_view{"\"\\t\"", 4},
         .expected = std::string_view{"\t", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_line_feed", 30},
         .production = std::string_view{"ns-esc-line-feed", 16},
         .source = std::string_view{"\"\\n\"", 4},
         .expected = std::string_view{"\n", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_vertical_tab", 33},
         .production = std::string_view{"ns-esc-vertical-tab", 19},
         .source = std::string_view{"\"\\v\"", 4},
         .expected = std::string_view{"\v", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_form_feed", 30},
         .production = std::string_view{"ns-esc-form-feed", 16},
         .source = std::string_view{"\"\\f\"", 4},
         .expected = std::string_view{"\f", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_carriage_return", 36},
         .production = std::string_view{"ns-esc-carriage-return", 22},
         .source = std::string_view{"\"\\r\"", 4},
         .expected = std::string_view{"\r", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_escape", 27},
         .production = std::string_view{"ns-esc-escape", 13},
         .source = std::string_view{"\"\\e\"", 4},
         .expected = std::string_view{"\x1B", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_space", 26},
         .production = std::string_view{"ns-esc-space", 12},
         .source = std::string_view{"\"\\ \"", 4},
         .expected = std::string_view{" ", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_double_quote", 33},
         .production = std::string_view{"ns-esc-double-quote", 19},
         .source = std::string_view{"\"\\\"\"", 4},
         .expected = std::string_view{"\"", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_slash", 26},
         .production = std::string_view{"ns-esc-slash", 12},
         .source = std::string_view{"\"\\/\"", 4},
         .expected = std::string_view{"/", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_backslash", 30},
         .production = std::string_view{"ns-esc-backslash", 16},
         .source = std::string_view{"\"\\\\\"", 4},
         .expected = std::string_view{"\\", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_next_line", 30},
         .production = std::string_view{"ns-esc-next-line", 16},
         .source = std::string_view{"\"\\N\"", 4},
         .expected = std::string_view{"\xC2\x85", 2},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_non_breaking_space", 39},
         .production = std::string_view{"ns-esc-non-breaking-space", 25},
         .source = std::string_view{"\"\\_\"", 4},
         .expected = std::string_view{"\xC2\xA0", 2},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_line_separator", 35},
         .production = std::string_view{"ns-esc-line-separator", 21},
         .source = std::string_view{"\"\\L\"", 4},
         .expected = std::string_view{"\xE2\x80\xA8", 3},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_ns_esc_paragraph_separator", 40},
         .production = std::string_view{"ns-esc-paragraph-separator", 26},
         .source = std::string_view{"\"\\P\"", 4},
         .expected = std::string_view{"\xE2\x80\xA9", 3},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_unicode_8", 23},
         .production = std::string_view{"ns-esc-8-bit", 12},
         .source = std::string_view{"\"\\x41\"", 6},
         .expected = std::string_view{"A", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_unicode_16", 24},
         .production = std::string_view{"ns-esc-16-bit", 13},
         .source = std::string_view{"\"\\u0041\"", 8},
         .expected = std::string_view{"A", 1},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_unicode_32", 24},
         .production = std::string_view{"ns-esc-32-bit", 13},
         .source = std::string_view{"\"\\U0001F642\"", 12},
         .expected = std::string_view{"\xF0\x9F\x99\x82", 4},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = true,
         .transformed = true
     },
     {
         .name = std::string_view{"double_quoted_invalid_escape", 28},
         .production = std::string_view{"c-ns-esc-char", 13},
         .source = std::string_view{"\"\\q\"", 4},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = false,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_invalid_unicode_short", 35},
         .production = std::string_view{"ns-esc-16-bit", 13},
         .source = std::string_view{"\"\\u123\"", 7},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = false,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_invalid_unicode_digit", 35},
         .production = std::string_view{"ns-esc-16-bit", 13},
         .source = std::string_view{"\"\\u12G4\"", 8},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = false,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_invalid_unicode_surrogate", 39},
         .production = std::string_view{"ns-esc-16-bit", 13},
         .source = std::string_view{"\"\\uD800\"", 8},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = false,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_invalid_unicode_range", 35},
         .production = std::string_view{"ns-esc-32-bit", 13},
         .source = std::string_view{"\"\\U00110000\"", 12},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = false,
         .transformed = false
     },
     {
         .name = std::string_view{"double_quoted_unterminated", 26},
         .production = std::string_view{"c-double-quoted", 15},
         .source = std::string_view{"\"alpha", 6},
         .expected = std::string_view{"", 0},
         .style = YamlScalarStyle::DoubleQuoted,
         .valid = false,
         .transformed = false
     },
     }
};

} // namespace job::yaml::tests
