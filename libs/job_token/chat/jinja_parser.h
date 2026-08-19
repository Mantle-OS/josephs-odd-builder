#pragma once
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include "jinja_ast.h"
#include "jinja_token.h"
#include "jobtoken_export.h"
namespace job::token {
class JOBTOKEN_EXPORT ParseError : public std::runtime_error
{
public:
    ParseError(std::string_view message, std::size_t line, std::size_t column) :
        std::runtime_error{
            std::string{message} +
            " at line " +
            std::to_string(line) +
            ":" +
            std::to_string(column)
        },
        m_line{line},
        m_column{column}
    {
    }
    [[nodiscard]] std::size_t line() const noexcept
    {
        return m_line;
    }
    [[nodiscard]] std::size_t column() const noexcept
    {
        return m_column;
    }
private:
    std::size_t m_line;
    std::size_t m_column;
};
class JOBTOKEN_EXPORT JinjaParser
{
public:
    explicit JinjaParser(std::span<const JinjaToken> tokens) noexcept;
    [[nodiscard]] std::unique_ptr<BodyNode> parse();
private:
    // Statement parsing
    [[nodiscard]] std::unique_ptr<BodyNode> parseBlock(std::initializer_list<JinjaType> stopTokens = {});
    [[nodiscard]] std::unique_ptr<StmtNode> parseStatement();
    [[nodiscard]] std::unique_ptr<OutputNode> parseOutputStatement();
    [[nodiscard]] std::unique_ptr<IfNode> parseIfStatement();
    [[nodiscard]] std::unique_ptr<ForNode> parseForStatement();
    [[nodiscard]] std::unique_ptr<SetNode> parseSetStatement();

    // Expression parsing
    [[nodiscard]] std::unique_ptr<ExprNode> parseExpression(int minPrecedence = 0);
    [[nodiscard]] std::unique_ptr<ExprNode> parseConditional(std::unique_ptr<ExprNode> expression);
    [[nodiscard]] std::unique_ptr<ExprNode> parsePrimary();
    [[nodiscard]] std::unique_ptr<ExprNode> parsePostfix(std::unique_ptr<ExprNode> expression);
    [[nodiscard]] std::unique_ptr<ExprNode> parseFilter(std::unique_ptr<ExprNode> target);
    [[nodiscard]] std::unique_ptr<ExprNode> parseTestName();

    // Stream navigation
    [[nodiscard]] const JinjaToken &peek() const noexcept;
    [[nodiscard]] const JinjaToken &peekNext() const noexcept;
    [[nodiscard]] const JinjaToken &previous() const noexcept;
    [[nodiscard]] bool isAtEnd() const noexcept;
    const JinjaToken &advance() noexcept;
    // Token checks
    [[nodiscard]] bool check(JinjaType type) const noexcept;
    [[nodiscard]] bool checkNext(JinjaType type) const noexcept;
    [[nodiscard]] bool match(JinjaType type) noexcept;
    const JinjaToken &consume(JinjaType type, std::string_view errorMessage);
    [[nodiscard]] bool checkBlockKeyword(JinjaType keyword) const noexcept;
    // Operators
    [[nodiscard]] int binaryPrecedence(JinjaType type) const noexcept;
    [[nodiscard]] BinaryOp binaryOp(JinjaType type) const;
private:
    [[nodiscard]] static std::string decodeStringLiteral(std::string_view value);

    std::span<const JinjaToken> m_tokens;
    std::size_t m_current{0};
    inline static const JinjaToken m_eof {
        JinjaType::Eof,
        {},
        1,
        1
    };
};
} // namespace job::token