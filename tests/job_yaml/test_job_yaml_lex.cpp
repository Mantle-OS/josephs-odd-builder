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

TEST_CASE("YamlLex returns End for empty input", "[job_yaml][lex]")
{
    YamlLex lex{""};

    const YamlLexeme token = lex.next();

    REQUIRE(token.type == YamlLexType::End);
    REQUIRE(token.scalarStyle == YamlScalarStyle::None);
    REQUIRE(token.text.empty());
    REQUIRE(token.offset == 0);
    REQUIRE(token.size() == 0);
    REQUIRE(token.endOffset() == 0);
    REQUIRE(token.empty());

    const YamlSourceRange range = token.range();

    REQUIRE(range.offset == 0);
    REQUIRE(range.size == 0);
    REQUIRE(range.endOffset() == 0);
    REQUIRE(range.empty());
}

TEST_CASE("YamlLex emits indentation at line start", "[job_yaml][lex][indent]")
{
    YamlLex lex{"    value"};

    const YamlLexeme indent = lex.next();
    const YamlLexeme scalar = lex.next();

    REQUIRE(indent.type == YamlLexType::Indent);
    REQUIRE(indent.text == "    ");
    REQUIRE(indent.offset == 0);
    REQUIRE(indent.size() == 4);
    REQUIRE(indent.endOffset() == 4);

    REQUIRE(scalar.type == YamlLexType::Scalar);
    REQUIRE(scalar.text == "value");
    REQUIRE(scalar.offset == 4);
    REQUIRE(scalar.size() == 5);
    REQUIRE(scalar.endOffset() == 9);
}

TEST_CASE("YamlLex preserves indentation characters", "[job_yaml][lex][indent]")
{
    YamlLex lex{" \t  value"};

    const YamlLexeme indent = lex.next();
    const YamlLexeme scalar = lex.next();

    REQUIRE(indent.type == YamlLexType::Indent);
    REQUIRE(indent.text == " \t  ");
    REQUIRE(indent.offset == 0);
    REQUIRE(indent.size() == 4);
    REQUIRE(indent.endOffset() == 4);

    REQUIRE(scalar.type == YamlLexType::Scalar);
    REQUIRE(scalar.text == "value");
    REQUIRE(scalar.offset == 4);
}

TEST_CASE("YamlLex skips inline whitespace after content", "[job_yaml][lex]")
{
    const auto tokens = lexAll("name:    alpha"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "name");

    REQUIRE(tokens[1].type == YamlLexType::MappingValue);

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "alpha");
    REQUIRE(tokens[2].offset == 9);

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex emits indentation after line break", "[job_yaml][lex][indent]")
{
    const auto tokens = lexAll("root:\n  child: value"sv);

    REQUIRE(tokens.size() == 8);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "root");

    REQUIRE(tokens[1].type == YamlLexType::MappingValue);

    REQUIRE(tokens[2].type == YamlLexType::LineBreak);

    REQUIRE(tokens[3].type == YamlLexType::Indent);
    REQUIRE(tokens[3].text == "  ");
    REQUIRE(tokens[3].offset == 6);
    REQUIRE(tokens[3].size() == 2);
    REQUIRE(tokens[3].endOffset() == 8);

    REQUIRE(tokens[4].type == YamlLexType::Scalar);
    REQUIRE(tokens[4].text == "child");

    REQUIRE(tokens[5].type == YamlLexType::MappingValue);

    REQUIRE(tokens[6].type == YamlLexType::Scalar);
    REQUIRE(tokens[6].text == "value");

    REQUIRE(tokens[7].type == YamlLexType::End);
}

TEST_CASE("YamlLex emits indentation independently on each line", "[job_yaml][lex][indent]")
{
    const auto tokens = lexAll(
        "root:\n"
        "  child:\n"
        "    value\n"
        "  sibling"sv);

    REQUIRE(tokens.size() == 13);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "root");
    REQUIRE(tokens[1].type == YamlLexType::MappingValue);
    REQUIRE(tokens[2].type == YamlLexType::LineBreak);

    REQUIRE(tokens[3].type == YamlLexType::Indent);
    REQUIRE(tokens[3].text == "  ");
    REQUIRE(tokens[4].type == YamlLexType::Scalar);
    REQUIRE(tokens[4].text == "child");
    REQUIRE(tokens[5].type == YamlLexType::MappingValue);
    REQUIRE(tokens[6].type == YamlLexType::LineBreak);

    REQUIRE(tokens[7].type == YamlLexType::Indent);
    REQUIRE(tokens[7].text == "    ");
    REQUIRE(tokens[8].type == YamlLexType::Scalar);
    REQUIRE(tokens[8].text == "value");
    REQUIRE(tokens[9].type == YamlLexType::LineBreak);

    REQUIRE(tokens[10].type == YamlLexType::Indent);
    REQUIRE(tokens[10].text == "  ");
    REQUIRE(tokens[11].type == YamlLexType::Scalar);
    REQUIRE(tokens[11].text == "sibling");

    REQUIRE(tokens[12].type == YamlLexType::End);
}

