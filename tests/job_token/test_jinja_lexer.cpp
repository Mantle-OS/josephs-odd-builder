#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <chat/jinja_lexer.h>
#include <chat/jinja_token.h>

using job::token::JinjaLexer;
using job::token::JinjaToken;
using job::token::JinjaType;
using job::token::jinjaTypeString;

//
// Block 1: usage / examples
//

TEST_CASE("JinjaLexer tokenizes plain text", "[token][jinja][lexer][usage]")
{
    JinjaLexer lexer{"Hello, compiler cousin!"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "Hello, compiler cousin!");
    REQUIRE(tokens[1].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer tokenizes output expression", "[token][jinja][lexer][usage][expression]")
{
    JinjaLexer lexer{"{{ message.content }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[0].text == "{{");
    REQUIRE(tokens[1].type == JinjaType::Identifier);
    REQUIRE(tokens[1].text == "message");
    REQUIRE(tokens[2].type == JinjaType::Dot);
    REQUIRE(tokens[2].text == ".");
    REQUIRE(tokens[3].type == JinjaType::Identifier);
    REQUIRE(tokens[3].text == "content");
    REQUIRE(tokens[4].type == JinjaType::ExprEnd);
    REQUIRE(tokens[4].text == "}}");
    REQUIRE(tokens[5].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer tokenizes for block", "[token][jinja][lexer][usage][block]")
{
    JinjaLexer lexer{"{% for message in messages %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 7);
    REQUIRE(tokens[0].type == JinjaType::BlockBegin);
    REQUIRE(tokens[0].text == "{%");
    REQUIRE(tokens[1].type == JinjaType::KwFor);
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "message");
    REQUIRE(tokens[3].type == JinjaType::KwIn);
    REQUIRE(tokens[4].type == JinjaType::Identifier);
    REQUIRE(tokens[4].text == "messages");
    REQUIRE(tokens[5].type == JinjaType::BlockEnd);
    REQUIRE(tokens[5].text == "%}");
    REQUIRE(tokens[6].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer tokenizes realistic chat template fragment", "[token][jinja][lexer][usage][chat]")
{
    static constexpr std::string_view Source =
        "{{ bos_token }}"
        "{% for message in messages %}"
        "{{ message.role }}={{ message.content }};"
        "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE_FALSE(tokens.empty());
    REQUIRE(tokens.back().type == JinjaType::Eof);
    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].type == JinjaType::Identifier);
    REQUIRE(tokens[1].text == "bos_token");
    REQUIRE(tokens[2].type == JinjaType::ExprEnd);
    REQUIRE(tokens[3].type == JinjaType::BlockBegin);
    REQUIRE(tokens[4].type == JinjaType::KwFor);
    REQUIRE(tokens[5].type == JinjaType::Identifier);
    REQUIRE(tokens[5].text == "message");
    REQUIRE(tokens[6].type == JinjaType::KwIn);
    REQUIRE(tokens[7].type == JinjaType::Identifier);
    REQUIRE(tokens[7].text == "messages");
    REQUIRE(tokens[8].type == JinjaType::BlockEnd);
}

TEST_CASE("JinjaLexer tokenizes single and double quoted strings", "[token][jinja][lexer][usage][string]")
{
    JinjaLexer lexer{"{{ 'hello' + \"world\" }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].type == JinjaType::StringLiteral);
    REQUIRE(tokens[1].text == "hello");
    REQUIRE(tokens[2].type == JinjaType::Plus);
    REQUIRE(tokens[3].type == JinjaType::StringLiteral);
    REQUIRE(tokens[3].text == "world");
    REQUIRE(tokens[4].type == JinjaType::ExprEnd);
    REQUIRE(tokens[5].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer keeps escaped characters inside string token", "[token][jinja][lexer][usage][string][escape]")
{
    JinjaLexer lexer{R"({{ "hello\"world" }})"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[1].type == JinjaType::StringLiteral);
    REQUIRE(tokens[1].text == R"(hello\"world)");
}

TEST_CASE("JinjaLexer tokenizes integer and floating point literals", "[token][jinja][lexer][usage][number]")
{
    JinjaLexer lexer{"{{ 42 + 3.14159 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[1].type == JinjaType::NumberLiteral);
    REQUIRE(tokens[1].text == "42");
    REQUIRE(tokens[2].type == JinjaType::Plus);
    REQUIRE(tokens[3].type == JinjaType::NumberLiteral);
    REQUIRE(tokens[3].text == "3.14159");
}

TEST_CASE("JinjaLexer allows only one decimal point in a number token", "[token][jinja][lexer][usage][number]")
{
    JinjaLexer lexer{"{{ 12.34.56 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[1].type == JinjaType::NumberLiteral);
    REQUIRE(tokens[1].text == "12.34");
    REQUIRE(tokens[2].type == JinjaType::Dot);
    REQUIRE(tokens[2].text == ".");
    REQUIRE(tokens[3].type == JinjaType::NumberLiteral);
    REQUIRE(tokens[3].text == "56");
}

TEST_CASE("JinjaLexer recognizes control flow keywords", "[token][jinja][lexer][usage][keyword]")
{
    JinjaLexer lexer{
        "{% for value in values "
        "endfor if elif else endif set "
        "and or not is %}"
    };

    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[1].type == JinjaType::KwFor);
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "value");
    REQUIRE(tokens[3].type == JinjaType::KwIn);
    REQUIRE(tokens[4].type == JinjaType::Identifier);
    REQUIRE(tokens[4].text == "values");
    REQUIRE(tokens[5].type == JinjaType::KwEndFor);
    REQUIRE(tokens[6].type == JinjaType::KwIf);
    REQUIRE(tokens[7].type == JinjaType::KwElif);
    REQUIRE(tokens[8].type == JinjaType::KwElse);
    REQUIRE(tokens[9].type == JinjaType::KwEndIf);
    REQUIRE(tokens[10].type == JinjaType::KwSet);
    REQUIRE(tokens[11].type == JinjaType::KwAnd);
    REQUIRE(tokens[12].type == JinjaType::KwOr);
    REQUIRE(tokens[13].type == JinjaType::KwNot);
    REQUIRE(tokens[14].type == JinjaType::KwIs);
    REQUIRE(tokens[15].type == JinjaType::BlockEnd);
    REQUIRE(tokens[16].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer recognizes boolean and null keywords", "[token][jinja][lexer][usage][keyword][literal]")
{
    JinjaLexer lexer{"{{ true True false False none None null }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 10);
    REQUIRE(tokens[1].type == JinjaType::KwTrue);
    REQUIRE(tokens[1].text == "true");
    REQUIRE(tokens[2].type == JinjaType::KwTrue);
    REQUIRE(tokens[2].text == "True");
    REQUIRE(tokens[3].type == JinjaType::KwFalse);
    REQUIRE(tokens[3].text == "false");
    REQUIRE(tokens[4].type == JinjaType::KwFalse);
    REQUIRE(tokens[4].text == "False");
    REQUIRE(tokens[5].type == JinjaType::KwNone);
    REQUIRE(tokens[5].text == "none");
    REQUIRE(tokens[6].type == JinjaType::KwNone);
    REQUIRE(tokens[6].text == "None");
    REQUIRE(tokens[7].type == JinjaType::KwNone);
    REQUIRE(tokens[7].text == "null");
    REQUIRE(tokens[8].type == JinjaType::ExprEnd);
    REQUIRE(tokens[9].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer leaves keyword prefixes as identifiers", "[token][jinja][lexer][usage][keyword]")
{
    JinjaLexer lexer{"{{ format inside endif_value true_value }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[1].type == JinjaType::Identifier);
    REQUIRE(tokens[1].text == "format");
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "inside");
    REQUIRE(tokens[3].type == JinjaType::Identifier);
    REQUIRE(tokens[3].text == "endif_value");
    REQUIRE(tokens[4].type == JinjaType::Identifier);
    REQUIRE(tokens[4].text == "true_value");
}

TEST_CASE("JinjaLexer tokenizes comparison operators", "[token][jinja][lexer][usage][operator]")
{
    JinjaLexer lexer{"{{ a == b != c < d <= e > f >= g }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[2].type == JinjaType::Eq);
    REQUIRE(tokens[2].text == "==");
    REQUIRE(tokens[4].type == JinjaType::Ne);
    REQUIRE(tokens[4].text == "!=");
    REQUIRE(tokens[6].type == JinjaType::Lt);
    REQUIRE(tokens[6].text == "<");
    REQUIRE(tokens[8].type == JinjaType::Le);
    REQUIRE(tokens[8].text == "<=");
    REQUIRE(tokens[10].type == JinjaType::Gt);
    REQUIRE(tokens[10].text == ">");
    REQUIRE(tokens[12].type == JinjaType::Ge);
    REQUIRE(tokens[12].text == ">=");
}

TEST_CASE("JinjaLexer tokenizes arithmetic and concatenation operators", "[token][jinja][lexer][usage][operator]")
{
    JinjaLexer lexer{"{{ a + b - c * d / e // f % g ** h ~ i }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[2].type == JinjaType::Plus);
    REQUIRE(tokens[4].type == JinjaType::Minus);
    REQUIRE(tokens[6].type == JinjaType::Mul);
    REQUIRE(tokens[8].type == JinjaType::Div);
    REQUIRE(tokens[10].type == JinjaType::FloorDiv);
    REQUIRE(tokens[12].type == JinjaType::Mod);
    REQUIRE(tokens[14].type == JinjaType::Pow);
    REQUIRE(tokens[16].type == JinjaType::Tilde);
}

TEST_CASE("JinjaLexer distinguishes multiply from power", "[token][jinja][lexer][usage][operator][power]")
{
    JinjaLexer lexer{"{{ a * b ** c }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[2].type == JinjaType::Mul);
    REQUIRE(tokens[2].text == "*");
    REQUIRE(tokens[4].type == JinjaType::Pow);
    REQUIRE(tokens[4].text == "**");
}

TEST_CASE("JinjaLexer distinguishes division from floor division", "[token][jinja][lexer][usage][operator][floor-div]")
{
    JinjaLexer lexer{"{{ a / b // c }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[2].type == JinjaType::Div);
    REQUIRE(tokens[2].text == "/");
    REQUIRE(tokens[4].type == JinjaType::FloorDiv);
    REQUIRE(tokens[4].text == "//");
}

TEST_CASE("JinjaLexer tokenizes assignment", "[token][jinja][lexer][usage][operator][assign]")
{
    JinjaLexer lexer{"{% set answer = 42 %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 7);
    REQUIRE(tokens[0].type == JinjaType::BlockBegin);
    REQUIRE(tokens[1].type == JinjaType::KwSet);
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "answer");
    REQUIRE(tokens[3].type == JinjaType::Assign);
    REQUIRE(tokens[3].text == "=");
    REQUIRE(tokens[4].type == JinjaType::NumberLiteral);
    REQUIRE(tokens[4].text == "42");
    REQUIRE(tokens[5].type == JinjaType::BlockEnd);
    REQUIRE(tokens[6].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer tokenizes punctuation", "[token][jinja][lexer][usage][punctuation]")
{
    JinjaLexer lexer{"{{ a.b, c: d; e | f(x)[0] }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    bool foundDot = false;
    bool foundComma = false;
    bool foundColon = false;
    bool foundSemicolon = false;
    bool foundPipe = false;
    bool foundParenOpen = false;
    bool foundParenClose = false;
    bool foundBracketOpen = false;
    bool foundBracketClose = false;

    for (const JinjaToken &token : tokens) {
        foundDot |= token.type == JinjaType::Dot;
        foundComma |= token.type == JinjaType::Comma;
        foundColon |= token.type == JinjaType::Colon;
        foundSemicolon |= token.type == JinjaType::Semicolon;
        foundPipe |= token.type == JinjaType::Pipe;
        foundParenOpen |= token.type == JinjaType::ParenOpen;
        foundParenClose |= token.type == JinjaType::ParenClose;
        foundBracketOpen |= token.type == JinjaType::BracketOpen;
        foundBracketClose |= token.type == JinjaType::BracketClose;
    }

    REQUIRE(foundDot);
    REQUIRE(foundComma);
    REQUIRE(foundColon);
    REQUIRE(foundSemicolon);
    REQUIRE(foundPipe);
    REQUIRE(foundParenOpen);
    REQUIRE(foundParenClose);
    REQUIRE(foundBracketOpen);
    REQUIRE(foundBracketClose);
}

TEST_CASE("JinjaLexer tokenizes braces inside expression", "[token][jinja][lexer][usage][punctuation][brace]")
{
    JinjaLexer lexer{"{{ { value } }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[1].type == JinjaType::BraceOpen);
    REQUIRE(tokens[1].text == "{");
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "value");
    REQUIRE(tokens[3].type == JinjaType::BraceClose);
    REQUIRE(tokens[3].text == "}");
}

TEST_CASE("JinjaLexer ignores whitespace inside tags", "[token][jinja][lexer][usage][whitespace]")
{
    JinjaLexer lexer{"{{      value       }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].type == JinjaType::Identifier);
    REQUIRE(tokens[1].text == "value");
    REQUIRE(tokens[2].type == JinjaType::ExprEnd);
    REQUIRE(tokens[3].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer ignores whitespace inside block tags", "[token][jinja][lexer][usage][whitespace][block]")
{
    JinjaLexer lexer{"{%      if      value      %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 5);
    REQUIRE(tokens[0].type == JinjaType::BlockBegin);
    REQUIRE(tokens[1].type == JinjaType::KwIf);
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "value");
    REQUIRE(tokens[3].type == JinjaType::BlockEnd);
    REQUIRE(tokens[4].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer removes comments from token stream", "[token][jinja][lexer][usage][comment]")
{
    JinjaLexer lexer{"before{# Euler was definitely here #}after"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "before");
    REQUIRE(tokens[1].type == JinjaType::Text);
    REQUIRE(tokens[1].text == "after");
    REQUIRE(tokens[2].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer removes standalone comment", "[token][jinja][lexer][usage][comment]")
{
    JinjaLexer lexer{"{# nothing to see here #}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer trims whitespace before expression opening", "[token][jinja][lexer][usage][trim][expression]")
{
    JinjaLexer lexer{"Leading   {{- value }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "Leading");
    REQUIRE(tokens[1].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].text == "{{-");
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "value");
}

TEST_CASE("JinjaLexer trims whitespace after expression closing", "[token][jinja][lexer][usage][trim][expression]")
{
    JinjaLexer lexer{"{{ value -}}   Trailing"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].type == JinjaType::Identifier);
    REQUIRE(tokens[2].type == JinjaType::ExprEnd);
    REQUIRE(tokens[2].text == "-}}");
    REQUIRE(tokens[3].type == JinjaType::Text);
    REQUIRE(tokens[3].text == "Trailing");
    REQUIRE(tokens[4].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer trims whitespace around expression delimiters", "[token][jinja][lexer][usage][trim][expression]")
{
    JinjaLexer lexer{"Leading   {{- 'trimmed' -}}   Trailing"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "Leading");
    REQUIRE(tokens[1].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].text == "{{-");
    REQUIRE(tokens[2].type == JinjaType::StringLiteral);
    REQUIRE(tokens[2].text == "trimmed");
    REQUIRE(tokens[3].type == JinjaType::ExprEnd);
    REQUIRE(tokens[3].text == "-}}");
    REQUIRE(tokens[4].type == JinjaType::Text);
    REQUIRE(tokens[4].text == "Trailing");
    REQUIRE(tokens[5].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer trims whitespace around block delimiters", "[token][jinja][lexer][usage][trim][block]")
{
    JinjaLexer lexer{"before   {%- if value -%}   after"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 7);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "before");
    REQUIRE(tokens[1].type == JinjaType::BlockBegin);
    REQUIRE(tokens[1].text == "{%-");
    REQUIRE(tokens[2].type == JinjaType::KwIf);
    REQUIRE(tokens[3].type == JinjaType::Identifier);
    REQUIRE(tokens[3].text == "value");
    REQUIRE(tokens[4].type == JinjaType::BlockEnd);
    REQUIRE(tokens[4].text == "-%}");
    REQUIRE(tokens[5].type == JinjaType::Text);
    REQUIRE(tokens[5].text == "after");
    REQUIRE(tokens[6].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer trimmed comment strips surrounding whitespace", "[token][jinja][lexer][usage][comment][trim]")
{
    JinjaLexer lexer{"before   {#- drunk cousin commentary -#}   after"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "before");
    REQUIRE(tokens[1].type == JinjaType::Text);
    REQUIRE(tokens[1].text == "after");
    REQUIRE(tokens[2].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer comment without trim preserves surrounding text whitespace", "[token][jinja][lexer][usage][comment]")
{
    JinjaLexer lexer{"before   {# comment #}   after"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "before   ");
    REQUIRE(tokens[1].type == JinjaType::Text);
    REQUIRE(tokens[1].text == "   after");
    REQUIRE(tokens[2].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer reset starts tokenization from new source", "[token][jinja][lexer][usage][reset]")
{
    JinjaLexer lexer{"{{ first }}"};

    {
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

        REQUIRE(tokens[1].type == JinjaType::Identifier);
        REQUIRE(tokens[1].text == "first");
    }

    lexer.reset("{{ second }}");

    {
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

        REQUIRE(tokens[1].type == JinjaType::Identifier);
        REQUIRE(tokens[1].text == "second");
        REQUIRE(tokens[0].line == 1);
        REQUIRE(tokens[0].column == 1);
        REQUIRE(tokens[1].line == 1);
        REQUIRE(tokens[1].column == 4);
    }
}

TEST_CASE("JinjaLexer tracks token line and column", "[token][jinja][lexer][usage][position]")
{
    JinjaLexer lexer{"first\n" "{{ second }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].line == 1);
    REQUIRE(tokens[0].column == 1);
    REQUIRE(tokens[1].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].line == 2);
    REQUIRE(tokens[1].column == 1);
    REQUIRE(tokens[2].type == JinjaType::Identifier);
    REQUIRE(tokens[2].text == "second");
    REQUIRE(tokens[2].line == 2);
    REQUIRE(tokens[2].column == 4);
    REQUIRE(tokens[3].type == JinjaType::ExprEnd);
    REQUIRE(tokens[3].line == 2);
}

TEST_CASE("JinjaLexer tracks lines through whitespace inside tags", "[token][jinja][lexer][usage][position]")
{
    JinjaLexer lexer{"{{\n" "    value\n" "}}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[0].line == 1);
    REQUIRE(tokens[0].column == 1);
    REQUIRE(tokens[1].type == JinjaType::Identifier);
    REQUIRE(tokens[1].text == "value");
    REQUIRE(tokens[1].line == 2);
    REQUIRE(tokens[1].column == 5);
    REQUIRE(tokens[2].type == JinjaType::ExprEnd);
    REQUIRE(tokens[2].line == 3);
    REQUIRE(tokens[2].column == 1);
}

TEST_CASE("JinjaLexer nextToken consumes stream incrementally", "[token][jinja][lexer][usage][incremental]")
{
    JinjaLexer lexer{"{{ value }}"};

    JinjaToken token = lexer.nextToken();
    REQUIRE(token.type == JinjaType::ExprBegin);

    token = lexer.nextToken();
    REQUIRE(token.type == JinjaType::Identifier);
    REQUIRE(token.text == "value");

    token = lexer.nextToken();
    REQUIRE(token.type == JinjaType::ExprEnd);

    token = lexer.nextToken();
    REQUIRE(token.type == JinjaType::Eof);
}

TEST_CASE("Jinja token type strings describe lexer tokens", "[token][jinja][lexer][usage][type]")
{
    REQUIRE(jinjaTypeString(JinjaType::ExprBegin) == "{{");
    REQUIRE(jinjaTypeString(JinjaType::FloorDiv) == "//");
    REQUIRE(jinjaTypeString(JinjaType::Pow) == "**");
    REQUIRE(jinjaTypeString(JinjaType::Semicolon) == ";");
    REQUIRE(jinjaTypeString(JinjaType::Unknown) == "Unknown");
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("JinjaLexer empty input produces only EOF", "[token][jinja][lexer][edge]")
{
    JinjaLexer lexer{""};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == JinjaType::Eof);
    REQUIRE(tokens[0].text.empty());
    REQUIRE(tokens[0].line == 1);
    REQUIRE(tokens[0].column == 1);
}

TEST_CASE("JinjaLexer whitespace only input remains text", "[token][jinja][lexer][edge][whitespace]")
{
    JinjaLexer lexer{"   \n\t  "};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "   \n\t  ");
    REQUIRE(tokens[1].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer preserves ordinary braces as text", "[token][jinja][lexer][edge][text]")
{
    JinjaLexer lexer{"one { two } three"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "one { two } three");
    REQUIRE(tokens[1].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer preserves lone opening brace as text", "[token][jinja][lexer][edge][text]")
{
    JinjaLexer lexer{"hello {"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "hello {");
}

TEST_CASE("JinjaLexer emits unknown token for unsupported character inside tag", "[token][jinja][lexer][edge][unknown]")
{
    JinjaLexer lexer{"{{ @ }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[0].type == JinjaType::ExprBegin);
    REQUIRE(tokens[1].type == JinjaType::Unknown);
    REQUIRE(tokens[1].text == "@");
    REQUIRE(tokens[2].type == JinjaType::ExprEnd);
    REQUIRE(tokens[3].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer unterminated string becomes unknown token", "[token][jinja][lexer][edge][string]")
{
    JinjaLexer lexer{"{{ 'why_didnt_you_end"};

    const JinjaToken expressionBegin = lexer.nextToken();
    REQUIRE(expressionBegin.type == JinjaType::ExprBegin);

    const JinjaToken string = lexer.nextToken();
    REQUIRE(string.type == JinjaType::Unknown);
    REQUIRE(string.text == "why_didnt_you_end");
}

TEST_CASE("JinjaLexer unterminated expression reports unknown at tag EOF", "[token][jinja][lexer][edge][expression]")
{
    JinjaLexer lexer{"{{ value"};

    const JinjaToken expressionBegin = lexer.nextToken();
    REQUIRE(expressionBegin.type == JinjaType::ExprBegin);

    const JinjaToken value = lexer.nextToken();
    REQUIRE(value.type == JinjaType::Identifier);
    REQUIRE(value.text == "value");

    const JinjaToken malformedEnd = lexer.nextToken();
    REQUIRE(malformedEnd.type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer unterminated block reports EOF after last token", "[token][jinja][lexer][edge][block]")
{
    JinjaLexer lexer{"{% if value"};

    const JinjaToken blockBegin = lexer.nextToken();
    REQUIRE(blockBegin.type == JinjaType::BlockBegin);

    const JinjaToken keyword = lexer.nextToken();
    REQUIRE(keyword.type == JinjaType::KwIf);

    const JinjaToken value = lexer.nextToken();
    REQUIRE(value.type == JinjaType::Identifier);

    const JinjaToken malformedEnd = lexer.nextToken();
    REQUIRE(malformedEnd.type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer unterminated comment is discarded through EOF", "[token][jinja][lexer][edge][comment]")
{
    JinjaLexer lexer{"before{# Euler wandered off again"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "before");
    REQUIRE(tokens[1].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer standalone unterminated comment produces EOF", "[token][jinja][lexer][edge][comment]")
{
    JinjaLexer lexer{"{# still no closing comment"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == JinjaType::Eof);
}

TEST_CASE("JinjaLexer repeated EOF calls remain EOF", "[token][jinja][lexer][edge][eof]")
{
    JinjaLexer lexer{""};

    const JinjaToken first = lexer.nextToken();
    const JinjaToken second = lexer.nextToken();

    REQUIRE(first.type == JinjaType::Eof);
    REQUIRE(second.type == JinjaType::Eof);
    REQUIRE(first.line == second.line);
    REQUIRE(first.column == second.column);
}

TEST_CASE("JinjaLexer reset clears whitespace trimming state", "[token][jinja][lexer][edge][reset]")
{
    JinjaLexer lexer{"{{ value -}}   stripped"};

    REQUIRE(lexer.nextToken().type == JinjaType::ExprBegin);
    REQUIRE(lexer.nextToken().type == JinjaType::Identifier);
    REQUIRE(lexer.nextToken().type == JinjaType::ExprEnd);

    lexer.reset("   preserved");

    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == JinjaType::Text);
    REQUIRE(tokens[0].text == "   preserved");
}

//
// Block 3: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JinjaLexer chat template tokenization", "[token][jinja][lexer][benchmark]")
{
    std::string source;

    for (std::size_t i = 0; i < 50; ++i) {
        source +=
            "{% for message in messages %}"
            "{% if message.role == 'user' %}"
            "{{ message.role }}: {{ message.content }}"
            "{% endif %}"
            "{% endfor %}\n";
    }

    BENCHMARK("Tokenize repeated Jinja chat template")
    {
        JinjaLexer lexer{source};
        return lexer.tokenizeAll();
    };
}

#endif