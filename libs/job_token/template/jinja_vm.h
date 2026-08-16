#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "jobtoken_export.h"
#include "template/jinja_ast.h"

namespace job::token {

class Value;
using ValueList = std::vector<Value>;
using ValueMap  = std::unordered_map<std::string, Value>;

// Dynamic Runtime Value
enum class ValueType : uint8_t {
    None,
    Bool,
    Int,
    Float,
    String,
    List,
    Map
};

class JOBTOKEN_EXPORT Value {
public:
    using VariantType = std::variant<
        std::monostate,
        bool,
        int64_t,
        double,
        std::string,
        std::shared_ptr<ValueList>,
        std::shared_ptr<ValueMap>
        >;

    Value() noexcept;
    Value(std::nullptr_t) noexcept;
    Value(bool val) noexcept;
    Value(int32_t val) noexcept;
    Value(int64_t val) noexcept;
    Value(double val) noexcept;
    Value(std::string val);
    Value(std::string_view val);
    Value(const char* val);
    Value(ValueList val);
    Value(ValueMap val);

    [[nodiscard]] ValueType type() const noexcept;
    [[nodiscard]] bool isNone() const noexcept;
    [[nodiscard]] bool isBool() const noexcept;
    [[nodiscard]] bool isInt() const noexcept;
    [[nodiscard]] bool isFloat() const noexcept;
    [[nodiscard]] bool isNumber() const noexcept;
    [[nodiscard]] bool isString() const noexcept;
    [[nodiscard]] bool isList() const noexcept;
    [[nodiscard]] bool isMap() const noexcept;

    [[nodiscard]] bool asBool() const noexcept;
    [[nodiscard]] int64_t asInt() const noexcept;
    [[nodiscard]] double asFloat() const noexcept;
    [[nodiscard]] const std::string& asString() const;
    [[nodiscard]] const ValueList& asList() const;
    [[nodiscard]] ValueList& asList();
    [[nodiscard]] const ValueMap& asMap() const;
    [[nodiscard]] ValueMap& asMap();

    [[nodiscard]] bool isTruthy() const noexcept;
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] size_t length() const;

    [[nodiscard]] Value getItem(const Value& key) const;
    void setItem(const Value& key, Value val);

    [[nodiscard]] bool operator==(const Value& other) const;
    [[nodiscard]] bool operator!=(const Value& other) const;
    [[nodiscard]] bool operator<(const Value& other) const;
    [[nodiscard]] bool operator<=(const Value& other) const;
    [[nodiscard]] bool operator>(const Value& other) const;
    [[nodiscard]] bool operator>=(const Value& other) const;

private:
    VariantType data_;
};

// Look its the JVM all over again this time orcale gets none opf my money
// Jinja Execution Virtual Machine
using FilterFn   = std::function<Value(const Value& target, const std::vector<Value>& args)>;
using FunctionFn = std::function<Value(const std::vector<Value>& pos_args, const ValueMap& kw_args)>;

class JOBTOKEN_EXPORT JinjaVM final : public ast::Visitor {
public:
    JinjaVM();
    ~JinjaVM() override = default;

    void registerFilter(std::string name, FilterFn fn);
    void registerFunction(std::string name, FunctionFn fn);

    [[nodiscard]] std::string execute(const ast::Node& root, const ValueMap& context);
    [[nodiscard]] Value evaluate(const ast::ExprNode& expr, const ValueMap& context);

    // Visitor Implementation
    void visit(const ast::TextNode& node) override;
    void visit(const ast::OutputNode& node) override;
    void visit(const ast::IfNode& node) override;
    void visit(const ast::ForNode& node) override;
    void visit(const ast::SetNode& node) override;
    void visit(const ast::BlockNode& node) override;

    void visit(const ast::LiteralNode& node) override;
    void visit(const ast::IdentifierNode& node) override;
    void visit(const ast::UnaryOpNode& node) override;
    void visit(const ast::BinaryOpNode& node) override;
    void visit(const ast::MemberAccessNode& node) override;
    void visit(const ast::SubscriptNode& node) override;
    void visit(const ast::CallNode& node) override;
    void visit(const ast::FilterNode& node) override;
    void visit(const ast::ListNode& node) override;
    void visit(const ast::DictNode& node) override;

private:
    void registerDefaultBuiltins();

    void pushScope();
    void pushScope(ValueMap scope);
    void popScope();
    void setVariable(const std::string& name, Value val);
    [[nodiscard]] Value getVariable(const std::string& name) const;
    [[nodiscard]] bool hasVariable(const std::string& name) const;

    [[nodiscard]] Value eval(const ast::ExprNode& expr);

private:
    std::string output_buffer_;
    Value last_expr_value_;
    std::vector<ValueMap> scopes_;
    std::unordered_map<std::string, FilterFn> filters_;
    std::unordered_map<std::string, FunctionFn> functions_;
};

} // namespace job::token