TEST_CASE("YamlLex does not emit indentation for unindented line", "[job_yaml][lex][indent]")
{
    const auto tokens = lexAll("one\ntwo"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "one");

    REQUIRE(tokens[1].type == YamlLexType::LineBreak);

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "two");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex emits indentation before comment", "[job_yaml][lex][indent][comment]")
{
    const auto tokens = lexAll("  # comment"sv);

    REQUIRE(tokens.size() == 3);

    REQUIRE(tokens[0].type == YamlLexType::Indent);
    REQUIRE(tokens[0].text == "  ");

    REQUIRE(tokens[1].type == YamlLexType::Comment);
    REQUIRE(tokens[1].text == "# comment");

    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex emits indentation on blank indented line", "[job_yaml][lex][indent]")
{
    const auto tokens = lexAll("  \nnext"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Indent);
    REQUIRE(tokens[0].text == "  ");

    REQUIRE(tokens[1].type == YamlLexType::LineBreak);

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "next");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex emits structural tokens", "[job_yaml][lex]")
{
    const auto tokens = lexAll("- ? : , [ ] { } & * ! | > #"sv);

    REQUIRE(tokens.size() == 15);

    REQUIRE(tokens[0].type == YamlLexType::SequenceEntry);
    REQUIRE(tokens[0].text == "-");

    REQUIRE(tokens[1].type == YamlLexType::MappingKey);
    REQUIRE(tokens[1].text == "?");

    REQUIRE(tokens[2].type == YamlLexType::MappingValue);
    REQUIRE(tokens[2].text == ":");

    REQUIRE(tokens[3].type == YamlLexType::CollectEntry);
    REQUIRE(tokens[3].text == ",");

    REQUIRE(tokens[4].type == YamlLexType::FlowSequenceStart);
    REQUIRE(tokens[4].text == "[");

    REQUIRE(tokens[5].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[5].text == "]");

    REQUIRE(tokens[6].type == YamlLexType::FlowMappingStart);
    REQUIRE(tokens[6].text == "{");

    REQUIRE(tokens[7].type == YamlLexType::FlowMappingEnd);
    REQUIRE(tokens[7].text == "}");

    REQUIRE(tokens[8].type == YamlLexType::Anchor);
    REQUIRE(tokens[8].text == "&");

    REQUIRE(tokens[9].type == YamlLexType::Alias);
    REQUIRE(tokens[9].text == "*");

    REQUIRE(tokens[10].type == YamlLexType::Tag);
    REQUIRE(tokens[10].text == "!");

    REQUIRE(tokens[11].type == YamlLexType::Literal);
    REQUIRE(tokens[11].text == "|");

    REQUIRE(tokens[12].type == YamlLexType::Folded);
    REQUIRE(tokens[12].text == ">");

    REQUIRE(tokens[13].type == YamlLexType::Comment);
    REQUIRE(tokens[13].text == "#");

    REQUIRE(tokens[14].type == YamlLexType::End);
    REQUIRE(tokens[14].text.empty());
}

TEST_CASE("YamlLex structural lexemes have no scalar style", "[job_yaml][lex][scalar_style]")
{
    const auto tokens = lexAll("- ? : , [ ] { } & * ! | > #"sv);

    for (const YamlLexeme &token : tokens)
        REQUIRE(token.scalarStyle == YamlScalarStyle::None);
}

// ========================================
// Node properties
// ========================================

TEST_CASE("YamlLex scans anchor name", "[job_yaml][lex][node_property][anchor]")
{
    constexpr std::string_view source = "&alpha value";

    const auto tokens = lexAll(source);

    REQUIRE(tokens.size() == 3);

    REQUIRE(tokens[0].type == YamlLexType::Anchor);
    REQUIRE(tokens[0].text == "&alpha");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 6);
    REQUIRE(tokens[0].endOffset() == 6);
    REQUIRE(tokens[0].range().view(source) == "&alpha");

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "value");
    REQUIRE(tokens[1].offset == 7);

    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans alias name", "[job_yaml][lex][node_property][alias]")
{
    constexpr std::string_view source = "*alpha";

    const auto tokens = lexAll(source);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Alias);
    REQUIRE(tokens[0].text == "*alpha");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 6);
    REQUIRE(tokens[0].endOffset() == 6);
    REQUIRE(tokens[0].range().view(source) == "*alpha");

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans local tag", "[job_yaml][lex][node_property][tag]")
{
    const auto tokens = lexAll("!example value"sv);

    REQUIRE(tokens.size() == 3);

    REQUIRE(tokens[0].type == YamlLexType::Tag);
    REQUIRE(tokens[0].text == "!example");

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "value");

    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans secondary tag", "[job_yaml][lex][node_property][tag]")
{
    const auto tokens = lexAll("!!str value"sv);

    REQUIRE(tokens.size() == 3);

    REQUIRE(tokens[0].type == YamlLexType::Tag);
    REQUIRE(tokens[0].text == "!!str");

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "value");

    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans verbatim tag", "[job_yaml][lex][node_property][tag]")
{
    constexpr std::string_view source = "!<tag:example.com,2026:type> value";
    const auto tokens = lexAll(source);
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == YamlLexType::Tag);
    REQUIRE(tokens[0].text == "!<tag:example.com,2026:type>");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 28);
    REQUIRE(tokens[0].endOffset() == 28);
    REQUIRE(tokens[0].range().view(source) == "!<tag:example.com,2026:type>");

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "value");
    REQUIRE(tokens[1].offset == 29);
    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex preserves anchor spelling", "[job_yaml][lex][node_property][anchor]")
{
    const auto tokens = lexAll("&alpha-beta_42 value"sv);

    REQUIRE(tokens.size() == 3);

    REQUIRE(tokens[0].type == YamlLexType::Anchor);
    REQUIRE(tokens[0].text == "&alpha-beta_42");

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "value");

    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex preserves alias spelling", "[job_yaml][lex][node_property][alias]")
{
    const auto tokens = lexAll("*alpha-beta_42"sv);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Alias);
    REQUIRE(tokens[0].text == "*alpha-beta_42");

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex terminates node property at separation", "[job_yaml][lex][node_property]")
{
    SECTION("space")
    {
        const auto tokens = lexAll("&alpha value"sv);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == YamlLexType::Anchor);
        REQUIRE(tokens[0].text == "&alpha");
        REQUIRE(tokens[1].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].text == "value");
    }

    SECTION("tab")
    {
        const auto tokens = lexAll("&alpha\tvalue"sv);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == YamlLexType::Anchor);
        REQUIRE(tokens[0].text == "&alpha");
        REQUIRE(tokens[1].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].text == "value");
    }

    SECTION("line break")
    {
        const auto tokens = lexAll("&alpha\nvalue"sv);

        REQUIRE(tokens.size() == 4);
        REQUIRE(tokens[0].type == YamlLexType::Anchor);
        REQUIRE(tokens[0].text == "&alpha");
        REQUIRE(tokens[1].type == YamlLexType::LineBreak);
        REQUIRE(tokens[2].type == YamlLexType::Scalar);
        REQUIRE(tokens[2].text == "value");
    }
}

