#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "job_yaml_block_scalar_decoder.h"

namespace job::yaml::tests {

struct YamlBlockScalarFixture
{
    std::string_view name{};
    std::string_view production{};
    std::string_view source{};
    std::string_view expected{};
    YamlBlockScalarStyle style{YamlBlockScalarStyle::Literal};
    YamlBlockScalarChomping chomping{YamlBlockScalarChomping::Clip};
    std::size_t indentation{};
    bool valid{};
};

inline constexpr std::array<YamlBlockScalarFixture, 5> LiteralBlockScalarFixtures = {{
    {
        .name = std::string_view{"literal_simple", 14},
        .production = std::string_view{"c-l+literal", 11},
        .source = std::string_view{"|\n  alpha\n", 10},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_multiline", 17},
        .production = std::string_view{"l-literal-content", 17},
        .source = std::string_view{"|\n  alpha\n  beta\n", 17},
        .expected = std::string_view{"alpha\nbeta\n", 11},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_blank_line", 18},
        .production = std::string_view{"l-literal-content", 17},
        .source = std::string_view{"|\n  alpha\n\n  beta\n", 18},
        .expected = std::string_view{"alpha\n\nbeta\n", 12},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_leading_blank_line", 26},
        .production = std::string_view{"l-literal-content", 17},
        .source = std::string_view{"|\n\n  alpha\n", 11},
        .expected = std::string_view{"\nalpha\n", 7},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_more_indented", 21},
        .production = std::string_view{"l-literal-content", 17},
        .source = std::string_view{"|\n  alpha\n    beta\n  gamma\n", 27},
        .expected = std::string_view{"alpha\n  beta\ngamma\n", 19},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
}};

inline constexpr std::array<YamlBlockScalarFixture, 5> FoldedBlockScalarFixtures = {{
    {
        .name = std::string_view{"folded_simple", 13},
        .production = std::string_view{"c-l+folded", 10},
        .source = std::string_view{">\n  alpha\n  beta\n", 17},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_blank_line", 17},
        .production = std::string_view{"l-nb-diff-lines", 15},
        .source = std::string_view{">\n  alpha\n\n  beta\n", 18},
        .expected = std::string_view{"alpha\nbeta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_multiple_blank_lines", 27},
        .production = std::string_view{"l-nb-diff-lines", 15},
        .source = std::string_view{">\n  alpha\n\n\n  beta\n", 19},
        .expected = std::string_view{"alpha\n\nbeta\n", 12},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_more_indented_transition", 31},
        .production = std::string_view{"l-nb-diff-lines", 15},
        .source = std::string_view{">\n  alpha\n    beta\n  gamma\n", 27},
        .expected = std::string_view{"alpha\n  beta\ngamma\n", 19},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_more_indented_block", 26},
        .production = std::string_view{"l-nb-spaced-lines", 17},
        .source = std::string_view{">\n  alpha\n    beta\n    gamma\n  delta\n", 37},
        .expected = std::string_view{"alpha\n  beta\n  gamma\ndelta\n", 27},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
}};

inline constexpr std::array<YamlBlockScalarFixture, 6> BlockScalarChompingFixtures = {{
    {
        .name = std::string_view{"literal_clip", 12},
        .production = std::string_view{"c-chomping-indicator", 20},
        .source = std::string_view{"|\n  alpha\n\n", 11},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_strip", 13},
        .production = std::string_view{"c-chomping-indicator", 20},
        .source = std::string_view{"|-\n  alpha\n\n", 12},
        .expected = std::string_view{"alpha", 5},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Strip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_keep", 12},
        .production = std::string_view{"c-chomping-indicator", 20},
        .source = std::string_view{"|+\n  alpha\n\n", 12},
        .expected = std::string_view{"alpha\n\n", 7},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Keep,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_clip", 11},
        .production = std::string_view{"c-chomping-indicator", 20},
        .source = std::string_view{">\n  alpha\n  beta\n\n", 18},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_strip", 12},
        .production = std::string_view{"c-chomping-indicator", 20},
        .source = std::string_view{">-\n  alpha\n  beta\n\n", 19},
        .expected = std::string_view{"alpha beta", 10},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Strip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_keep", 11},
        .production = std::string_view{"c-chomping-indicator", 20},
        .source = std::string_view{">+\n  alpha\n  beta\n\n", 19},
        .expected = std::string_view{"alpha beta\n\n", 12},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Keep,
        .indentation = 0,
        .valid = true
    },
}};

inline constexpr std::array<YamlBlockScalarFixture, 20> BlockScalarIndentationFixtures = {{
    {
        .name = std::string_view{"literal_auto_indent", 19},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|\n  alpha\n", 10},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"folded_auto_indent", 18},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">\n  alpha\n  beta\n", 17},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_1", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|1\n alpha\n", 10},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 1,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_1", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">1\n alpha\n beta\n", 16},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 1,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_2", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|2\n  alpha\n", 11},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 2,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_2", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">2\n  alpha\n  beta\n", 18},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 2,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_3", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|3\n   alpha\n", 12},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 3,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_3", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">3\n   alpha\n   beta\n", 20},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 3,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_4", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|4\n    alpha\n", 13},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 4,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_4", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">4\n    alpha\n    beta\n", 22},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 4,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_5", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|5\n     alpha\n", 14},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 5,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_5", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">5\n     alpha\n     beta\n", 24},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 5,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_6", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|6\n      alpha\n", 15},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 6,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_6", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">6\n      alpha\n      beta\n", 26},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 6,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_7", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|7\n       alpha\n", 16},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 7,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_7", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">7\n       alpha\n       beta\n", 28},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 7,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_8", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|8\n        alpha\n", 17},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 8,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_8", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">8\n        alpha\n        beta\n", 30},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 8,
        .valid = true
    },
    {
        .name = std::string_view{"literal_explicit_indent_9", 25},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|9\n         alpha\n", 18},
        .expected = std::string_view{"alpha\n", 6},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 9,
        .valid = true
    },
    {
        .name = std::string_view{"folded_explicit_indent_9", 24},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">9\n         alpha\n         beta\n", 32},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 9,
        .valid = true
    },
}};

