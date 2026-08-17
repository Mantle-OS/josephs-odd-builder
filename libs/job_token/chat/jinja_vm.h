#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "jinja_ast.h"
#include "jobtoken_export.h"

namespace job::token {

class Value;
class ValueList;
class ValueMap;

// ============================================================================
// Dynamic runtime value
// ============================================================================

enum class ValueType : uint8_t {
    None,
    Bool,
    Int,
    Float,
    String,
    List,
    Map
};

class JOBTOKEN_EXPORT Value
{
public:
    using Ptr  = std::shared_ptr<Value>;
    using WPtr = std::weak_ptr<Value>;
    using UPtr = std::unique_ptr<Value>;

    using VariantType = std::variant<
        std::monostate,
        bool,
        std::int64_t,
        double,
        std::string,
        std::shared_ptr<ValueList>,
        std::shared_ptr<ValueMap>>;

    Value() noexcept;
    Value(std::nullptr_t) noexcept;
    Value(bool value) noexcept;
    Value(std::int32_t value) noexcept;
    Value(std::int64_t value) noexcept;
    Value(double value) noexcept;
    Value(std::string value);
    Value(std::string_view value);
    Value(const char *value);
    Value(ValueList value);
    Value(ValueMap value);

    ~Value() = default;

    Value(const Value &) = default;
    Value &operator=(const Value &) = default;
    Value(Value &&) noexcept = default;
    Value &operator=(Value &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<Value>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<Value>();
    }

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
    [[nodiscard]] std::int64_t asInt() const noexcept;
    [[nodiscard]] double asFloat() const noexcept;

    [[nodiscard]] const std::string &asString() const;
    [[nodiscard]] const ValueList &asList() const;
    [[nodiscard]] ValueList &asList();
    [[nodiscard]] const ValueMap &asMap() const;
    [[nodiscard]] ValueMap &asMap();

    [[nodiscard]] bool isTruthy() const noexcept;
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::size_t length() const;

    [[nodiscard]] Value getItem(const Value &key) const;
    void setItem(const Value &key, Value value);

    [[nodiscard]] bool operator==(const Value &other) const;
    [[nodiscard]] bool operator!=(const Value &other) const;
    [[nodiscard]] bool operator<(const Value &other) const;
    [[nodiscard]] bool operator<=(const Value &other) const;
    [[nodiscard]] bool operator>(const Value &other) const;
    [[nodiscard]] bool operator>=(const Value &other) const;

private:
    VariantType m_data;
};

// ============================================================================
// Runtime containers
// ============================================================================

class JOBTOKEN_EXPORT ValueList : public std::vector<Value>
{
public:
    using std::vector<Value>::vector;
};

class JOBTOKEN_EXPORT ValueMap : public std::unordered_map<std::string, Value>
{
public:
    using std::unordered_map<std::string, Value>::unordered_map;
};

// ============================================================================
// Jinja execution VM
//
// Look, it's the JVM all over again.
// This time Oracle gets none of my money.
// ============================================================================

using FilterFn = std::function<
    Value(
        const Value &target,
        const std::vector<Value> &args,
        const ValueMap &kwArgs)>;

using FunctionFn = std::function<
    Value(
        const std::vector<Value> &posArgs,
        const ValueMap &kwArgs)>;

class JOBTOKEN_EXPORT JinjaVM : public Visitor
{
public:
    JinjaVM();
    ~JinjaVM() override = default;

    JinjaVM(const JinjaVM &) = delete;
    JinjaVM &operator=(const JinjaVM &) = delete;
    JinjaVM(JinjaVM &&) = delete;
    JinjaVM &operator=(JinjaVM &&) = delete;

    void registerFilter(std::string name, FilterFn function);
    void registerFunction(std::string name, FunctionFn function);

    [[nodiscard]] std::string execute(const Node &root, const ValueMap &context);
    [[nodiscard]] Value evaluate(const ExprNode &expression, const ValueMap &context);

    // Visitor implementation
    void visit(const TextNode &node) override;
    void visit(const OutputNode &node) override;
    void visit(const IfNode &node) override;
    void visit(const ForNode &node) override;
    void visit(const SetNode &node) override;
    void visit(const BodyNode &node) override;

    void visit(const LiteralNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const UnaryOpNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const ConditionalNode &node) override;
    void visit(const MemberAccessNode &node) override;
    void visit(const SubscriptNode &node) override;
    void visit(const CallNode &node) override;
    void visit(const FilterNode &node) override;
    void visit(const ListNode &node) override;
    void visit(const DictNode &node) override;

private:
    void registerDefaultBuiltins();

    void pushScope();
    void pushScope(ValueMap scope);
    void popScope();

    void setVariable(const std::string &name, Value value);

    [[nodiscard]] Value getVariable(const std::string &name) const;
    [[nodiscard]] bool hasVariable(const std::string &name) const;

    [[nodiscard]] Value eval(const ExprNode &expression);

    // Dispatches <object>.<member>(...) calls such as map.items().
    [[nodiscard]] Value callMember(const Value &object,
                                   const std::string &member,
                                   const std::vector<Value> &args,
                                   const ValueMap &kwArgs);

private:
    std::string m_outputBuffer;
    Value m_lastExpressionValue;

    std::vector<ValueMap> m_scopes;

    std::unordered_map<std::string, FilterFn> m_filters;
    std::unordered_map<std::string, FunctionFn> m_functions;
};

} // namespace job::token