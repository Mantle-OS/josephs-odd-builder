#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <chat/jinja_ast.h>
#include <chat/jinja_lexer.h>
#include <chat/jinja_parser.h>
#include <chat/jinja_token.h>

using job::token::BinaryOp;
using job::token::BinaryOpNode;
using job::token::BodyNode;
using job::token::CallNode;
using job::token::ConditionalNode;
using job::token::DictNode;
using job::token::FilterNode;
using job::token::ForNode;
using job::token::IdentifierNode;
using job::token::IfNode;
using job::token::JinjaLexer;
using job::token::JinjaParser;
using job::token::JinjaToken;
using job::token::JinjaType;
using job::token::ListNode;
using job::token::LiteralNode;
using job::token::MemberAccessNode;
using job::token::NodeType;
using job::token::OutputNode;
using job::token::ParseError;
using job::token::SetNode;
using job::token::SubscriptNode;
using job::token::TextNode;
using job::token::UnaryOp;
using job::token::UnaryOpNode;

//
// Block 1: usage / examples
//

TEST_CASE("JinjaParser parses plain text", "[token][jinja][parser][usage]")
{
    JinjaLexer lexer{"Hello compiler!"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->type == NodeType::Body);
    REQUIRE(root->statements.size() == 1);
    REQUIRE(root->statements[0]->type == NodeType::Text);

    const auto *text = static_cast<const TextNode *>(root->statements[0].get());

    REQUIRE(text->text == "Hello compiler!");
}

TEST_CASE("JinjaParser parses output identifier", "[token][jinja][parser][usage][output]")
{
    JinjaLexer lexer{"{{ value }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->statements.size() == 1);
    REQUIRE(root->statements[0]->type == NodeType::Output);

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression != nullptr);
    REQUIRE(output->expression->type == NodeType::Identifier);

    const auto *identifier = static_cast<const IdentifierNode *>(output->expression.get());

    REQUIRE(identifier->name == "value");
}

TEST_CASE("JinjaParser preserves statement ordering", "[token][jinja][parser][usage][body]")
{
    JinjaLexer lexer{"before{{ value }}after"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->statements.size() == 3);
    REQUIRE(root->statements[0]->type == NodeType::Text);
    REQUIRE(root->statements[1]->type == NodeType::Output);
    REQUIRE(root->statements[2]->type == NodeType::Text);

    const auto *before = static_cast<const TextNode *>(root->statements[0].get());
    const auto *after = static_cast<const TextNode *>(root->statements[2].get());

    REQUIRE(before->text == "before");
    REQUIRE(after->text == "after");
}