TEST_CASE("YamlLex terminates node property at flow delimiters", "[job_yaml][lex][node_property]")
{
    SECTION("comma")
    {
        const auto tokens = lexAll("&alpha,"sv);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == YamlLexType::Anchor);
        REQUIRE(tokens[0].text == "&alpha");
        REQUIRE(tokens[1].type == YamlLexType::CollectEntry);
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("sequence end")
    {
        const auto tokens = lexAll("[*alpha]"sv);

        REQUIRE(tokens.size() == 4);
        REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
        REQUIRE(tokens[1].type == YamlLexType::Alias);
        REQUIRE(tokens[1].text == "*alpha");
        REQUIRE(tokens[2].type == YamlLexType::FlowSequenceEnd);
        REQUIRE(tokens[3].type == YamlLexType::End);
    }

    SECTION("mapping end")
    {
        const auto tokens = lexAll("{!example}"sv);

        REQUIRE(tokens.size() == 4);
        REQUIRE(tokens[0].type == YamlLexType::FlowMappingStart);
        REQUIRE(tokens[1].type == YamlLexType::Tag);
        REQUIRE(tokens[1].text == "!example");
        REQUIRE(tokens[2].type == YamlLexType::FlowMappingEnd);
        REQUIRE(tokens[3].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex scans multiple node properties independently", "[job_yaml][lex][node_property]")
{
    const auto tokens = lexAll("&alpha !example value"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Anchor);
    REQUIRE(tokens[0].text == "&alpha");

    REQUIRE(tokens[1].type == YamlLexType::Tag);
    REQUIRE(tokens[1].text == "!example");

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "value");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans tag before anchor independently", "[job_yaml][lex][node_property]")
{
    const auto tokens = lexAll("!example &alpha value"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Tag);
    REQUIRE(tokens[0].text == "!example");

    REQUIRE(tokens[1].type == YamlLexType::Anchor);
    REQUIRE(tokens[1].text == "&alpha");

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "value");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex bare node property sigils remain tokens", "[job_yaml][lex][node_property]")
{
    SECTION("anchor")
    {
        const auto tokens = lexAll("&"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Anchor);
        REQUIRE(tokens[0].text == "&");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("alias")
    {
        const auto tokens = lexAll("*"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Alias);
        REQUIRE(tokens[0].text == "*");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("tag")
    {
        const auto tokens = lexAll("!"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Tag);
        REQUIRE(tokens[0].text == "!");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex returns remainder for unterminated verbatim tag", "[job_yaml][lex][node_property][tag]")
{
    constexpr std::string_view source = "!<tag:example.com,2026:type";

    const auto tokens = lexAll(source);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Tag);
    REQUIRE(tokens[0].text == source);
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == source.size());
    REQUIRE(tokens[0].endOffset() == source.size());
    REQUIRE(tokens[0].range().view(source) == source);

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex node property views point directly into source",
          "[job_yaml][lex][node_property][range][view]")
{
    constexpr std::string_view source = "&alpha *beta !example";

    YamlLex lex{source};

    const YamlLexeme anchor = lex.next();
    const YamlLexeme alias = lex.next();
    const YamlLexeme tag = lex.next();

    REQUIRE(anchor.type == YamlLexType::Anchor);
    REQUIRE(anchor.text == "&alpha");
    REQUIRE(anchor.text.data() == source.data() + anchor.offset);
    REQUIRE(anchor.range().view(source).data() == anchor.text.data());

    REQUIRE(alias.type == YamlLexType::Alias);
    REQUIRE(alias.text == "*beta");
    REQUIRE(alias.text.data() == source.data() + alias.offset);
    REQUIRE(alias.range().view(source).data() == alias.text.data());

    REQUIRE(tag.type == YamlLexType::Tag);
    REQUIRE(tag.text == "!example");
    REQUIRE(tag.text.data() == source.data() + tag.offset);
    REQUIRE(tag.range().view(source).data() == tag.text.data());
}

TEST_CASE("YamlLex handles line breaks", "[job_yaml][lex]")
{
    SECTION("LF")
    {
        const auto tokens = lexAll("\n"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::LineBreak);
        REQUIRE(tokens[0].text == "\n");
        REQUIRE(tokens[0].size() == 1);
        REQUIRE(tokens[0].endOffset() == 1);
        REQUIRE(tokens[1].type == YamlLexType::End);
        REQUIRE(tokens[1].text.empty());
    }

    SECTION("CR")
    {
        const auto tokens = lexAll("\r"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::LineBreak);
        REQUIRE(tokens[0].text == "\r");
        REQUIRE(tokens[0].size() == 1);
        REQUIRE(tokens[0].endOffset() == 1);
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("CRLF")
    {
        const auto tokens = lexAll("\r\n"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::LineBreak);
        REQUIRE(tokens[0].text == "\r\n");
        REQUIRE(tokens[0].offset == 0);
        REQUIRE(tokens[0].size() == 2);
        REQUIRE(tokens[0].endOffset() == 2);
        REQUIRE(tokens[1].type == YamlLexType::End);
        REQUIRE(tokens[1].offset == 2);
    }
}

TEST_CASE("YamlLex scans comments to line end", "[job_yaml][lex]")
{
    const auto tokens = lexAll("# hello world\nnext"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Comment);
    REQUIRE(tokens[0].text == "# hello world");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 13);
    REQUIRE(tokens[0].endOffset() == 13);

    REQUIRE(tokens[1].type == YamlLexType::LineBreak);
    REQUIRE(tokens[1].text == "\n");

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "next");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans a plain scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("hello world"sv);

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::Plain);
    REQUIRE(tokens[0].text == "hello world");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 11);
    REQUIRE(tokens[0].endOffset() == 11);
    REQUIRE_FALSE(tokens[0].empty());
    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex trims trailing inline whitespace from scalar range", "[job_yaml][lex][range]")
{
    constexpr std::string_view source = "hello world   \t";

    const auto tokens = lexAll(source);

    REQUIRE(tokens.size() == 2);

    const YamlLexeme &scalar = tokens[0];

    REQUIRE(scalar.type == YamlLexType::Scalar);
    REQUIRE(scalar.text == "hello world");
    REQUIRE(scalar.offset == 0);
    REQUIRE(scalar.size() == 11);
    REQUIRE(scalar.endOffset() == 11);

    const YamlSourceRange range = scalar.range();

    REQUIRE(range.offset == 0);
    REQUIRE(range.size == 11);
    REQUIRE(range.endOffset() == 11);
    REQUIRE(range.view(source) == "hello world");

    REQUIRE(tokens[1].type == YamlLexType::End);
    REQUIRE(tokens[1].offset == source.size());
}

TEST_CASE("YamlLex separates mapping key and value", "[job_yaml][lex]")
{
    const auto tokens = lexAll("name: alpha"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "name");

    REQUIRE(tokens[1].type == YamlLexType::MappingValue);
    REQUIRE(tokens[1].text == ":");

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "alpha");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex recognizes sequence entry followed by scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("- item"sv);

    REQUIRE(tokens.size() == 3);

    REQUIRE(tokens[0].type == YamlLexType::SequenceEntry);
    REQUIRE(tokens[0].text == "-");

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "item");

    REQUIRE(tokens[2].type == YamlLexType::End);
}

TEST_CASE("YamlLex recognizes indented sequence entry", "[job_yaml][lex][indent][sequence]")
{
    const auto tokens = lexAll("  - item"sv);

    REQUIRE(tokens.size() == 4);

    REQUIRE(tokens[0].type == YamlLexType::Indent);
    REQUIRE(tokens[0].text == "  ");

    REQUIRE(tokens[1].type == YamlLexType::SequenceEntry);
    REQUIRE(tokens[1].text == "-");

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "item");

    REQUIRE(tokens[3].type == YamlLexType::End);
}

