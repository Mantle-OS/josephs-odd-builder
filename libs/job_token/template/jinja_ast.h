#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include "jobtoken_export.h"

namespace job::token::ast {

// Forward declarations
struct Visitor;
struct TextNode;
struct OutputNode;
struct IfNode;
struct ForNode;
struct SetNode;
struct BlockNode;

struct LiteralNode;
struct IdentifierNode;
struct UnaryOpNode;
struct BinaryOpNode;
struct MemberAccessNode;
struct SubscriptNode;
struct CallNode;
struct FilterNode;
struct ListNode;
struct DictNode;

// ============================================================================
// Base AST Node & Visitor
// ============================================================================

enum class NodeType : uint8_t {
    // Statements / Template Structure
    Text,
    Output,
    If,
    For,
    Set,
    Block,

    // Expressions
    Literal,
    Identifier,
    UnaryOp,
    BinaryOp,
    MemberAccess,
    Subscript,
    Call,
    Filter,
    List,
    Dict
};

struct JOBTOKEN_EXPORT Visitor {
    virtual ~Visitor() = default;

    virtual void visit(const TextNode& node) = 0;
    virtual void visit(const OutputNode& node) = 0;
    virtual void visit(const IfNode& node) = 0;
    virtual void visit(const ForNode& node) = 0;
    virtual void visit(const SetNode& node) = 0;
    virtual void visit(const BlockNode& node) = 0;

    virtual void visit(const LiteralNode& node) = 0;
    virtual void visit(const IdentifierNode& node) = 0;
    virtual void visit(const UnaryOpNode& node) = 0;
    virtual void visit(const BinaryOpNode& node) = 0;
    virtual void visit(const MemberAccessNode& node) = 0;
    virtual void visit(const SubscriptNode& node) = 0;
    virtual void visit(const CallNode& node) = 0;
    virtual void visit(const FilterNode& node) = 0;
    virtual void visit(const ListNode& node) = 0;
    virtual void visit(const DictNode& node) = 0;
};

struct JOBTOKEN_EXPORT Node {
    NodeType type;
    size_t line{0};
    size_t column{0};

    explicit Node(NodeType t, size_t l = 0, size_t c = 0) noexcept
        : type(t), line(l), column(c) {}
    virtual ~Node() = default;

    virtual void accept(Visitor& visitor) const = 0;
};

struct JOBTOKEN_EXPORT ExprNode : Node {
    using Node::Node;
};

struct JOBTOKEN_EXPORT StmtNode : Node {
    using Node::Node;
};

// ============================================================================
// Statements / Template Structure Nodes
// ============================================================================

struct JOBTOKEN_EXPORT TextNode final : StmtNode {
    std::string text;

    explicit TextNode(std::string t, size_t l = 0, size_t c = 0)
        : StmtNode(NodeType::Text, l, c), text(std::move(t)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT OutputNode final : StmtNode {
    std::unique_ptr<ExprNode> expression;

    explicit OutputNode(std::unique_ptr<ExprNode> expr, size_t l = 0, size_t c = 0)
        : StmtNode(NodeType::Output, l, c), expression(std::move(expr)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT BlockNode final : StmtNode {
    std::vector<std::unique_ptr<StmtNode>> statements;

    explicit BlockNode(size_t l = 0, size_t c = 0)
        : StmtNode(NodeType::Block, l, c) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT IfNode final : StmtNode {
    struct Branch {
        std::unique_ptr<ExprNode> condition; // nullptr represents 'else'
        std::unique_ptr<BlockNode> body;
    };

    std::vector<Branch> branches;

    explicit IfNode(size_t l = 0, size_t c = 0)
        : StmtNode(NodeType::If, l, c) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT ForNode final : StmtNode {
    std::string loop_var;
    std::optional<std::string> value_var; // For `for key, value in dict`
    std::unique_ptr<ExprNode> iterable;
    std::unique_ptr<BlockNode> body;
    std::unique_ptr<BlockNode> else_body; // Optional Jinja for-else

    explicit ForNode(size_t l = 0, size_t c = 0)
        : StmtNode(NodeType::For, l, c) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT SetNode final : StmtNode {
    std::string name;
    std::unique_ptr<ExprNode> value;

    SetNode(std::string n, std::unique_ptr<ExprNode> val, size_t l = 0, size_t c = 0)
        : StmtNode(NodeType::Set, l, c), name(std::move(n)), value(std::move(val)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

// ============================================================================
// Expression Nodes
// ============================================================================

using LiteralValue = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

struct JOBTOKEN_EXPORT LiteralNode final : ExprNode {
    LiteralValue value;

    explicit LiteralNode(LiteralValue val, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::Literal, l, c), value(std::move(val)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT IdentifierNode final : ExprNode {
    std::string name;

    explicit IdentifierNode(std::string n, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::Identifier, l, c), name(std::move(n)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

enum class UnaryOp : uint8_t {
    Not,
    Negate,
    Pos
};

struct JOBTOKEN_EXPORT UnaryOpNode final : ExprNode {
    UnaryOp op;
    std::unique_ptr<ExprNode> operand;

    UnaryOpNode(UnaryOp o, std::unique_ptr<ExprNode> expr, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::UnaryOp, l, c), op(o), operand(std::move(expr)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

enum class BinaryOp : uint8_t {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Concat, // ~
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
    In,
    Is
};

struct JOBTOKEN_EXPORT BinaryOpNode final : ExprNode {
    BinaryOp op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;

    BinaryOpNode(BinaryOp o, std::unique_ptr<ExprNode> lhs, std::unique_ptr<ExprNode> rhs, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::BinaryOp, l, c), op(o), left(std::move(lhs)), right(std::move(rhs)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT MemberAccessNode final : ExprNode {
    std::unique_ptr<ExprNode> object;
    std::string member;

    MemberAccessNode(std::unique_ptr<ExprNode> obj, std::string m, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::MemberAccess, l, c), object(std::move(obj)), member(std::move(m)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT SubscriptNode final : ExprNode {
    std::unique_ptr<ExprNode> object;
    std::unique_ptr<ExprNode> index;

    SubscriptNode(std::unique_ptr<ExprNode> obj, std::unique_ptr<ExprNode> idx, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::Subscript, l, c), object(std::move(obj)), index(std::move(idx)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT CallNode final : ExprNode {
    std::unique_ptr<ExprNode> callee;
    std::vector<std::unique_ptr<ExprNode>> pos_args;
    std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> kw_args;

    explicit CallNode(std::unique_ptr<ExprNode> target, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::Call, l, c), callee(std::move(target)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT FilterNode final : ExprNode {
    std::unique_ptr<ExprNode> target;
    std::string filter_name;
    std::vector<std::unique_ptr<ExprNode>> args;

    FilterNode(std::unique_ptr<ExprNode> expr, std::string name, size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::Filter, l, c), target(std::move(expr)), filter_name(std::move(name)) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT ListNode final : ExprNode {
    std::vector<std::unique_ptr<ExprNode>> elements;

    explicit ListNode(size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::List, l, c) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

struct JOBTOKEN_EXPORT DictNode final : ExprNode {
    std::vector<std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ExprNode>>> pairs;

    explicit DictNode(size_t l = 0, size_t c = 0)
        : ExprNode(NodeType::Dict, l, c) {}

    void accept(Visitor& visitor) const override { visitor.visit(*this); }
};

} // namespace job::token::ast