TEST_CASE("JinjaParser parses numeric literals", "[token][jinja][parser][usage][literal]")
{
    SECTION("integer")
    {
        JinjaLexer lexer{"{{ 42 }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::Literal);

        const auto *literal = static_cast<const LiteralNode *>(output->expression.get());

        REQUIRE(std::holds_alternative<std::int64_t>(literal->value));
        REQUIRE(std::get<std::int64_t>(literal->value) == 42);
    }

    SECTION("floating point")
    {
        JinjaLexer lexer{"{{ 3.14159 }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::Literal);

        const auto *literal = static_cast<const LiteralNode *>(output->expression.get());

        REQUIRE(std::holds_alternative<double>(literal->value));
        REQUIRE(std::get<double>(literal->value) == 3.14159);
    }
}

TEST_CASE("JinjaParser parses string literal", "[token][jinja][parser][usage][literal]")
{
    JinjaLexer lexer{"{{ 'drunk cousin' }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Literal);

    const auto *literal = static_cast<const LiteralNode *>(output->expression.get());

    REQUIRE(std::holds_alternative<std::string>(literal->value));
    REQUIRE(std::get<std::string>(literal->value) == "drunk cousin");
}

TEST_CASE("JinjaParser parses boolean and none literals", "[token][jinja][parser][usage][literal]")
{
    SECTION("true")
    {
        JinjaLexer lexer{"{{ true }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::Literal);

        const auto *literal = static_cast<const LiteralNode *>(output->expression.get());

        REQUIRE(std::holds_alternative<bool>(literal->value));
        REQUIRE(std::get<bool>(literal->value));
    }

    SECTION("false")
    {
        JinjaLexer lexer{"{{ false }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *literal = static_cast<const LiteralNode *>(output->expression.get());

        REQUIRE(std::holds_alternative<bool>(literal->value));
        REQUIRE_FALSE(std::get<bool>(literal->value));
    }

    SECTION("none")
    {
        JinjaLexer lexer{"{{ none }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *literal = static_cast<const LiteralNode *>(output->expression.get());

        REQUIRE(std::holds_alternative<std::nullptr_t>(literal->value));
    }
}

TEST_CASE("JinjaParser parses member access", "[token][jinja][parser][usage][postfix]")
{
    JinjaLexer lexer{"{{ message.content }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::MemberAccess);

    const auto *member = static_cast<const MemberAccessNode *>(output->expression.get());

    REQUIRE(member->member == "content");
    REQUIRE(member->object->type == NodeType::Identifier);

    const auto *object = static_cast<const IdentifierNode *>(member->object.get());

    REQUIRE(object->name == "message");
}

TEST_CASE("JinjaParser parses chained member access", "[token][jinja][parser][usage][postfix]")
{
    JinjaLexer lexer{"{{ message.author.name }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::MemberAccess);

    const auto *name = static_cast<const MemberAccessNode *>(output->expression.get());

    REQUIRE(name->member == "name");
    REQUIRE(name->object->type == NodeType::MemberAccess);

    const auto *author = static_cast<const MemberAccessNode *>(name->object.get());

    REQUIRE(author->member == "author");
    REQUIRE(author->object->type == NodeType::Identifier);

    const auto *message = static_cast<const IdentifierNode *>(author->object.get());

    REQUIRE(message->name == "message");
}

TEST_CASE("JinjaParser parses subscript expression", "[token][jinja][parser][usage][postfix]")
{
    JinjaLexer lexer{"{{ message['content'] }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Subscript);

    const auto *subscript = static_cast<const SubscriptNode *>(output->expression.get());

    REQUIRE(subscript->object->type == NodeType::Identifier);
    REQUIRE(subscript->index->type == NodeType::Literal);

    const auto *index = static_cast<const LiteralNode *>(subscript->index.get());

    REQUIRE(std::holds_alternative<std::string>(index->value));
    REQUIRE(std::get<std::string>(index->value) == "content");
}

TEST_CASE("JinjaParser parses function call arguments", "[token][jinja][parser][usage][call]")
{
    JinjaLexer lexer{"{{ function(1, value, name='Euler') }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Call);

    const auto *call = static_cast<const CallNode *>(output->expression.get());

    REQUIRE(call->callee->type == NodeType::Identifier);

    const auto *callee = static_cast<const IdentifierNode *>(call->callee.get());

    REQUIRE(callee->name == "function");
    REQUIRE(call->posArgs.size() == 2);
    REQUIRE(call->kwArgs.size() == 1);
    REQUIRE(call->kwArgs[0].first == "name");
    REQUIRE(call->kwArgs[0].second->type == NodeType::Literal);
}

TEST_CASE("JinjaParser accepts empty and trailing comma function arguments", "[token][jinja][parser][usage][call]")
{
    SECTION("empty")
    {
        JinjaLexer lexer{"{{ function() }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *call = static_cast<const CallNode *>(output->expression.get());

        REQUIRE(call->posArgs.empty());
        REQUIRE(call->kwArgs.empty());
    }

    SECTION("trailing comma")
    {
        JinjaLexer lexer{"{{ function(1, 2,) }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *call = static_cast<const CallNode *>(output->expression.get());

        REQUIRE(call->posArgs.size() == 2);
        REQUIRE(call->kwArgs.empty());
    }
}

TEST_CASE("JinjaParser parses filters", "[token][jinja][parser][usage][filter]")
{
    JinjaLexer lexer{"{{ value | trim }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Filter);

    const auto *filter = static_cast<const FilterNode *>(output->expression.get());

    REQUIRE(filter->filterName == "trim");
    REQUIRE(filter->target->type == NodeType::Identifier);
}

TEST_CASE("JinjaParser parses chained filters in order", "[token][jinja][parser][usage][filter]")
{
    JinjaLexer lexer{"{{ value | trim | upper }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Filter);

    const auto *upper = static_cast<const FilterNode *>(output->expression.get());

    REQUIRE(upper->filterName == "upper");
    REQUIRE(upper->target->type == NodeType::Filter);

    const auto *trim = static_cast<const FilterNode *>(upper->target.get());

    REQUIRE(trim->filterName == "trim");
}

TEST_CASE("JinjaParser parses filter arguments", "[token][jinja][parser][usage][filter]")
{
    JinjaLexer lexer{"{{ value | replace('a', 'b', count=2) }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
    const auto *filter = static_cast<const FilterNode *>(output->expression.get());

    REQUIRE(filter->filterName == "replace");
    REQUIRE(filter->args.size() == 2);
    REQUIRE(filter->kwArgs.size() == 1);
    REQUIRE(filter->kwArgs[0].first == "count");
}

TEST_CASE("JinjaParser parses list literals", "[token][jinja][parser][usage][list]")
{
    SECTION("normal list")
    {
        JinjaLexer lexer{"{{ [1, 2, 3] }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::List);

        const auto *list = static_cast<const ListNode *>(output->expression.get());

        REQUIRE(list->elements.size() == 3);
    }

    SECTION("empty list")
    {
        JinjaLexer lexer{"{{ [] }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *list = static_cast<const ListNode *>(output->expression.get());

        REQUIRE(list->elements.empty());
    }

    SECTION("trailing comma")
    {
        JinjaLexer lexer{"{{ [1, 2,] }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *list = static_cast<const ListNode *>(output->expression.get());

        REQUIRE(list->elements.size() == 2);
    }
}

TEST_CASE("JinjaParser parses dictionary literals", "[token][jinja][parser][usage][dict]")
{
    SECTION("normal dictionary")
    {
        JinjaLexer lexer{"{{ {'role': 'user', 'content': message.content} }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::Dict);

        const auto *dict = static_cast<const DictNode *>(output->expression.get());

        REQUIRE(dict->pairs.size() == 2);
    }

    SECTION("empty dictionary")
    {
        JinjaLexer lexer{"{{ {} }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *dict = static_cast<const DictNode *>(output->expression.get());

        REQUIRE(dict->pairs.empty());
    }

    SECTION("trailing comma")
    {
        JinjaLexer lexer{"{{ {'a': 1, 'b': 2,} }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
        const auto *dict = static_cast<const DictNode *>(output->expression.get());

        REQUIRE(dict->pairs.size() == 2);
    }
}

TEST_CASE("JinjaParser parses set statement", "[token][jinja][parser][usage][set]")
{
    JinjaLexer lexer{"{% set answer = 42 %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root->statements.size() == 1);
    REQUIRE(root->statements[0]->type == NodeType::Set);

    const auto *set = static_cast<const SetNode *>(root->statements[0].get());

    REQUIRE(set->name == "answer");
    REQUIRE(set->value->type == NodeType::Literal);
}

TEST_CASE("JinjaParser parses if statement", "[token][jinja][parser][usage][if]")
{
    JinjaLexer lexer{"{% if enabled %}" "YES" "{% endif %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root->statements.size() == 1);
    REQUIRE(root->statements[0]->type == NodeType::If);

    const auto *ifNode = static_cast<const IfNode *>(root->statements[0].get());

    REQUIRE(ifNode->branches.size() == 1);
    REQUIRE(ifNode->branches[0].condition != nullptr);
    REQUIRE(ifNode->branches[0].body != nullptr);
    REQUIRE(ifNode->branches[0].body->statements.size() == 1);
}

TEST_CASE("JinjaParser parses if elif else branches", "[token][jinja][parser][usage][if]")
{
    JinjaLexer lexer{
        "{% if role == 'system' %}"
        "SYSTEM"
        "{% elif role == 'user' %}"
        "USER"
        "{% else %}"
        "ASSISTANT"
        "{% endif %}"
    };

    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root->statements.size() == 1);
    REQUIRE(root->statements[0]->type == NodeType::If);

    const auto *ifNode = static_cast<const IfNode *>(root->statements[0].get());

    REQUIRE(ifNode->branches.size() == 3);
    REQUIRE(ifNode->branches[0].condition != nullptr);
    REQUIRE(ifNode->branches[1].condition != nullptr);
    REQUIRE(ifNode->branches[2].condition == nullptr);
    REQUIRE(ifNode->branches[0].body->statements.size() == 1);
    REQUIRE(ifNode->branches[1].body->statements.size() == 1);
    REQUIRE(ifNode->branches[2].body->statements.size() == 1);
}

TEST_CASE("JinjaParser parses for loop", "[token][jinja][parser][usage][for]")
{
    JinjaLexer lexer{"{% for message in messages %}" "{{ message.content }}" "{% endfor %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root->statements.size() == 1);
    REQUIRE(root->statements[0]->type == NodeType::For);

    const auto *forNode = static_cast<const ForNode *>(root->statements[0].get());

    REQUIRE(forNode->targets.size() == 1);
    REQUIRE(forNode->targets[0] == "message");
    REQUIRE(forNode->iterable != nullptr);
    REQUIRE(forNode->iterable->type == NodeType::Identifier);
    REQUIRE(forNode->body != nullptr);
    REQUIRE(forNode->body->statements.size() == 1);
    REQUIRE(forNode->elseBody == nullptr);
}

TEST_CASE("JinjaParser parses multiple for loop targets", "[token][jinja][parser][usage][for]")
{
    JinjaLexer lexer{"{% for key, value in items %}" "{{ key }}={{ value }}" "{% endfor %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *forNode = static_cast<const ForNode *>(root->statements[0].get());

    REQUIRE(forNode->targets.size() == 2);
    REQUIRE(forNode->targets[0] == "key");
    REQUIRE(forNode->targets[1] == "value");
}

TEST_CASE("JinjaParser parses for else block", "[token][jinja][parser][usage][for][else]")
{
    JinjaLexer lexer{"{% for message in messages %}" "{{ message.content }}" "{% else %}" "EMPTY" "{% endfor %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *forNode = static_cast<const ForNode *>(root->statements[0].get());

    REQUIRE(forNode->body != nullptr);
    REQUIRE(forNode->elseBody != nullptr);
    REQUIRE(forNode->elseBody->statements.size() == 1);

    const auto *text = static_cast<const TextNode *>(forNode->elseBody->statements[0].get());

    REQUIRE(text->text == "EMPTY");
}

TEST_CASE("JinjaParser parses nested control structures", "[token][jinja][parser][usage][nested]")
{
    JinjaLexer lexer{"{% for message in messages %}" "{% if message.role == 'user' %}" "{{ message.content }}" "{% endif %}" "{% endfor %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root->statements.size() == 1);

    const auto *forNode = static_cast<const ForNode *>(root->statements[0].get());

    REQUIRE(forNode->body != nullptr);
    REQUIRE(forNode->body->statements.size() == 1);
    REQUIRE(forNode->body->statements[0]->type == NodeType::If);
}

TEST_CASE("JinjaParser parses unary operators", "[token][jinja][parser][usage][operator][unary]")
{
    SECTION("not")
    {
        JinjaLexer lexer{"{{ not enabled }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::UnaryOp);

        const auto *unary = static_cast<const UnaryOpNode *>(output->expression.get());

        REQUIRE(unary->op == UnaryOp::Not);
    }

    SECTION("negative")
    {
        JinjaLexer lexer{"{{ -value }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::UnaryOp);

        const auto *unary = static_cast<const UnaryOpNode *>(output->expression.get());

        REQUIRE(unary->op == UnaryOp::Negate);
    }

    SECTION("positive")
    {
        JinjaLexer lexer{"{{ +value }}"};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::UnaryOp);

        const auto *unary = static_cast<const UnaryOpNode *>(output->expression.get());

        REQUIRE(unary->op == UnaryOp::Pos);
    }
}

TEST_CASE("JinjaParser gives multiplication precedence over addition", "[token][jinja][parser][usage][precedence]")
{
    JinjaLexer lexer{"{{ 1 + 2 * 3 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::BinaryOp);

    const auto *add = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(add->op == BinaryOp::Add);
    REQUIRE(add->right->type == NodeType::BinaryOp);

    const auto *multiply = static_cast<const BinaryOpNode *>(add->right.get());

    REQUIRE(multiply->op == BinaryOp::Mul);
}

TEST_CASE("JinjaParser gives power precedence over multiplication", "[token][jinja][parser][usage][precedence][power]")
{
    JinjaLexer lexer{"{{ 2 * 3 ** 4 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
    const auto *multiply = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(multiply->op == BinaryOp::Mul);
    REQUIRE(multiply->right->type == NodeType::BinaryOp);

    const auto *power = static_cast<const BinaryOpNode *>(multiply->right.get());

    REQUIRE(power->op == BinaryOp::Pow);
}

TEST_CASE("JinjaParser makes power right associative", "[token][jinja][parser][usage][precedence][power]")
{
    JinjaLexer lexer{"{{ 2 ** 3 ** 4 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
    const auto *outer = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(outer->op == BinaryOp::Pow);
    REQUIRE(outer->right->type == NodeType::BinaryOp);

    const auto *inner = static_cast<const BinaryOpNode *>(outer->right.get());

    REQUIRE(inner->op == BinaryOp::Pow);
}

TEST_CASE("JinjaParser parses arithmetic operators", "[token][jinja][parser][usage][operator]")
{
    struct Case
    {
        std::string_view source;
        BinaryOp operation;
    };

    static constexpr Case Cases[] = {
        {"{{ a + b }}",  BinaryOp::Add},
        {"{{ a - b }}",  BinaryOp::Sub},
        {"{{ a * b }}",  BinaryOp::Mul},
        {"{{ a / b }}",  BinaryOp::Div},
        {"{{ a // b }}", BinaryOp::FloorDiv},
        {"{{ a % b }}",  BinaryOp::Mod},
        {"{{ a ** b }}", BinaryOp::Pow},
        {"{{ a ~ b }}",  BinaryOp::Concat}
    };

    for (const Case &test : Cases) {
        JinjaLexer lexer{test.source};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::BinaryOp);

        const auto *binary = static_cast<const BinaryOpNode *>(output->expression.get());

        REQUIRE(binary->op == test.operation);
    }
}

TEST_CASE("JinjaParser parses comparison and boolean operators", "[token][jinja][parser][usage][operator][comparison]")
{
    struct Case
    {
        std::string_view source;
        BinaryOp operation;
    };

    static constexpr Case Cases[] = {
        {"{{ a == b }}",  BinaryOp::Eq},
        {"{{ a != b }}",  BinaryOp::Ne},
        {"{{ a < b }}",   BinaryOp::Lt},
        {"{{ a <= b }}",  BinaryOp::Le},
        {"{{ a > b }}",   BinaryOp::Gt},
        {"{{ a >= b }}",  BinaryOp::Ge},
        {"{{ a and b }}", BinaryOp::And},
        {"{{ a or b }}",  BinaryOp::Or},
        {"{{ a in b }}",  BinaryOp::In}
    };

    for (const Case &test : Cases) {
        JinjaLexer lexer{test.source};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::BinaryOp);

        const auto *binary = static_cast<const BinaryOpNode *>(output->expression.get());

        REQUIRE(binary->op == test.operation);
    }
}

TEST_CASE("JinjaParser parses compound not in operator", "[token][jinja][parser][usage][operator][comparison]")
{
    JinjaLexer lexer{"{{ value not in values }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::BinaryOp);

    const auto *binary = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(binary->op == BinaryOp::NotIn);
}

TEST_CASE("JinjaParser parses is test name", "[token][jinja][parser][usage][operator][test]")
{
    JinjaLexer lexer{"{{ value is defined }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::BinaryOp);

    const auto *binary = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(binary->op == BinaryOp::Is);
    REQUIRE(binary->right->type == NodeType::Literal);

    const auto *testName = static_cast<const LiteralNode *>(binary->right.get());

    REQUIRE(std::holds_alternative<std::string>(testName->value));
    REQUIRE(std::get<std::string>(testName->value) == "defined");
}

TEST_CASE("JinjaParser parses compound is not test", "[token][jinja][parser][usage][operator][test]")
{
    JinjaLexer lexer{"{{ value is not none }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::BinaryOp);

    const auto *binary = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(binary->op == BinaryOp::IsNot);
    REQUIRE(binary->right->type == NodeType::Literal);

    const auto *testName = static_cast<const LiteralNode *>(binary->right.get());

    REQUIRE(std::holds_alternative<std::string>(testName->value));
    REQUIRE(std::get<std::string>(testName->value) == "none");
}

TEST_CASE("JinjaParser preserves is test names as string literals", "[token][jinja][parser][usage][operator][test]")
{
    static constexpr std::string_view Names[] = {
        "defined", "none", "even", "odd", "string", "number", "true", "false"
    };

    for (const std::string_view name : Names) {
        const std::string source = "{{ value is " + std::string{name} + " }}";

        JinjaLexer lexer{source};
        const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
        JinjaParser parser{tokens};
        std::unique_ptr<BodyNode> root = parser.parse();

        const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

        REQUIRE(output->expression->type == NodeType::BinaryOp);

        const auto *binary = static_cast<const BinaryOpNode *>(output->expression.get());

        REQUIRE(binary->op == BinaryOp::Is);
        REQUIRE(binary->right->type == NodeType::Literal);

        const auto *testName = static_cast<const LiteralNode *>(binary->right.get());

        REQUIRE(std::holds_alternative<std::string>(testName->value));
        REQUIRE(std::get<std::string>(testName->value) == name);
    }
}

TEST_CASE("JinjaParser parses conditional expression", "[token][jinja][parser][usage][conditional]")
{
    JinjaLexer lexer{"{{ 'yes' if enabled else 'no' }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Conditional);

    const auto *conditional = static_cast<const ConditionalNode *>(output->expression.get());

    REQUIRE(conditional->condition != nullptr);
    REQUIRE(conditional->trueValue != nullptr);
    REQUIRE(conditional->falseValue != nullptr);
    REQUIRE(conditional->condition->type == NodeType::Identifier);
    REQUIRE(conditional->trueValue->type == NodeType::Literal);
    REQUIRE(conditional->falseValue->type == NodeType::Literal);
}

TEST_CASE("JinjaParser allows conditional expression without else", "[token][jinja][parser][usage][conditional]")
{
    JinjaLexer lexer{"{{ value if enabled }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Conditional);

    const auto *conditional = static_cast<const ConditionalNode *>(output->expression.get());

    REQUIRE(conditional->condition != nullptr);
    REQUIRE(conditional->trueValue != nullptr);
    REQUIRE(conditional->falseValue == nullptr);
}

TEST_CASE("JinjaParser respects parenthesized expression", "[token][jinja][parser][usage][precedence]")
{
    JinjaLexer lexer{"{{ (1 + 2) * 3 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());
    const auto *multiply = static_cast<const BinaryOpNode *>(output->expression.get());

    REQUIRE(multiply->op == BinaryOp::Mul);
    REQUIRE(multiply->left->type == NodeType::BinaryOp);

    const auto *add = static_cast<const BinaryOpNode *>(multiply->left.get());

    REQUIRE(add->op == BinaryOp::Add);
}

TEST_CASE("JinjaParser supports chained postfix expressions", "[token][jinja][parser][usage][postfix]")
{
    JinjaLexer lexer{"{{ messages[0].content | trim }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    const auto *output = static_cast<const OutputNode *>(root->statements[0].get());

    REQUIRE(output->expression->type == NodeType::Filter);

    const auto *filter = static_cast<const FilterNode *>(output->expression.get());

    REQUIRE(filter->filterName == "trim");
    REQUIRE(filter->target->type == NodeType::MemberAccess);

    const auto *member = static_cast<const MemberAccessNode *>(filter->target.get());

    REQUIRE(member->member == "content");
    REQUIRE(member->object->type == NodeType::Subscript);
}

TEST_CASE("JinjaParser records source positions", "[token][jinja][parser][usage][position]")
{
    JinjaLexer lexer{"first\n" "{{ value }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root->statements.size() == 2);
    REQUIRE(root->statements[0]->line == 1);
    REQUIRE(root->statements[0]->column == 1);
    REQUIRE(root->statements[1]->line == 2);
    REQUIRE(root->statements[1]->column == 1);

    const auto *output = static_cast<const OutputNode *>(root->statements[1].get());

    REQUIRE(output->expression->line == 2);
    REQUIRE(output->expression->column == 4);
}

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("JinjaParser accepts empty token span", "[token][jinja][parser][edge]")
{
    const std::vector<JinjaToken> tokens;
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->type == NodeType::Body);
    REQUIRE(root->statements.empty());
}

TEST_CASE("JinjaParser accepts lexer EOF only stream", "[token][jinja][parser][edge]")
{
    JinjaLexer lexer{""};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    std::unique_ptr<BodyNode> root = parser.parse();

    REQUIRE(root != nullptr);
    REQUIRE(root->statements.empty());
}

TEST_CASE("JinjaParser rejects unclosed output expression", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ value"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects unclosed if statement", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{% if condition %}" "still waiting for endif"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects unclosed for statement", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{% for value in values %}" "{{ value }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects unexpected control block", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{% endif %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects invalid lexer token", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ @ }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects missing expression", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects missing member name", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ value. }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects missing subscript close", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ value[0 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects missing function call close", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ function(1, 2 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects missing filter name", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ value | }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects malformed list", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ [1, 2 }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects dictionary without colon", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{{ {'a' 1} }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects for loop without target", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{% for in values %}" "{% endfor %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects for target list ending with comma", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{% for key, in values %}" "{% endfor %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects set without assignment", "[token][jinja][parser][edge][error]")
{
    JinjaLexer lexer{"{% set answer 42 %}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser ParseError retains source position", "[token][jinja][parser][edge][error][position]")
{
    JinjaLexer lexer{"first\n" "{{ value. }}"};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};

    try {
        (void)parser.parse();
        FAIL("Expected JinjaParser to throw ParseError");
    } catch (const ParseError &error) {
        REQUIRE(error.line() == 2);
        REQUIRE(error.column() > 1);
        REQUIRE_FALSE(std::string{error.what()}.empty());
    }
}

TEST_CASE("JinjaParser rejects invalid integer literal token", "[token][jinja][parser][edge][literal]")
{
    const std::vector<JinjaToken> tokens = {
        {JinjaType::ExprBegin,    "{{",                                     1, 1},
        {JinjaType::NumberLiteral, "999999999999999999999999999999999999", 1, 4},
        {JinjaType::ExprEnd,      "}}",                                     1, 40},
        {JinjaType::Eof,          {},                                       1, 42}
    };

    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

TEST_CASE("JinjaParser rejects invalid floating point literal token", "[token][jinja][parser][edge][literal]")
{
    //
    // This token cannot normally be emitted by JinjaLexer.
    //
    // The test intentionally verifies that the parser still validates
    // the complete numeric token rather than silently accepting a
    // valid prefix such as "1.2" from "1.2.3".
    //
    const std::vector<JinjaToken> tokens = {
        {JinjaType::ExprBegin,     "{{",  1, 1},
        {JinjaType::NumberLiteral, "1.2.3", 1, 4},
        {JinjaType::ExprEnd,       "}}",  1, 9},
        {JinjaType::Eof,           {},    1, 11}
    };

    JinjaParser parser{tokens};

    REQUIRE_THROWS_AS(parser.parse(), ParseError);
}

//
// Block 3: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JinjaParser chat template AST construction", "[token][jinja][parser][benchmark]")
{
    static constexpr std::string_view Source =
        "{% for message in messages %}"
        "{% if message.role == 'system' %}"
        "[SYSTEM]{{ message.content }}[/SYSTEM]"
        "{% elif message.role == 'user' %}"
        "[USER]{{ message.content | trim }}[/USER]"
        "{% else %}"
        "[ASSISTANT]{{ message.content }}[/ASSISTANT]"
        "{% endif %}"
        "{% endfor %}"
        "{% if add_generation_prompt %}"
        "[ASSISTANT]"
        "{% endif %}";

    JinjaLexer lexer{Source};

    //
    // The parser borrows this storage.
    //
    // Keep it outside the benchmark both for lifetime correctness and
    // so this benchmark measures AST construction rather than lexing.
    //
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    BENCHMARK("Parse chat template token stream into AST")
    {
        JinjaParser parser{tokens};
        return parser.parse();
    };
}

#endif