TEST_CASE("YamlLex keeps dash inside plain scalar when not followed by separation", "[job_yaml][lex]")
{
    const auto tokens = lexAll("hello-world"sv);

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "hello-world");
    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex keeps colon inside plain scalar when not followed by separation", "[job_yaml][lex]")
{
    const auto tokens = lexAll("http://example.com"sv);

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "http://example.com");
    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex handles flow sequence", "[job_yaml][lex]")
{
    const auto tokens = lexAll("[one, two, three]"sv);

    REQUIRE(tokens.size() == 8);

    REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "one");
    REQUIRE(tokens[2].type == YamlLexType::CollectEntry);
    REQUIRE(tokens[3].type == YamlLexType::Scalar);
    REQUIRE(tokens[3].text == "two");
    REQUIRE(tokens[4].type == YamlLexType::CollectEntry);
    REQUIRE(tokens[5].type == YamlLexType::Scalar);
    REQUIRE(tokens[5].text == "three");
    REQUIRE(tokens[6].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[7].type == YamlLexType::End);
}

TEST_CASE("YamlLex handles flow mapping", "[job_yaml][lex]")
{
    const auto tokens = lexAll("{name: alpha, age: 42}"sv);

    REQUIRE(tokens.size() == 10);

    REQUIRE(tokens[0].type == YamlLexType::FlowMappingStart);

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "name");

    REQUIRE(tokens[2].type == YamlLexType::MappingValue);

    REQUIRE(tokens[3].type == YamlLexType::Scalar);
    REQUIRE(tokens[3].text == "alpha");

    REQUIRE(tokens[4].type == YamlLexType::CollectEntry);

    REQUIRE(tokens[5].type == YamlLexType::Scalar);
    REQUIRE(tokens[5].text == "age");

    REQUIRE(tokens[6].type == YamlLexType::MappingValue);

    REQUIRE(tokens[7].type == YamlLexType::Scalar);
    REQUIRE(tokens[7].text == "42");

    REQUIRE(tokens[8].type == YamlLexType::FlowMappingEnd);
    REQUIRE(tokens[9].type == YamlLexType::End);
}

