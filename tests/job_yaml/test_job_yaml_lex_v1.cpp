#include <catch2/catch_test_macros.hpp>

#include <job_yaml_lex.h>

#include "test_job_yaml_utils.h"

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

static void requireLexeme(const YamlLexeme &lexeme,
                          YamlLexType type,
                          YamlScalarStyle style,
                          std::string_view text,
                          std::size_t offset)
{
    REQUIRE(lexeme.type == type);
    REQUIRE(lexeme.scalarStyle == style);
    REQUIRE(lexeme.text == text);
    REQUIRE(lexeme.offset == offset);
}

// ========================================
// Block scalar headers
// ========================================

TEST_CASE("YamlLex scans literal block scalar headers",
          "[job_yaml][lex][block_scalar][literal]")
{
    SECTION("indicator only")
    {
        const auto tokens = lexAll("|\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("strip")
    {
        const auto tokens = lexAll("|-\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|-", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("keep")
    {
        const auto tokens = lexAll("|+\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|+", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("explicit indentation")
    {
        const auto tokens = lexAll("|2\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|2", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("indentation then strip")
    {
        const auto tokens = lexAll("|2-\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|2-", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("strip then indentation")
    {
        const auto tokens = lexAll("|-2\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|-2", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("indentation then keep")
    {
        const auto tokens = lexAll("|2+\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|2+", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("keep then indentation")
    {
        const auto tokens = lexAll("|+2\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|+2", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex scans folded block scalar headers",
          "[job_yaml][lex][block_scalar][folded]")
{
    SECTION("indicator only")
    {
        const auto tokens = lexAll(">\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("strip")
    {
        const auto tokens = lexAll(">-\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">-", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("keep")
    {
        const auto tokens = lexAll(">+\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">+", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("explicit indentation")
    {
        const auto tokens = lexAll(">2\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">2", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("indentation then strip")
    {
        const auto tokens = lexAll(">2-\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">2-", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("strip then indentation")
    {
        const auto tokens = lexAll(">-2\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">-2", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("indentation then keep")
    {
        const auto tokens = lexAll(">2+\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">2+", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("keep then indentation")
    {
        const auto tokens = lexAll(">+2\n"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">+2", 0);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex block scalar header preserves following comment and line break",
          "[job_yaml][lex][block_scalar][comment]")
{
    const auto tokens = lexAll("|2- # comment\n"sv);

    REQUIRE(tokens.size() == 4);
    requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|2-", 0);

    REQUIRE(tokens[1].type == YamlLexType::Comment);
    REQUIRE(tokens[1].text == "# comment");

    REQUIRE(tokens[2].type == YamlLexType::LineBreak);
    REQUIRE(tokens[2].text == "\n");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex block scalar header stops after one indentation and chomping indicator",
          "[job_yaml][lex][block_scalar][invalid]")
{
    SECTION("zero indentation is not consumed")
    {
        const auto tokens = lexAll("|0"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|", 0);

        REQUIRE(tokens[1].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].text == "0");

        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("duplicate indentation is not consumed")
    {
        const auto tokens = lexAll("|22"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Literal, YamlScalarStyle::None, "|2", 0);

        REQUIRE(tokens[1].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].text == "2");

        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("duplicate chomping is not consumed")
    {
        const auto tokens = lexAll(">++"sv);

        REQUIRE(tokens.size() == 3);
        requireLexeme(tokens[0], YamlLexType::Folded, YamlScalarStyle::None, ">+", 0);

        REQUIRE(tokens[1].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].text == "+");

        REQUIRE(tokens[2].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex resetAtLineStart resumes lexing with line-start state",
          "[job_yaml][lex][block_scalar][reset]")
{
    constexpr std::string_view source =
        "[alpha]\n"
        "  beta";

    YamlLex lex{source};

    REQUIRE(lex.next().type == YamlLexType::FlowSequenceStart);
    REQUIRE(lex.next().type == YamlLexType::Scalar);

    lex.resetAtLineStart(8);

    const YamlLexeme indent = lex.next();
    const YamlLexeme scalar = lex.next();

    REQUIRE(indent.type == YamlLexType::Indent);
    REQUIRE(indent.text == "  ");
    REQUIRE(indent.offset == 8);

    REQUIRE(scalar.type == YamlLexType::Scalar);
    REQUIRE(scalar.text == "beta");
    REQUIRE(scalar.offset == 10);

    REQUIRE(lex.next().type == YamlLexType::End);
}


TEST_CASE("YamlLex recognizes document markers at line start",
          "[job_yaml][lex][document]")
{
    SECTION("document start")
    {
        const auto tokens = lexAll("---\nalpha"sv);

        REQUIRE(tokens.size() == 4);

        REQUIRE(tokens[0].type == YamlLexType::DocumentStart);
        REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::None);
        REQUIRE(tokens[0].text == "---");
        REQUIRE(tokens[0].offset == 0);

        REQUIRE(tokens[1].type == YamlLexType::LineBreak);

        REQUIRE(tokens[2].type == YamlLexType::Scalar);
        REQUIRE(tokens[2].scalarStyle == YamlScalarStyle::Plain);
        REQUIRE(tokens[2].text == "alpha");

        REQUIRE(tokens[3].type == YamlLexType::End);
    }

    SECTION("document end")
    {
        const auto tokens = lexAll("...\n"sv);

        REQUIRE(tokens.size() == 3);

        REQUIRE(tokens[0].type == YamlLexType::DocumentEnd);
        REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::None);
        REQUIRE(tokens[0].text == "...");
        REQUIRE(tokens[0].offset == 0);

        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex recognizes document markers after line breaks",
          "[job_yaml][lex][document]")
{
    const auto tokens = lexAll("alpha\n---\nbeta\n...\n"sv);

    REQUIRE(tokens.size() == 9);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "alpha");

    REQUIRE(tokens[1].type == YamlLexType::LineBreak);

    REQUIRE(tokens[2].type == YamlLexType::DocumentStart);
    REQUIRE(tokens[2].text == "---");

    REQUIRE(tokens[3].type == YamlLexType::LineBreak);

    REQUIRE(tokens[4].type == YamlLexType::Scalar);
    REQUIRE(tokens[4].text == "beta");

    REQUIRE(tokens[5].type == YamlLexType::LineBreak);

    REQUIRE(tokens[6].type == YamlLexType::DocumentEnd);
    REQUIRE(tokens[6].text == "...");

    REQUIRE(tokens[7].type == YamlLexType::LineBreak);
    REQUIRE(tokens[8].type == YamlLexType::End);
}

TEST_CASE("YamlLex requires a document marker terminator",
          "[job_yaml][lex][document]")
{
    SECTION("document start lookalike")
    {
        const auto tokens = lexAll("---alpha"sv);

        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens[0].type != YamlLexType::DocumentStart);
    }

    SECTION("document end lookalike")
    {
        const auto tokens = lexAll("...alpha"sv);

        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens[0].type != YamlLexType::DocumentEnd);
    }
}

TEST_CASE("YamlLex accepts valid document marker terminators",
          "[job_yaml][lex][document]")
{
    SECTION("document start before space")
    {
        const auto tokens = lexAll("--- alpha"sv);

        REQUIRE(tokens[0].type == YamlLexType::DocumentStart);
        REQUIRE(tokens[0].text == "---");
    }

    SECTION("document end before space")
    {
        const auto tokens = lexAll("... # done"sv);

        REQUIRE(tokens[0].type == YamlLexType::DocumentEnd);
        REQUIRE(tokens[0].text == "...");
    }

    SECTION("document start at end of source")
    {
        const auto tokens = lexAll("---"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::DocumentStart);
        REQUIRE(tokens[0].text == "---");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("document end at end of source")
    {
        const auto tokens = lexAll("..."sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::DocumentEnd);
        REQUIRE(tokens[0].text == "...");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex does not recognize indented document markers",
          "[job_yaml][lex][document][indent]")
{
    SECTION("indented document start")
    {
        const auto tokens = lexAll("  ---"sv);

        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens[0].type == YamlLexType::Indent);

        for (const YamlLexeme &token : tokens)
            REQUIRE(token.type != YamlLexType::DocumentStart);
    }

    SECTION("indented document end")
    {
        const auto tokens = lexAll("  ..."sv);

        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens[0].type == YamlLexType::Indent);

        for (const YamlLexeme &token : tokens)
            REQUIRE(token.type != YamlLexType::DocumentEnd);
    }
}

TEST_CASE("YamlLex recognizes directives only at true line start",
          "[job_yaml][lex][directive]")
{
    SECTION("directive at source start")
    {
        const auto tokens = lexAll("%YAML 1.2"sv);

        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens[0].type == YamlLexType::Directive);
        REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::None);
        REQUIRE(tokens[0].text == "%");
        REQUIRE(tokens[0].offset == 0);
    }

    SECTION("directive after line break")
    {
        const auto tokens = lexAll("alpha\n%TAG !e! tag:example.com,2000:app/"sv);

        REQUIRE(tokens.size() >= 4);
        REQUIRE(tokens[0].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::Directive);
        REQUIRE(tokens[2].text == "%");
    }

    SECTION("percent inside content")
    {
        const auto tokens = lexAll("alpha % beta"sv);

        for (const YamlLexeme &token : tokens)
            REQUIRE(token.type != YamlLexType::Directive);
    }

    SECTION("indented percent")
    {
        const auto tokens = lexAll("  %YAML 1.2"sv);

        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens[0].type == YamlLexType::Indent);

        for (const YamlLexeme &token : tokens)
            REQUIRE(token.type != YamlLexType::Directive);
    }
}

TEST_CASE("YamlLex document tokens preserve source ranges",
          "[job_yaml][lex][document][range]")
{
    constexpr std::string_view source = "---\nalpha\n...\n";
    const auto tokens = lexAll(source);

    REQUIRE(tokens.size() == 7);

    REQUIRE(tokens[0].type == YamlLexType::DocumentStart);
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 3);
    REQUIRE(tokens[0].endOffset() == 3);
    REQUIRE(tokens[0].range().offset == 0);
    REQUIRE(tokens[0].range().size == 3);

    REQUIRE(tokens[4].type == YamlLexType::DocumentEnd);
    REQUIRE(tokens[4].offset == 10);
    REQUIRE(tokens[4].size() == 3);
    REQUIRE(tokens[4].endOffset() == 13);
    REQUIRE(tokens[4].range().offset == 10);
    REQUIRE(tokens[4].range().size == 3);
}


} // namespace job::yaml::tests