#pragma once

#include <span>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>

#include "template/jinja_ast.h"
#include "template/jinja_token.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ParseError : public std::runtime_error {
public:
    ParseError(std::string_view msg, size_t line, size_t column)
        : std::runtime_error(std::string(msg) + " at line " + std::to_string(line) + ":" + std::to_string(column)),
        line_(line), col_(column) {}

    [[nodiscard]] size_t line() const noexcept { return line_; }
    [[nodiscard]] size_t column() const noexcept { return col_; }

private:
    size_t line_;
    size_t col_;
};

class JOBTOKEN_EXPORT JinjaParser {
public:
    explicit JinjaParser(std::span<const JinjaToken> tokens) noexcept;
    explicit JinjaParser(const std::vector<JinjaToken>& tokens) noexcept;

    [[nodiscard]] std::unique_ptr<ast::BlockNode> parse();

private:
    // Statement parsing
    std::unique_ptr<ast::BlockNode> parseBlock(std::initializer_list<JinjaTokenType> stop_tokens = {});
    std::unique_ptr<ast::StmtNode> parseStatement();
    std::unique_ptr<ast::OutputNode> parseOutputStatement();
    std::unique_ptr<ast::IfNode> parseIfStatement();
    std::unique_ptr<ast::ForNode> parseForStatement();
    std::unique_ptr<ast::SetNode> parseSetStatement();

    // Expression parsing (Pratt precedence climbing)
    std::unique_ptr<ast::ExprNode> parseExpression(int min_precedence = 0);
    std::unique_ptr<ast::ExprNode> parsePrimary();
    std::unique_ptr<ast::ExprNode> parsePostfix(std::unique_ptr<ast::ExprNode> expr);
    std::unique_ptr<ast::ExprNode> parseFilter(std::unique_ptr<ast::ExprNode> target);

    // Stream navigation & token checks
    [[nodiscard]] const JinjaToken& peek() const noexcept;
    [[nodiscard]] const JinjaToken& previous() const noexcept;
    [[nodiscard]] bool isAtEnd() const noexcept;
    const JinjaToken& advance() noexcept;
    bool check(JinjaTokenType type) const noexcept;
    bool match(JinjaTokenType type) noexcept;
    const JinjaToken& consume(JinjaTokenType type, std::string_view error_msg);

    [[nodiscard]] bool checkBlockKeyword(JinjaTokenType kw) const noexcept;
    [[nodiscard]] int getBinaryPrecedence(JinjaTokenType type) const noexcept;
    [[nodiscard]] ast::BinaryOp tokenToBinaryOp(JinjaTokenType type) const;

private:
    std::span<const JinjaToken> tokens_;
    size_t current_{0};
    static const JinjaToken eof_token_;
};

} // namespace job::token