// ========================================
// Flow collection lexical context
// ========================================

TEST_CASE("YamlLex handles empty flow collections",
          "[job_yaml][lex][flow]")
{
    SECTION("sequence")
    {
        const auto tokens = lexAll("[]"sv);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
        REQUIRE(tokens[0].text == "[");
        REQUIRE(tokens[1].type == YamlLexType::FlowSequenceEnd);
        REQUIRE(tokens[1].text == "]");
        REQUIRE(tokens[2].type == YamlLexType::End);
    }

    SECTION("mapping")
    {
        const auto tokens = lexAll("{}"sv);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == YamlLexType::FlowMappingStart);
        REQUIRE(tokens[0].text == "{");
        REQUIRE(tokens[1].type == YamlLexType::FlowMappingEnd);
        REQUIRE(tokens[1].text == "}");
        REQUIRE(tokens[2].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex handles nested flow collections",
          "[job_yaml][lex][flow][nested]")
{
    const auto tokens = lexAll("[alpha, {beta: [gamma, delta]}]"sv);

    REQUIRE(tokens.size() == 14);

    REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);

    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "alpha");

    REQUIRE(tokens[2].type == YamlLexType::CollectEntry);

    REQUIRE(tokens[3].type == YamlLexType::FlowMappingStart);

    REQUIRE(tokens[4].type == YamlLexType::Scalar);
    REQUIRE(tokens[4].text == "beta");

    REQUIRE(tokens[5].type == YamlLexType::MappingValue);

    REQUIRE(tokens[6].type == YamlLexType::FlowSequenceStart);

    REQUIRE(tokens[7].type == YamlLexType::Scalar);
    REQUIRE(tokens[7].text == "gamma");

    REQUIRE(tokens[8].type == YamlLexType::CollectEntry);

    REQUIRE(tokens[9].type == YamlLexType::Scalar);
    REQUIRE(tokens[9].text == "delta");

    REQUIRE(tokens[10].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[11].type == YamlLexType::FlowMappingEnd);
    REQUIRE(tokens[12].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[13].type == YamlLexType::End);
}

