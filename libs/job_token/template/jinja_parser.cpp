#include "template/jinja_parser.h"

#include <charconv>
#include <string>

namespace job::token {

const JinjaToken JinjaParser::eof_token_{JinjaTokenType::Eof, "", 0, 0};

JinjaParser::JinjaParser(std::span<const JinjaToken> tokens) noexcept
    : tokens_(tokens) {}

JinjaParser::JinjaParser(const std::vector<JinjaToken>& tokens) noexcept
    : tokens_(tokens.data(), tokens.size()) {}

std::unique_ptr<ast::BlockNode> JinjaParser::parse() {
    return parseBlock();
}

bool JinjaParser::checkBlockKeyword(JinjaTokenType kw) const noexcept {
    if (current_ + 1 < tokens_.size()) {
        return tokens_[current_].is(JinjaTokenType::BlockBegin) && tokens_[current_ + 1].is(kw);
    }
    return false;
}

std::unique_ptr<ast::BlockNode> JinjaParser::parseBlock(std::initializer_list<JinjaTokenType> stop_tokens) {
    auto block = std::make_unique<ast::BlockNode>(peek().line, peek().column);

    while (!isAtEnd()) {
        for (auto stop : stop_tokens) {
            if (checkBlockKeyword(stop)) {
                return block;
            }
        }

        if (auto stmt = parseStatement()) {
            block->statements.push_back(std::move(stmt));
        }
    }
    return block;
}

std::unique_ptr<ast::StmtNode> JinjaParser::parseStatement() {
    if (match(JinjaTokenType::Text)) {
        return std::make_unique<ast::TextNode>(
            std::string(previous().text), previous().line, previous().column);
    }

    if (match(JinjaTokenType::ExprBegin)) {
        return parseOutputStatement();
    }

    if (checkBlockKeyword(JinjaTokenType::KwIf)) {
        advance(); // consume BlockBegin
        advance(); // consume KwIf
        return parseIfStatement();
    }

    if (checkBlockKeyword(JinjaTokenType::KwFor)) {
        advance(); // consume BlockBegin
        advance(); // consume KwFor
        return parseForStatement();
    }

    if (checkBlockKeyword(JinjaTokenType::KwSet)) {
        advance(); // consume BlockBegin
        advance(); // consume KwSet
        return parseSetStatement();
    }

    if (peek().is(JinjaTokenType::BlockBegin)) {
        throw ParseError("Unexpected template control block", peek().line, peek().column);
    }

    advance();
    return nullptr;
}

std::unique_ptr<ast::OutputNode> JinjaParser::parseOutputStatement() {
    size_t line = previous().line;
    size_t col = previous().column;

    auto expr = parseExpression();
    consume(JinjaTokenType::ExprEnd, "Expected '}}' or '-}}' after expression");
    return std::make_unique<ast::OutputNode>(std::move(expr), line, col);
}

std::unique_ptr<ast::IfNode> JinjaParser::parseIfStatement() {
    auto if_node = std::make_unique<ast::IfNode>(previous().line, previous().column);

    // Initial if branch condition
    auto condition = parseExpression();
    consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after if condition");
    auto body = parseBlock({JinjaTokenType::KwElif, JinjaTokenType::KwElse, JinjaTokenType::KwEndIf});
    if_node->branches.push_back({std::move(condition), std::move(body)});

    // Elif branches
    while (checkBlockKeyword(JinjaTokenType::KwElif)) {
        consume(JinjaTokenType::BlockBegin, "Expected '{%' before 'elif'");
        consume(JinjaTokenType::KwElif, "Expected 'elif'");
        auto elif_cond = parseExpression();
        consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after elif condition");
        auto elif_body = parseBlock({JinjaTokenType::KwElif, JinjaTokenType::KwElse, JinjaTokenType::KwEndIf});
        if_node->branches.push_back({std::move(elif_cond), std::move(elif_body)});
    }

    // Optional Else branch
    if (checkBlockKeyword(JinjaTokenType::KwElse)) {
        consume(JinjaTokenType::BlockBegin, "Expected '{%' before 'else'");
        consume(JinjaTokenType::KwElse, "Expected 'else'");
        consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after else");
        auto else_body = parseBlock({JinjaTokenType::KwEndIf});
        if_node->branches.push_back({nullptr, std::move(else_body)});
    }

    consume(JinjaTokenType::BlockBegin, "Expected '{%' before 'endif'");
    consume(JinjaTokenType::KwEndIf, "Expected 'endif' to close if block");
    consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after endif");
    return if_node;
}

std::unique_ptr<ast::ForNode> JinjaParser::parseForStatement() {
    auto for_node = std::make_unique<ast::ForNode>(previous().line, previous().column);

    const auto& id_tok = consume(JinjaTokenType::Identifier, "Expected loop variable name");
    for_node->loop_var = std::string(id_tok.text);

    if (match(JinjaTokenType::Comma)) {
        const auto& val_tok = consume(JinjaTokenType::Identifier, "Expected second variable name in key-value loop");
        for_node->value_var = std::string(val_tok.text);
    }

    consume(JinjaTokenType::KwIn, "Expected 'in' keyword in for loop");
    for_node->iterable = parseExpression();
    consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after for loop header");

    for_node->body = parseBlock({JinjaTokenType::KwElse, JinjaTokenType::KwEndFor});

    if (checkBlockKeyword(JinjaTokenType::KwElse)) {
        consume(JinjaTokenType::BlockBegin, "Expected '{%' before 'else'");
        consume(JinjaTokenType::KwElse, "Expected 'else'");
        consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after for-else");
        for_node->else_body = parseBlock({JinjaTokenType::KwEndFor});
    }

    consume(JinjaTokenType::BlockBegin, "Expected '{%' before 'endfor'");
    consume(JinjaTokenType::KwEndFor, "Expected 'endfor' to close loop");
    consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after endfor");
    return for_node;
}

std::unique_ptr<ast::SetNode> JinjaParser::parseSetStatement() {
    size_t line = previous().line;
    size_t col = previous().column;

    const auto& id_tok = consume(JinjaTokenType::Identifier, "Expected identifier after 'set'");
    consume(JinjaTokenType::Assign, "Expected '=' in set statement");
    auto expr = parseExpression();
    consume(JinjaTokenType::BlockEnd, "Expected '%}' or '-%}' after set statement");

    return std::make_unique<ast::SetNode>(std::string(id_tok.text), std::move(expr), line, col);
}

std::unique_ptr<ast::ExprNode> JinjaParser::parseExpression(int min_prec) {
    auto lhs = parsePrimary();

    while (true) {
        lhs = parsePostfix(std::move(lhs));

        int prec = getBinaryPrecedence(peek().type);
        if (prec <= min_prec) {
            break;
        }

        JinjaTokenType op_type = advance().type;
        auto op = tokenToBinaryOp(op_type);
        auto rhs = parseExpression(prec);
        lhs = std::make_unique<ast::BinaryOpNode>(op, std::move(lhs), std::move(rhs), lhs->line, lhs->column);
    }

    return lhs;
}

std::unique_ptr<ast::ExprNode> JinjaParser::parsePrimary() {
    size_t line = peek().line;
    size_t col = peek().column;

    if (match(JinjaTokenType::NumberLiteral)) {
        std::string_view sv = previous().text;
        if (sv.find('.') != std::string_view::npos || sv.find('e') != std::string_view::npos || sv.find('E') != std::string_view::npos) {
            double val = std::stod(std::string(sv));
            return std::make_unique<ast::LiteralNode>(val, line, col);
        }
        int64_t val = 0;
        std::from_chars(sv.data(), sv.data() + sv.size(), val);
        return std::make_unique<ast::LiteralNode>(val, line, col);
    }

    if (match(JinjaTokenType::StringLiteral)) {
        std::string_view sv = previous().text;
        if (sv.size() >= 2 && ((sv.front() == '"' && sv.back() == '"') || (sv.front() == '\'' && sv.back() == '\''))) {
            sv.remove_prefix(1);
            sv.remove_suffix(1);
        }
        return std::make_unique<ast::LiteralNode>(std::string(sv), line, col);
    }

    if (match(JinjaTokenType::KwTrue)) {
        return std::make_unique<ast::LiteralNode>(true, line, col);
    }

    if (match(JinjaTokenType::KwFalse)) {
        return std::make_unique<ast::LiteralNode>(false, line, col);
    }

    if (match(JinjaTokenType::KwNone)) {
        return std::make_unique<ast::LiteralNode>(nullptr, line, col);
    }

    if (match(JinjaTokenType::Identifier)) {
        return std::make_unique<ast::IdentifierNode>(std::string(previous().text), line, col);
    }

    if (match(JinjaTokenType::ParenOpen)) {
        auto expr = parseExpression();
        consume(JinjaTokenType::ParenClose, "Expected ')' after parenthesized expression");
        return expr;
    }

    if (match(JinjaTokenType::BracketOpen)) {
        auto list_node = std::make_unique<ast::ListNode>(line, col);
        if (!check(JinjaTokenType::BracketClose)) {
            do {
                list_node->elements.push_back(parseExpression());
            } while (match(JinjaTokenType::Comma));
        }
        consume(JinjaTokenType::BracketClose, "Expected ']' after list elements");
        return list_node;
    }

    if (match(JinjaTokenType::BraceOpen)) {
        auto dict_node = std::make_unique<ast::DictNode>(line, col);
        if (!check(JinjaTokenType::BraceClose)) {
            do {
                auto key = parseExpression();
                consume(JinjaTokenType::Colon, "Expected ':' between dict key and value");
                auto val = parseExpression();
                dict_node->pairs.emplace_back(std::move(key), std::move(val));
            } while (match(JinjaTokenType::Comma));
        }
        consume(JinjaTokenType::BraceClose, "Expected '}' after dict entries");
        return dict_node;
    }

    if (match(JinjaTokenType::KwNot)) {
        // 'not' binds looser than comparisons/member-access (precedence 25)
        auto operand = parseExpression(25);
        return std::make_unique<ast::UnaryOpNode>(ast::UnaryOp::Not, std::move(operand), line, col);
    }

    if (match(JinjaTokenType::Minus) || match(JinjaTokenType::Plus)) {
        ast::UnaryOp op = previous().is(JinjaTokenType::Minus) ? ast::UnaryOp::Negate : ast::UnaryOp::Pos;
        // Unary minus/plus binds tightly (precedence 65)
        auto operand = parseExpression(65);
        return std::make_unique<ast::UnaryOpNode>(op, std::move(operand), line, col);
    }

    throw ParseError("Expected expression", line, col);
}

std::unique_ptr<ast::ExprNode> JinjaParser::parsePostfix(std::unique_ptr<ast::ExprNode> expr) {
    while (true) {
        if (match(JinjaTokenType::Dot)) {
            const auto& id = consume(JinjaTokenType::Identifier, "Expected property name after '.'");
            expr = std::make_unique<ast::MemberAccessNode>(
                std::move(expr), std::string(id.text), expr->line, expr->column);
        } else if (match(JinjaTokenType::BracketOpen)) {
            auto idx = parseExpression();
            consume(JinjaTokenType::BracketClose, "Expected ']' after subscript index");
            expr = std::make_unique<ast::SubscriptNode>(
                std::move(expr), std::move(idx), expr->line, expr->column);
        } else if (match(JinjaTokenType::ParenOpen)) {
            auto call = std::make_unique<ast::CallNode>(std::move(expr), expr->line, expr->column);
            if (!check(JinjaTokenType::ParenClose)) {
                do {
                    if (peek().is(JinjaTokenType::Identifier) && (current_ + 1 < tokens_.size()) && tokens_[current_ + 1].is(JinjaTokenType::Assign)) {
                        std::string kw_name(advance().text);
                        advance(); // consume '='
                        call->kw_args.emplace_back(std::move(kw_name), parseExpression());
                    } else {
                        call->pos_args.push_back(parseExpression());
                    }
                } while (match(JinjaTokenType::Comma));
            }
            consume(JinjaTokenType::ParenClose, "Expected ')' after function arguments");
            expr = std::move(call);
        } else if (match(JinjaTokenType::Pipe)) {
            expr = parseFilter(std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<ast::ExprNode> JinjaParser::parseFilter(std::unique_ptr<ast::ExprNode> target) {
    const auto& name_tok = consume(JinjaTokenType::Identifier, "Expected filter name after '|'");
    auto filter = std::make_unique<ast::FilterNode>(
        std::move(target), std::string(name_tok.text), target->line, target->column);

    if (match(JinjaTokenType::ParenOpen)) {
        if (!check(JinjaTokenType::ParenClose)) {
            do {
                filter->args.push_back(parseExpression());
            } while (match(JinjaTokenType::Comma));
        }
        consume(JinjaTokenType::ParenClose, "Expected ')' after filter arguments");
    }
    return filter;
}

int JinjaParser::getBinaryPrecedence(JinjaTokenType type) const noexcept {
    switch (type) {
    case JinjaTokenType::KwOr:                                    return 10;
    case JinjaTokenType::KwAnd:                                   return 20;
    case JinjaTokenType::KwIn:
    case JinjaTokenType::KwIs:                                    return 30;
    case JinjaTokenType::Eq:
    case JinjaTokenType::Ne:
    case JinjaTokenType::Lt:
    case JinjaTokenType::Le:
    case JinjaTokenType::Gt:
    case JinjaTokenType::Ge:                                      return 40;
    case JinjaTokenType::Tilde:                                   return 45;
    case JinjaTokenType::Plus:
    case JinjaTokenType::Minus:                                   return 50;
    case JinjaTokenType::Mul:
    case JinjaTokenType::Div:
    case JinjaTokenType::Mod:                                     return 60;
    default:                                                      return 0;
    }
}

ast::BinaryOp JinjaParser::tokenToBinaryOp(JinjaTokenType type) const {
    switch (type) {
    case JinjaTokenType::Plus:   return ast::BinaryOp::Add;
    case JinjaTokenType::Minus:  return ast::BinaryOp::Sub;
    case JinjaTokenType::Mul:    return ast::BinaryOp::Mul;
    case JinjaTokenType::Div:    return ast::BinaryOp::Div;
    case JinjaTokenType::Mod:    return ast::BinaryOp::Mod;
    case JinjaTokenType::Tilde:  return ast::BinaryOp::Concat;
    case JinjaTokenType::Eq:     return ast::BinaryOp::Eq;
    case JinjaTokenType::Ne:     return ast::BinaryOp::Ne;
    case JinjaTokenType::Lt:     return ast::BinaryOp::Lt;
    case JinjaTokenType::Le:     return ast::BinaryOp::Le;
    case JinjaTokenType::Gt:     return ast::BinaryOp::Gt;
    case JinjaTokenType::Ge:     return ast::BinaryOp::Ge;
    case JinjaTokenType::KwAnd:  return ast::BinaryOp::And;
    case JinjaTokenType::KwOr:   return ast::BinaryOp::Or;
    case JinjaTokenType::KwIn:   return ast::BinaryOp::In;
    case JinjaTokenType::KwIs:   return ast::BinaryOp::Is;
    default: throw std::logic_error("Invalid binary operator token");
    }
}

const JinjaToken& JinjaParser::peek() const noexcept {
    if (isAtEnd()) return eof_token_;
    return tokens_[current_];
}

const JinjaToken& JinjaParser::previous() const noexcept {
    if (current_ == 0) return eof_token_;
    return tokens_[current_ - 1];
}

bool JinjaParser::isAtEnd() const noexcept {
    return current_ >= tokens_.size() || tokens_[current_].type == JinjaTokenType::Eof;
}

const JinjaToken& JinjaParser::advance() noexcept {
    if (!isAtEnd()) current_++;
    return previous();
}

bool JinjaParser::check(JinjaTokenType type) const noexcept {
    if (isAtEnd()) return type == JinjaTokenType::Eof;
    return peek().type == type;
}

bool JinjaParser::match(JinjaTokenType type) noexcept {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const JinjaToken& JinjaParser::consume(JinjaTokenType type, std::string_view error_msg) {
    if (check(type)) return advance();
    throw ParseError(error_msg, peek().line, peek().column);
}

} // namespace job::token