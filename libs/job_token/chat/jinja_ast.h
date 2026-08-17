#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "jobtoken_export.h"

namespace job::token {

// Forward declarations
struct Visitor;

struct TextNode;
struct OutputNode;
struct IfNode;
struct ForNode;
struct SetNode;
struct BodyNode;

struct LiteralNode;
struct IdentifierNode;
struct UnaryOpNode;
struct BinaryOpNode;
struct ConditionalNode;
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
    // Statements / template structure
    Text,
    Output,
    If,
    For,
    Set,
    Body,

    // Expressions
    Literal,
    Identifier,
    UnaryOp,
    BinaryOp,
    Conditional,
    MemberAccess,
    Subscript,
    Call,
    Filter,
    List,
    Dict
};

struct JOBTOKEN_EXPORT Visitor {
    virtual ~Visitor() = default;

    virtual void visit(const TextNode &node) = 0;
    virtual void visit(const OutputNode &node) = 0;
    virtual void visit(const IfNode &node) = 0;
    virtual void visit(const ForNode &node) = 0;
    virtual void visit(const SetNode &node) = 0;
    virtual void visit(const BodyNode &node) = 0;

    virtual void visit(const LiteralNode &node) = 0;
    virtual void visit(const IdentifierNode &node) = 0;
    virtual void visit(const UnaryOpNode &node) = 0;
    virtual void visit(const BinaryOpNode &node) = 0;
    virtual void visit(const ConditionalNode &node) = 0;
    virtual void visit(const MemberAccessNode &node) = 0;
    virtual void visit(const SubscriptNode &node) = 0;
    virtual void visit(const CallNode &node) = 0;
    virtual void visit(const FilterNode &node) = 0;
    virtual void visit(const ListNode &node) = 0;
    virtual void visit(const DictNode &node) = 0;
};

struct JOBTOKEN_EXPORT Node {
    NodeType    type;
    std::size_t line{1};
    std::size_t column{1};

    explicit Node(NodeType nodeType, std::size_t lineNumber = 1, std::size_t columnNumber = 1) noexcept :
        type{nodeType},
        line{lineNumber},
        column{columnNumber}
    {
    }

    virtual ~Node() = default;

    virtual void accept(Visitor &visitor) const = 0;
};

struct JOBTOKEN_EXPORT ExprNode : Node {
    using Node::Node;
};

struct JOBTOKEN_EXPORT StmtNode : Node {
    using Node::Node;
};

// ============================================================================
// Statements / template structure
// ============================================================================

struct JOBTOKEN_EXPORT TextNode final : StmtNode {
    std::string text;