TEST_CASE("YamlLex keeps quoted flow punctuation inside scalar",
          "[job_yaml][lex][flow][quoted]")
{
    SECTION("single quoted")
    {
        const auto tokens = lexAll("['alpha,beta', 'gamma]delta']"sv);

        REQUIRE(tokens.size() == 6);
        REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
        requireLexeme(tokens[1], YamlLexType::Scalar, YamlScalarStyle::SingleQuoted, "'alpha,beta'", 1);
        REQUIRE(tokens[2].type == YamlLexType::CollectEntry);
        requireLexeme(tokens[3], YamlLexType::Scalar, YamlScalarStyle::SingleQuoted, "'gamma]delta'", 15);
        REQUIRE(tokens[4].type == YamlLexType::FlowSequenceEnd);
        REQUIRE(tokens[5].type == YamlLexType::End);
    }

    SECTION("double quoted")
    {
        const auto tokens = lexAll("{name: \"alpha,beta]gamma\"}"sv);

        REQUIRE(tokens.size() == 6);
        REQUIRE(tokens[0].type == YamlLexType::FlowMappingStart);
        REQUIRE(tokens[1].type == YamlLexType::Scalar);
        REQUIRE(tokens[1].text == "name");
        REQUIRE(tokens[2].type == YamlLexType::MappingValue);
        requireLexeme(tokens[3], YamlLexType::Scalar, YamlScalarStyle::DoubleQuoted, "\"alpha,beta]gamma\"", 7);
        REQUIRE(tokens[4].type == YamlLexType::FlowMappingEnd);
        REQUIRE(tokens[5].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex keeps hash inside flow plain scalar without comment separation",
          "[job_yaml][lex][flow][plain][comment]")
{
    const auto tokens = lexAll("[alpha#beta, gamma]"sv);

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "alpha#beta");
    REQUIRE(tokens[2].type == YamlLexType::CollectEntry);
    REQUIRE(tokens[3].type == YamlLexType::Scalar);
    REQUIRE(tokens[3].text == "gamma");
    REQUIRE(tokens[4].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[5].type == YamlLexType::End);
}

TEST_CASE("YamlLex recognizes separated comment inside flow collection",
          "[job_yaml][lex][flow][plain][comment]")
{
    const auto tokens = lexAll("[alpha # comment\n]"sv);

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "alpha");
    REQUIRE(tokens[2].type == YamlLexType::Comment);
    REQUIRE(tokens[2].text == "# comment");
    REQUIRE(tokens[3].type == YamlLexType::LineBreak);
    REQUIRE(tokens[4].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[5].type == YamlLexType::End);
}

