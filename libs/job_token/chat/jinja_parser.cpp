#include "jinja_parser.h"

#include <charconv>
#include <string>
#include <system_error>

namespace job::token {

JinjaParser::JinjaParser(std::span<const JinjaToken> tokens) noexcept
    : m_tokens{tokens}
{
}

std::unique_ptr<BodyNode> JinjaParser::parse()
{
    return parseBlock();
}

bool JinjaParser::checkBlockKeyword(JinjaType keyword) const noexcept
{
    return check(JinjaType::BlockBegin) && checkNext(keyword);
}

std::unique_ptr<BodyNode> JinjaParser::parseBlock(std::initializer_list<JinjaType> stopTokens)
{
    auto body = std::make_unique<BodyNode>(peek().line, peek().column);

    while (!isAtEnd()) {
        for (const JinjaType stop : stopTokens)
            if (checkBlockKeyword(stop)) return body;

        auto statement = parseStatement();
        if (statement) body->statements.push_back(std::move(statement));
    }

    return body;
}

std::unique_ptr<StmtNode> JinjaParser::parseStatement()
{
    if (match(JinjaType::Text))
        return std::make_unique<TextNode>(std::string{previous().text}, previous().line, previous().column);

    if (match(JinjaType::ExprBegin))
        return parseOutputStatement();

    if (checkBlockKeyword(JinjaType::KwIf)) {
        advance(); // BlockBegin
        advance(); // KwIf
        return parseIfStatement();
    }

    if (checkBlockKeyword(JinjaType::KwFor)) {
        advance(); // BlockBegin
        advance(); // KwFor
        return parseForStatement();
    }

    if (checkBlockKeyword(JinjaType::KwSet)) {
        advance(); // BlockBegin
        advance(); // KwSet
        return parseSetStatement();
    }

    if (check(JinjaType::BlockBegin))
        throw ParseError{"Unexpected template control block", peek().line, peek().column};

    if (check(JinjaType::Unknown))
        throw ParseError{"Invalid token", peek().line, peek().column};

    throw ParseError{"Unexpected token while parsing template", peek().line, peek().column};
}

std::unique_ptr<OutputNode> JinjaParser::parseOutputStatement()
{
    const std::size_t line = previous().line;
    const std::size_t column = previous().column;

    auto expression = parseExpression();
    consume(JinjaType::ExprEnd, "Expected '}}' or '-}}' after expression");

    return std::make_unique<OutputNode>(std::move(expression), line, column);
}

std::unique_ptr<IfNode> JinjaParser::parseIfStatement()
{
    const std::size_t line = previous().line;
    const std::size_t column = previous().column;

    auto node = std::make_unique<IfNode>(line, column);

    //
    // ------------------------------------------------------------
    // Initial if branch
    // ------------------------------------------------------------
    //
    auto condition = parseExpression();
    consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after if condition");
    auto body = parseBlock({JinjaType::KwElif, JinjaType::KwElse, JinjaType::KwEndIf});
    node->branches.push_back({std::move(condition), std::move(body)});

    //
    // ------------------------------------------------------------
    // elif
    // ------------------------------------------------------------
    //
    while (checkBlockKeyword(JinjaType::KwElif)) {
        consume(JinjaType::BlockBegin, "Expected '{%' before 'elif'");
        consume(JinjaType::KwElif, "Expected 'elif'");

        auto elifCondition = parseExpression();
        consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after elif condition");
        auto elifBody = parseBlock({JinjaType::KwElif, JinjaType::KwElse, JinjaType::KwEndIf});

        node->branches.push_back({std::move(elifCondition), std::move(elifBody)});
    }

    //
    // ------------------------------------------------------------
    // else
    // ------------------------------------------------------------
    //
    if (checkBlockKeyword(JinjaType::KwElse)) {
        consume(JinjaType::BlockBegin, "Expected '{%' before 'else'");
        consume(JinjaType::KwElse, "Expected 'else'");
        consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after else");

        auto elseBody = parseBlock({JinjaType::KwEndIf});
        node->branches.push_back({nullptr, std::move(elseBody)});
    }

    consume(JinjaType::BlockBegin, "Expected '{%' before 'endif'");
    consume(JinjaType::KwEndIf, "Expected 'endif' to close if block");
    consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after endif");

    return node;
}

std::unique_ptr<ForNode> JinjaParser::parseForStatement()
{
    const std::size_t line = previous().line;
    const std::size_t column = previous().column;

    auto node = std::make_unique<ForNode>(line, column);

    //
    // ------------------------------------------------------------
    // Loop targets
    //
    //     for item in items
    //     for key, value in items
    //     for a, b, c in items
    // ------------------------------------------------------------
    //
    const JinjaToken &firstTarget = consume(JinjaType::Identifier, "Expected loop variable name");
    node->targets.emplace_back(firstTarget.text);

    while (match(JinjaType::Comma)) {
        const JinjaToken &target = consume(JinjaType::Identifier, "Expected loop variable name after ','");
        node->targets.emplace_back(target.text);
    }

    consume(JinjaType::KwIn, "Expected 'in' keyword in for loop");
    node->iterable = parseExpression();
    consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after for loop header");
    node->body = parseBlock({JinjaType::KwElse, JinjaType::KwEndFor});

    //
    // else what bro ? :P
    //
    if (checkBlockKeyword(JinjaType::KwElse)) {
        consume(JinjaType::BlockBegin, "Expected '{%' before 'else'");
        consume(JinjaType::KwElse, "Expected 'else'");
        consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after for-else");
        node->elseBody = parseBlock({JinjaType::KwEndFor});
    }

    consume(JinjaType::BlockBegin, "Expected '{%' before 'endfor'");
    consume(JinjaType::KwEndFor, "Expected 'endfor' to close loop");
    consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after endfor");

    return node;
}

std::unique_ptr<SetNode> JinjaParser::parseSetStatement()
{
    const std::size_t line = previous().line;
    const std::size_t column = previous().column;

    const JinjaToken &identifier = consume(JinjaType::Identifier, "Expected identifier after 'set'");
    consume(JinjaType::Assign, "Expected '=' in set statement");

    auto expression = parseExpression();
    consume(JinjaType::BlockEnd, "Expected '%}' or '-%}' after set statement");

    return std::make_unique<SetNode>(std::string{identifier.text}, std::move(expression), line, column);
}

std::unique_ptr<ExprNode> JinjaParser::parseExpression(int minPrecedence)
{
    auto left = parsePrimary();
    left = parsePostfix(std::move(left));

    while (true) {
        //
        // ------------------------------------------------------------
        // Compound comparison operators
        //
        //     x not in y
        //     x is not none
        // ------------------------------------------------------------
        //
        BinaryOp operation{};
        int precedence = 0;
        bool compound = false;

        if (check(JinjaType::KwNot) && checkNext(JinjaType::KwIn)) {
            operation = BinaryOp::NotIn;
            precedence = binaryPrecedence(JinjaType::KwIn);
            compound = true;
        } else if (check(JinjaType::KwIs) && checkNext(JinjaType::KwNot)) {
            operation = BinaryOp::IsNot;
            precedence = binaryPrecedence(JinjaType::KwIs);
            compound = true;
        } else {
            precedence = binaryPrecedence(peek().type);
        }

        if (precedence <= minPrecedence)
            break;

        const std::size_t line = left->line;
        const std::size_t column = left->column;

        if (compound) { advance(); advance(); }
        else           operation = binaryOp(advance().type);

        std::unique_ptr<ExprNode> right;

        // "is"/"is not" test names (defined, none, even, odd, string,
        // number, ...) must survive as their literal name -- never
        // evaluated as a variable lookup (the bare-identifier bug) and
        // never coerced into a keyword's own literal value either (e.g.
        // bare 'none' would otherwise parse as the actual None literal,
        // not the string "none" JinjaVM::visit(BinaryOpNode) compares
        // test names against). See parseTestName().
        if (operation == BinaryOp::Is || operation == BinaryOp::IsNot)
            right = parseTestName();

        if (!right) {
            //
            // Pow is right-associative:
            //
            //     2 ** 3 ** 2
            //     2 ** (3 ** 2)
            //
            const int rightPrecedence = operation == BinaryOp::Pow ? precedence - 1 : precedence;
            right = parseExpression(rightPrecedence);
        }

        left = std::make_unique<BinaryOpNode>(operation, std::move(left), std::move(right), line, column);
        left = parsePostfix(std::move(left));
    }

    if (minPrecedence == 0)
        left = parseConditional(std::move(left));

    return left;
}

std::unique_ptr<ExprNode> JinjaParser::parseTestName()
{
    // Only intercept the token shapes that collide with a recognized
    // test name (defined/none/even/odd/string/number, plus true/false
    // for completeness) -- anything else after "is"/"is not" falls back
    // to the general expression parser in parseExpression() unchanged,
    // so this never narrows what "is"/"is not" accepts, only fixes how
    // these specific tokens are represented.
    const std::size_t line = peek().line;
    const std::size_t column = peek().column;

    if (check(JinjaType::Identifier) || check(JinjaType::KwNone) ||
        check(JinjaType::KwTrue)     || check(JinjaType::KwFalse)) {
        const JinjaToken &name = advance();
        return std::make_unique<LiteralNode>(std::string{name.text}, line, column);
    }

    return nullptr;
}

std::unique_ptr<ExprNode> JinjaParser::parseConditional(std::unique_ptr<ExprNode> expression)
{
    if (!match(JinjaType::KwIf))
        return expression;

    const std::size_t line = expression->line;
    const std::size_t column = expression->column;

    auto condition = parseExpression();
    std::unique_ptr<ExprNode> falseValue;

    if (match(JinjaType::KwElse))
        falseValue = parseExpression();

    return std::make_unique<ConditionalNode>(std::move(condition), std::move(expression), std::move(falseValue), line, column);
}

std::unique_ptr<ExprNode> JinjaParser::parsePrimary()
{
    const std::size_t line = peek().line;
    const std::size_t column = peek().column;

    //
    // ------------------------------------------------------------
    // Numbers
    // ------------------------------------------------------------
    //
    if (match(JinjaType::NumberLiteral)) {
        const std::string_view value = previous().text;

        if (value.find('.') != std::string_view::npos) {
            try {
                const std::string text{value};
                std::size_t consumed = 0;
                const double number = std::stod(text, &consumed);

                if (consumed != text.size())
                    throw ParseError{ "Invalid floating-point literal", line, column };

                return std::make_unique<LiteralNode>(number, line, column);
            } catch (const ParseError &) {
                throw;
            } catch (const std::exception &) {
                throw ParseError{ "Invalid floating-point literal", line, column };
            }
        }

        std::int64_t number = 0;
        const auto result = std::from_chars(value.data(),
                                            value.data() + value.size(),
                                            number);

        if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            throw ParseError{ "Invalid integer literal", line, column };

        return std::make_unique<LiteralNode>(number, line, column);
    }

    //
    // ------------------------------------------------------------
    // Strings
    //
    // The lexer already strips the surrounding quotes. The token text is
    // the string body.
    // ------------------------------------------------------------
    //
    if (match(JinjaType::StringLiteral))
        return std::make_unique<LiteralNode>(decodeStringLiteral( previous().text), line, column);

    if (match(JinjaType::KwTrue))
        return std::make_unique<LiteralNode>(true, line, column);

    if (match(JinjaType::KwFalse))
        return std::make_unique<LiteralNode>(false, line, column);

    if (match(JinjaType::KwNone))
        return std::make_unique<LiteralNode>(nullptr, line, column);

    if (match(JinjaType::Identifier))
        return std::make_unique<IdentifierNode>(std::string{previous().text}, line, column);

    //
    // ------------------------------------------------------------
    // Parenthesized expression
    // ------------------------------------------------------------
    //
    if (match(JinjaType::ParenOpen)) {
        auto expression = parseExpression();
        consume(JinjaType::ParenClose, "Expected ')' after parenthesized expression");
        return expression;
    }

    //
    // ------------------------------------------------------------
    // List
    // ------------------------------------------------------------
    //
    if (match(JinjaType::BracketOpen)) {
        auto list = std::make_unique<ListNode>(line, column);

        while (!check(JinjaType::BracketClose)) {
            list->elements.push_back(parseExpression());

            if (!match(JinjaType::Comma))
                break;

            // Trailing comma.
            if (check(JinjaType::BracketClose))
                break;
        }

        consume(JinjaType::BracketClose, "Expected ']' after list elements");
        return list;
    }

    //
    // ------------------------------------------------------------
    // Dictionary
    // ------------------------------------------------------------
    //
    if (match(JinjaType::BraceOpen)) {
        auto dict = std::make_unique<DictNode>(line, column);

        while (!check(JinjaType::BraceClose)) {
            auto key = parseExpression();
            consume(JinjaType::Colon, "Expected ':' between dict key and value");
            auto value = parseExpression();
            dict->pairs.emplace_back(std::move(key), std::move(value));

            if (!match(JinjaType::Comma))
                break;

            // Trailing comma.
            if (check(JinjaType::BraceClose))
                break;
        }

        consume(JinjaType::BraceClose, "Expected '}' after dict entries");
        return dict;
    }

    //
    // ------------------------------------------------------------
    // Unary not
    // ------------------------------------------------------------
    //
    if (match(JinjaType::KwNot)) {
        auto operand = parseExpression(25);
        return std::make_unique<UnaryOpNode>(UnaryOp::Not, std::move(operand), line, column);
    }

    //
    // ------------------------------------------------------------
    // Unary + / -
    // ------------------------------------------------------------
    //
    if (match(JinjaType::Minus)) {
        auto operand = parseExpression(65);
        return std::make_unique<UnaryOpNode>(UnaryOp::Negate, std::move(operand), line, column);
    }

    if (match(JinjaType::Plus)) {
        auto operand = parseExpression(65);
        return std::make_unique<UnaryOpNode>(UnaryOp::Pos, std::move(operand), line, column);
    }

    if (check(JinjaType::Unknown))
        throw ParseError{"Invalid token in expression", line, column};

    throw ParseError{"Expected expression", line, column};
}

std::unique_ptr<ExprNode> JinjaParser::parsePostfix(std::unique_ptr<ExprNode> expression)
{
    while (true) {
        //
        // ------------------------------------------------------------
        // Member access
        // ------------------------------------------------------------
        //
        if (match(JinjaType::Dot)) {
            const JinjaToken &identifier = consume(JinjaType::Identifier, "Expected property name after '.'");
            const std::size_t line = expression->line;
            const std::size_t column = expression->column;

            expression = std::make_unique<MemberAccessNode>(std::move(expression), std::string{identifier.text}, line, column);
            continue;
        }

        //
        // ------------------------------------------------------------
        // Subscript
        // ------------------------------------------------------------
        //
        if (match(JinjaType::BracketOpen)) {
            const std::size_t line = expression->line;
            const std::size_t column = expression->column;

            auto index = parseExpression();
            consume(JinjaType::BracketClose, "Expected ']' after subscript index");

            expression = std::make_unique<SubscriptNode>(std::move(expression), std::move(index), line, column);
            continue;
        }

        //
        // ------------------------------------------------------------
        // Call
        // ------------------------------------------------------------
        //
        if (match(JinjaType::ParenOpen)) {
            const std::size_t line = expression->line;
            const std::size_t column = expression->column;

            auto call = std::make_unique<CallNode>(std::move(expression), line, column);

            while (!check(JinjaType::ParenClose)) {
                if (check(JinjaType::Identifier) && checkNext(JinjaType::Assign)) {
                    std::string name{advance().text};
                    consume(JinjaType::Assign, "Expected '=' after keyword argument name");
                    call->kwArgs.emplace_back(std::move(name), parseExpression());
                } else {
                    call->posArgs.push_back(parseExpression());
                }

                if (!match(JinjaType::Comma))
                    break;

                // Trailing comma.
                if (check(JinjaType::ParenClose))
                    break;
            }

            consume(JinjaType::ParenClose, "Expected ')' after function arguments");
            expression = std::move(call);
            continue;
        }

        //
        // ------------------------------------------------------------
        // Filter
        // ------------------------------------------------------------
        //
        if (match(JinjaType::Pipe)) {
            expression = parseFilter(std::move(expression));
            continue;
        }

        break;
    }

    return expression;
}

std::unique_ptr<ExprNode> JinjaParser::parseFilter(std::unique_ptr<ExprNode> target)
{
    const JinjaToken &name = consume(JinjaType::Identifier, "Expected filter name after '|'");
    const std::size_t line = target->line;
    const std::size_t column = target->column;

    auto filter = std::make_unique<FilterNode>(std::move(target), std::string{name.text}, line, column);

    if (!match(JinjaType::ParenOpen))
        return filter;

    while (!check(JinjaType::ParenClose)) {
        if (check(JinjaType::Identifier) && checkNext(JinjaType::Assign)) {
            std::string argumentName{advance().text};
            consume(JinjaType::Assign, "Expected '=' after filter keyword argument name");
            filter->kwArgs.emplace_back(std::move(argumentName), parseExpression());
        } else {
            filter->args.push_back(parseExpression());
        }

        if (!match(JinjaType::Comma))
            break;

        // Trailing comma.
        if (check(JinjaType::ParenClose))
            break;
    }

    consume(JinjaType::ParenClose, "Expected ')' after filter arguments");
    return filter;
}

int JinjaParser::binaryPrecedence(JinjaType type) const noexcept
{
    switch (type) {
    case JinjaType::KwOr:  return 10;
    case JinjaType::KwAnd: return 20;

    case JinjaType::KwIn:
    case JinjaType::KwIs:
    case JinjaType::Eq:
    case JinjaType::Ne:
    case JinjaType::Lt:
    case JinjaType::Le:
    case JinjaType::Gt:
    case JinjaType::Ge:
        return 30;

    case JinjaType::Tilde: return 40;

    case JinjaType::Plus:
    case JinjaType::Minus:
        return 50;

    case JinjaType::Mul:
    case JinjaType::Div:
    case JinjaType::FloorDiv:
    case JinjaType::Mod:
        return 60;

    case JinjaType::Pow: return 70;

    default: return 0;
    }
}

BinaryOp JinjaParser::binaryOp(JinjaType type) const
{
    switch (type) {
    case JinjaType::Plus:     return BinaryOp::Add;
    case JinjaType::Minus:    return BinaryOp::Sub;
    case JinjaType::Mul:      return BinaryOp::Mul;
    case JinjaType::Div:      return BinaryOp::Div;
    case JinjaType::FloorDiv: return BinaryOp::FloorDiv;
    case JinjaType::Mod:      return BinaryOp::Mod;
    case JinjaType::Pow:      return BinaryOp::Pow;
    case JinjaType::Tilde:    return BinaryOp::Concat;
    case JinjaType::Eq:       return BinaryOp::Eq;
    case JinjaType::Ne:       return BinaryOp::Ne;
    case JinjaType::Lt:       return BinaryOp::Lt;
    case JinjaType::Le:       return BinaryOp::Le;
    case JinjaType::Gt:       return BinaryOp::Gt;
    case JinjaType::Ge:       return BinaryOp::Ge;
    case JinjaType::KwAnd:    return BinaryOp::And;
    case JinjaType::KwOr:     return BinaryOp::Or;
    case JinjaType::KwIn:     return BinaryOp::In;
    case JinjaType::KwIs:     return BinaryOp::Is;
    default: throw std::logic_error{"Invalid binary operator token"};
    }
}

const JinjaToken &JinjaParser::peek() const noexcept
{
    return isAtEnd() ? m_eof : m_tokens[m_current];
}

const JinjaToken &JinjaParser::peekNext() const noexcept
{
    return m_current + 1 >= m_tokens.size() ? m_eof : m_tokens[m_current + 1];
}

const JinjaToken &JinjaParser::previous() const noexcept
{
    return m_current == 0 ? m_eof : m_tokens[m_current - 1];
}

bool JinjaParser::isAtEnd() const noexcept
{
    return m_current >= m_tokens.size() || m_tokens[m_current].type == JinjaType::Eof;
}

const JinjaToken &JinjaParser::advance() noexcept
{
    if (!isAtEnd()) ++m_current;
    return previous();
}

bool JinjaParser::check(JinjaType type) const noexcept
{
    return isAtEnd() ? type == JinjaType::Eof : peek().type == type;
}

bool JinjaParser::checkNext(JinjaType type) const noexcept
{
    return peekNext().type == type;
}

bool JinjaParser::match(JinjaType type) noexcept
{
    if (!check(type))
        return false;
    (void)advance();
    return true;
}

const JinjaToken &JinjaParser::consume(JinjaType type, std::string_view errorMessage)
{
    if (check(type))
        return advance();
    throw ParseError{errorMessage, peek().line, peek().column};
}


std::string JinjaParser::decodeStringLiteral(std::string_view value)
{
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char character = value[i];

        if (character != '\\' ||
            i + 1 >= value.size()) {
            result.push_back(character);
            continue;
        }

        const char escaped =
            value[++i];

        switch (escaped) {
        case 'n':
            result.push_back('\n');
            break;

        case 'r':
            result.push_back('\r');
            break;

        case 't':
            result.push_back('\t');
            break;

        case '\\':
            result.push_back('\\');
            break;

        case '\'':
            result.push_back('\'');
            break;

        case '"':
            result.push_back('"');
            break;

        default:
            // Preserve unknown escapes rather than silently destroying them.
            result.push_back('\\');
            result.push_back(escaped);
            break;
        }
    }

    return result;
}

} // namespace job::token