    explicit TextNode(std::string value, std::size_t line = 1, std::size_t column = 1) :
        StmtNode{NodeType::Text, line, column},
        text{std::move(value)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT OutputNode final : StmtNode {
    std::unique_ptr<ExprNode> expression;

    explicit OutputNode(std::unique_ptr<ExprNode> expr, std::size_t line = 1, std::size_t column = 1) :
        StmtNode{NodeType::Output, line, column},
        expression{std::move(expr)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT BodyNode final : StmtNode {
    std::vector<std::unique_ptr<StmtNode>> statements;

    explicit BodyNode(std::size_t line = 1, std::size_t column = 1) :
        StmtNode{NodeType::Body, line, column}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT IfNode final : StmtNode {
    struct Branch {
        std::unique_ptr<ExprNode> condition; // nullptr represents else
        std::unique_ptr<BodyNode> body;
    };

    std::vector<Branch> branches;

    explicit IfNode(std::size_t line = 1, std::size_t column = 1) :
        StmtNode{NodeType::If, line, column}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT ForNode final : StmtNode {
    std::vector<std::string> targets;
    std::unique_ptr<ExprNode> iterable;
    std::unique_ptr<BodyNode> body;
    std::unique_ptr<BodyNode> elseBody; // else what bro ? :P

    explicit ForNode(std::size_t line = 1, std::size_t column = 1) :
        StmtNode{NodeType::For, line, column}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT SetNode final : StmtNode {
    std::string name;
    std::unique_ptr<ExprNode> value;

    SetNode(std::string variableName, std::unique_ptr<ExprNode> expression, std::size_t line = 1, std::size_t column = 1) :
        StmtNode{NodeType::Set, line, column},
        name{std::move(variableName)},
        value{std::move(expression)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

// ============================================================================
// Expression nodes
// ============================================================================

using LiteralValue = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string>;
struct JOBTOKEN_EXPORT LiteralNode final : ExprNode {
    LiteralValue value;

    explicit LiteralNode(LiteralValue literal, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::Literal, line, column},
        value{std::move(literal)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT IdentifierNode final : ExprNode {
    std::string name;

    explicit IdentifierNode(std::string identifier, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::Identifier, line, column},
        name{std::move(identifier)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

enum class UnaryOp : uint8_t {
    Not,
    Negate,
    Pos
};

struct JOBTOKEN_EXPORT UnaryOpNode final : ExprNode {
    UnaryOp op;
    std::unique_ptr<ExprNode> operand;

    UnaryOpNode(UnaryOp operation, std::unique_ptr<ExprNode> expression, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::UnaryOp, line, column},
        op{operation},
        operand{std::move(expression)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

enum class BinaryOp : uint8_t {
    Add,
    Sub,
    Mul,
    Div,
    FloorDiv,
    Mod,
    Pow,
    Concat,
    // op
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    // cond
    And,
    Or,
    In,
    NotIn,
    Is,
    IsNot
};

struct JOBTOKEN_EXPORT BinaryOpNode final : ExprNode {
    BinaryOp op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;

    BinaryOpNode(BinaryOp operation, std::unique_ptr<ExprNode> lhs, std::unique_ptr<ExprNode> rhs, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::BinaryOp, line, column},
        op{operation},
        left{std::move(lhs)},
        right{std::move(rhs)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT ConditionalNode final : ExprNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<ExprNode> trueValue;
    std::unique_ptr<ExprNode> falseValue;

    ConditionalNode(std::unique_ptr<ExprNode> conditionExpr,
                    std::unique_ptr<ExprNode> trueExpr,
                    std::unique_ptr<ExprNode> falseExpr,
                    std::size_t line = 1,
                    std::size_t column = 1) :
        ExprNode{NodeType::Conditional, line, column},
        condition{std::move(conditionExpr)},
        trueValue{std::move(trueExpr)},
        falseValue{std::move(falseExpr)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT MemberAccessNode final : ExprNode {
    std::unique_ptr<ExprNode> object;
    std::string member;

    MemberAccessNode(std::unique_ptr<ExprNode> objectExpr, std::string memberName, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::MemberAccess, line, column},
        object{std::move(objectExpr)},
        member{std::move(memberName)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT SubscriptNode final : ExprNode {
    std::unique_ptr<ExprNode> object;
    std::unique_ptr<ExprNode> index;

    SubscriptNode(std::unique_ptr<ExprNode> objectExpr, std::unique_ptr<ExprNode> indexExpr, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::Subscript, line, column},
        object{std::move(objectExpr)},
        index{std::move(indexExpr)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT CallNode final : ExprNode {
    std::unique_ptr<ExprNode> callee;
    std::vector<std::unique_ptr<ExprNode>> posArgs;
    std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> kwArgs;

    explicit CallNode(std::unique_ptr<ExprNode> target, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::Call, line, column},
        callee{std::move(target)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT FilterNode final : ExprNode {
    std::unique_ptr<ExprNode> target;
    std::string filterName;
    std::vector<std::unique_ptr<ExprNode>> args;
    std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> kwArgs;

    FilterNode(std::unique_ptr<ExprNode> expression, std::string name, std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::Filter, line, column},
        target{std::move(expression)},
        filterName{std::move(name)}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT ListNode final : ExprNode {
    std::vector<std::unique_ptr<ExprNode>> elements;

    explicit ListNode(std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::List, line, column}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

struct JOBTOKEN_EXPORT DictNode final : ExprNode {
    using Pair = std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ExprNode>>;
    std::vector<Pair> pairs;
    explicit DictNode(std::size_t line = 1, std::size_t column = 1) :
        ExprNode{NodeType::Dict, line, column}
    {
    }

    void accept(Visitor &visitor) const override
    {
        visitor.visit(*this);
    }
};

} // namespace job::token