TEST_CASE("YamlLex keeps flow indicator characters inside block plain scalar",
          "[job_yaml][lex][flow][plain][context]")
{
    SECTION("comma")
    {
        const auto tokens = lexAll("alpha,beta"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Scalar);
        REQUIRE(tokens[0].text == "alpha,beta");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("brackets")
    {
        const auto tokens = lexAll("alpha[beta]gamma"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Scalar);
        REQUIRE(tokens[0].text == "alpha[beta]gamma");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("braces")
    {
        const auto tokens = lexAll("alpha{beta}gamma"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Scalar);
        REQUIRE(tokens[0].text == "alpha{beta}gamma");
        REQUIRE(tokens[1].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex terminates plain scalar at flow delimiters inside flow collection",
          "[job_yaml][lex][flow][plain][context]")
{
    const auto tokens = lexAll("[alpha,beta,{gamma: delta}]"sv);

    REQUIRE(tokens.size() == 12);
    REQUIRE(tokens[0].type == YamlLexType::FlowSequenceStart);
    REQUIRE(tokens[1].type == YamlLexType::Scalar);
    REQUIRE(tokens[1].text == "alpha");
    REQUIRE(tokens[2].type == YamlLexType::CollectEntry);
    REQUIRE(tokens[3].type == YamlLexType::Scalar);
    REQUIRE(tokens[3].text == "beta");
    REQUIRE(tokens[4].type == YamlLexType::CollectEntry);
    REQUIRE(tokens[5].type == YamlLexType::FlowMappingStart);
    REQUIRE(tokens[6].type == YamlLexType::Scalar);
    REQUIRE(tokens[6].text == "gamma");
    REQUIRE(tokens[7].type == YamlLexType::MappingValue);
    REQUIRE(tokens[8].type == YamlLexType::Scalar);
    REQUIRE(tokens[8].text == "delta");
    REQUIRE(tokens[9].type == YamlLexType::FlowMappingEnd);
    REQUIRE(tokens[10].type == YamlLexType::FlowSequenceEnd);
    REQUIRE(tokens[11].type == YamlLexType::End);
}

TEST_CASE("YamlLex classifies scalar styles", "[job_yaml][lex][scalar_style]")
{
    SECTION("plain")
    {
        const auto tokens = lexAll("alpha"sv);

        REQUIRE(tokens.size() == 2);
        requireLexeme(tokens[0], YamlLexType::Scalar, YamlScalarStyle::Plain, "alpha", 0);
        REQUIRE(tokens[1].scalarStyle == YamlScalarStyle::None);
    }

    SECTION("single quoted")
    {
        const auto tokens = lexAll("'alpha'"sv);

        REQUIRE(tokens.size() == 2);
        requireLexeme(tokens[0], YamlLexType::Scalar, YamlScalarStyle::SingleQuoted, "'alpha'", 0);
        REQUIRE(tokens[1].scalarStyle == YamlScalarStyle::None);
    }

    SECTION("double quoted")
    {
        const auto tokens = lexAll("\"alpha\""sv);

        REQUIRE(tokens.size() == 2);
        requireLexeme(tokens[0], YamlLexType::Scalar, YamlScalarStyle::DoubleQuoted, "\"alpha\"", 0);
        REQUIRE(tokens[1].scalarStyle == YamlScalarStyle::None);
    }
}

TEST_CASE("YamlLex scans single quoted scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("'hello world'"sv);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::SingleQuoted);
    REQUIRE(tokens[0].text == "'hello world'");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 13);
    REQUIRE(tokens[0].endOffset() == 13);

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex handles doubled quote inside single quoted scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("'it''s fine'"sv);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::SingleQuoted);
    REQUIRE(tokens[0].text == "'it''s fine'");

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex scans double quoted scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("\"hello world\""sv);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::DoubleQuoted);
    REQUIRE(tokens[0].text == "\"hello world\"");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 13);
    REQUIRE(tokens[0].endOffset() == 13);

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex skips escaped character inside double quoted scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("\"hello\\nworld\""sv);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::DoubleQuoted);
    REQUIRE(tokens[0].text == "\"hello\\nworld\"");

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex keeps escaped quote inside double quoted scalar", "[job_yaml][lex]")
{
    const auto tokens = lexAll("\"hello \\\"world\\\"\""sv);

    REQUIRE(tokens.size() == 2);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::DoubleQuoted);
    REQUIRE(tokens[0].text == "\"hello \\\"world\\\"\"");

    REQUIRE(tokens[1].type == YamlLexType::End);
}

TEST_CASE("YamlLex returns remainder as quoted token when quote is unterminated", "[job_yaml][lex]")
{
    SECTION("single quote")
    {
        const auto tokens = lexAll("'unterminated"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Scalar);
        REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::SingleQuoted);
        REQUIRE(tokens[0].text == "'unterminated");
        REQUIRE(tokens[0].offset == 0);
        REQUIRE(tokens[0].size() == 13);
        REQUIRE(tokens[0].endOffset() == 13);
        REQUIRE(tokens[1].type == YamlLexType::End);
    }

    SECTION("double quote")
    {
        const auto tokens = lexAll("\"unterminated"sv);

        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == YamlLexType::Scalar);
        REQUIRE(tokens[0].scalarStyle == YamlScalarStyle::DoubleQuoted);
        REQUIRE(tokens[0].text == "\"unterminated");
        REQUIRE(tokens[0].offset == 0);
        REQUIRE(tokens[0].size() == 13);
        REQUIRE(tokens[0].endOffset() == 13);
        REQUIRE(tokens[1].type == YamlLexType::End);
    }
}