inline constexpr std::array<YamlBlockScalarFixture, 4> BlockScalarHeaderOrderFixtures = {{
    {
        .name = std::string_view{"literal_indent_then_strip", 25},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"|2-\n  alpha\n", 12},
        .expected = std::string_view{"alpha", 5},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Strip,
        .indentation = 2,
        .valid = true
    },
    {
        .name = std::string_view{"literal_strip_then_indent", 25},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"|-2\n  alpha\n", 12},
        .expected = std::string_view{"alpha", 5},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Strip,
        .indentation = 2,
        .valid = true
    },
    {
        .name = std::string_view{"folded_indent_then_keep", 23},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{">2+\n  alpha\n  beta\n", 19},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Keep,
        .indentation = 2,
        .valid = true
    },
    {
        .name = std::string_view{"folded_keep_then_indent", 23},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{">+2\n  alpha\n  beta\n", 19},
        .expected = std::string_view{"alpha beta\n", 11},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Keep,
        .indentation = 2,
        .valid = true
    },
}};

inline constexpr std::array<YamlBlockScalarFixture, 9> InvalidBlockScalarFixtures = {{
    {
        .name = std::string_view{"literal_invalid_indent_zero", 27},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{"|0\n  alpha\n", 11},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"folded_invalid_indent_zero", 26},
        .production = std::string_view{"c-indentation-indicator", 23},
        .source = std::string_view{">0\n  alpha\n", 11},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"literal_duplicate_indent", 24},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"|22\n  alpha\n", 12},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"literal_duplicate_chomping", 26},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"|++\n  alpha\n", 12},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"folded_duplicate_chomping", 25},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{">--\n  alpha\n", 12},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"literal_invalid_header_character", 32},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{"|x\n  alpha\n", 11},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"folded_invalid_header_character", 31},
        .production = std::string_view{"c-b-block-header", 16},
        .source = std::string_view{">x\n  alpha\n", 11},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 0,
        .valid = false
    },
    {
        .name = std::string_view{"literal_insufficient_explicit_indent", 36},
        .production = std::string_view{"l-literal-content", 17},
        .source = std::string_view{"|3\n  alpha\n", 11},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Literal,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 3,
        .valid = false
    },
    {
        .name = std::string_view{"folded_insufficient_explicit_indent", 35},
        .production = std::string_view{"l-folded-content", 16},
        .source = std::string_view{">3\n  alpha\n", 11},
        .expected = std::string_view{"", 0},
        .style = YamlBlockScalarStyle::Folded,
        .chomping = YamlBlockScalarChomping::Clip,
        .indentation = 3,
        .valid = false
    },
}};

} // namespace job::yaml::tests