TEST_CASE("YamlLex preserves lexeme offsets and ranges", "[job_yaml][lex][range]")
{
    constexpr std::string_view source = "one: two\n  - three";

    const auto tokens = lexAll(source);

    REQUIRE(tokens.size() == 8);

    REQUIRE(tokens[0].type == YamlLexType::Scalar);
    REQUIRE(tokens[0].text == "one");
    REQUIRE(tokens[0].offset == 0);
    REQUIRE(tokens[0].size() == 3);
    REQUIRE(tokens[0].endOffset() == 3);

    REQUIRE(tokens[1].type == YamlLexType::MappingValue);
    REQUIRE(tokens[1].offset == 3);
    REQUIRE(tokens[1].size() == 1);
    REQUIRE(tokens[1].endOffset() == 4);

    REQUIRE(tokens[2].type == YamlLexType::Scalar);
    REQUIRE(tokens[2].text == "two");
    REQUIRE(tokens[2].offset == 5);
    REQUIRE(tokens[2].size() == 3);
    REQUIRE(tokens[2].endOffset() == 8);

    REQUIRE(tokens[3].type == YamlLexType::LineBreak);
    REQUIRE(tokens[3].offset == 8);
    REQUIRE(tokens[3].size() == 1);
    REQUIRE(tokens[3].endOffset() == 9);

    REQUIRE(tokens[4].type == YamlLexType::Indent);
    REQUIRE(tokens[4].text == "  ");
    REQUIRE(tokens[4].offset == 9);
    REQUIRE(tokens[4].size() == 2);
    REQUIRE(tokens[4].endOffset() == 11);

    REQUIRE(tokens[5].type == YamlLexType::SequenceEntry);
    REQUIRE(tokens[5].offset == 11);
    REQUIRE(tokens[5].size() == 1);
    REQUIRE(tokens[5].endOffset() == 12);

    REQUIRE(tokens[6].type == YamlLexType::Scalar);
    REQUIRE(tokens[6].text == "three");
    REQUIRE(tokens[6].offset == 13);
    REQUIRE(tokens[6].size() == 5);
    REQUIRE(tokens[6].endOffset() == 18);

    REQUIRE(tokens[7].type == YamlLexType::End);
    REQUIRE(tokens[7].offset == 18);
    REQUIRE(tokens[7].size() == 0);
    REQUIRE(tokens[7].endOffset() == 18);
}

TEST_CASE("YamlLex lexeme ranges reconstruct exact source slices", "[job_yaml][lex][range]")
{
    constexpr std::string_view source = "name: alpha\n  - beta";

    const auto tokens = lexAll(source);

    for (const YamlLexeme &token : tokens) {
        if (token.type == YamlLexType::End)
            continue;

        const YamlSourceRange range = token.range();

        REQUIRE(range.offset == token.offset);
        REQUIRE(range.size == token.size());
        REQUIRE(range.endOffset() == token.endOffset());
        REQUIRE(range.view(source) == token.text);
    }
}

TEST_CASE("YamlLex scalar views point directly into source", "[job_yaml][lex][range][view]")
{
    constexpr std::string_view source = "name: alpha";

    YamlLex lex{source};

    const YamlLexeme key = lex.next();
    const YamlLexeme separator = lex.next();
    const YamlLexeme value = lex.next();

    REQUIRE(key.type == YamlLexType::Scalar);
    REQUIRE(key.text == "name");
    REQUIRE(key.text.data() == source.data());

    REQUIRE(separator.type == YamlLexType::MappingValue);
    REQUIRE(separator.text.data() == source.data() + 4);

    REQUIRE(value.type == YamlLexType::Scalar);
    REQUIRE(value.text == "alpha");
    REQUIRE(value.offset == 6);
    REQUIRE(value.text.data() == source.data() + value.offset);
    REQUIRE(value.range().view(source).data() == value.text.data());
}

TEST_CASE("YamlLex exposes its source", "[job_yaml][lex][range]")
{
    constexpr std::string_view source = "alpha";

    YamlLex lex{source};

    REQUIRE(lex.source() == source);
    REQUIRE(lex.source().data() == source.data());

    const YamlLexeme scalar = lex.next();

    REQUIRE(scalar.text.data() == lex.source().data());
}

TEST_CASE("YamlLex exposes its cursor", "[job_yaml][lex]")
{
    YamlLex lex{"abc"};

    REQUIRE(lex.cursor().offset() == 0);
    REQUIRE(lex.cursor().current() == 'a');

    const auto token = lex.next();

    REQUIRE(token.type == YamlLexType::Scalar);
    REQUIRE(lex.cursor().offset() == 3);
    REQUIRE(lex.cursor().empty());

    lex.cursor().reset();

    REQUIRE(lex.cursor().offset() == 0);
    REQUIRE(lex.cursor().current() == 'a');
}

} // namespace job::yaml::tests