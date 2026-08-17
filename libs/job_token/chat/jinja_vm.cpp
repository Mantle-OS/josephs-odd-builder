#include "jinja_vm.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <string>
#include <type_traits>

namespace job::token {

// ============================================================================
// Value
// ============================================================================

Value::Value() noexcept : m_data{std::monostate{}} {}
Value::Value(std::nullptr_t) noexcept : m_data{std::monostate{}} {}
Value::Value(bool value) noexcept : m_data{value} {}
Value::Value(std::int32_t value) noexcept : m_data{static_cast<std::int64_t>(value)} {}
Value::Value(std::int64_t value) noexcept : m_data{value} {}
Value::Value(double value) noexcept : m_data{value} {}
Value::Value(std::string value) : m_data{std::move(value)} {}
Value::Value(std::string_view value) : m_data{std::string{value}} {}
Value::Value(const char *value) : m_data{std::string{value ? value : ""}} {}
Value::Value(ValueList value) : m_data{std::make_shared<ValueList>(std::move(value))} {}
Value::Value(ValueMap value) : m_data{std::make_shared<ValueMap>(std::move(value))} {}

ValueType Value::type() const noexcept
{
    switch (m_data.index()) {
    case 0: return ValueType::None;
    case 1: return ValueType::Bool;
    case 2: return ValueType::Int;
    case 3: return ValueType::Float;
    case 4: return ValueType::String;
    case 5: return ValueType::List;
    case 6: return ValueType::Map;
    default: return ValueType::None;
    }
}

bool Value::isNone() const noexcept   { return std::holds_alternative<std::monostate>(m_data); }
bool Value::isBool() const noexcept   { return std::holds_alternative<bool>(m_data); }
bool Value::isInt() const noexcept    { return std::holds_alternative<std::int64_t>(m_data); }
bool Value::isFloat() const noexcept  { return std::holds_alternative<double>(m_data); }
bool Value::isNumber() const noexcept { return isInt() || isFloat(); }
bool Value::isString() const noexcept { return std::holds_alternative<std::string>(m_data); }
bool Value::isList() const noexcept   { return std::holds_alternative<std::shared_ptr<ValueList>>(m_data); }
bool Value::isMap() const noexcept    { return std::holds_alternative<std::shared_ptr<ValueMap>>(m_data); }

bool Value::asBool() const noexcept
{
    if (isBool())  return std::get<bool>(m_data);
    if (isInt())   return std::get<std::int64_t>(m_data) != 0;
    if (isFloat()) return std::get<double>(m_data) != 0.0;
    return false;
}

std::int64_t Value::asInt() const noexcept
{
    if (isInt())   return std::get<std::int64_t>(m_data);
    if (isFloat()) return static_cast<std::int64_t>(std::get<double>(m_data));
    if (isBool())  return std::get<bool>(m_data) ? 1 : 0;
    return 0;
}

double Value::asFloat() const noexcept
{
    if (isFloat()) return std::get<double>(m_data);
    if (isInt())   return static_cast<double>(std::get<std::int64_t>(m_data));
    if (isBool())  return std::get<bool>(m_data) ? 1.0 : 0.0;
    return 0.0;
}

const std::string &Value::asString() const
{
    static const std::string empty;
    return isString() ? std::get<std::string>(m_data) : empty;
}

const ValueList &Value::asList() const
{
    static const ValueList empty;
    return isList() ? *std::get<std::shared_ptr<ValueList>>(m_data) : empty;
}

ValueList &Value::asList()
{
    if (!isList())
        m_data = std::make_shared<ValueList>();
    return *std::get<std::shared_ptr<ValueList>>(m_data);
}

const ValueMap &Value::asMap() const
{
    static const ValueMap empty;
    return isMap() ? *std::get<std::shared_ptr<ValueMap>>(m_data) : empty;
}

ValueMap &Value::asMap()
{
    if (!isMap())
        m_data = std::make_shared<ValueMap>();
    return *std::get<std::shared_ptr<ValueMap>>(m_data);
}

bool Value::isTruthy() const noexcept
{
    if (isNone())   return false;
    if (isBool())   return asBool();
    if (isInt())    return asInt() != 0;
    if (isFloat())  return asFloat() != 0.0;
    if (isString()) return !asString().empty();
    if (isList())   return !asList().empty();
    if (isMap())    return !asMap().empty();
    return false;
}

std::string Value::toString() const
{
    if (isNone()) return {};
    if (isBool()) return asBool() ? "True" : "False";
    if (isInt())  return std::to_string(asInt());

    if (isFloat()) {
        std::string value = std::to_string(asFloat());
        const std::size_t end = value.find_last_not_of('0');
        if (end != std::string::npos) value.erase(end + 1);
        if (!value.empty() && value.back() == '.') value.pop_back();
        return value;
    }

    if (isString()) return asString();

    if (isList()) {
        std::string output{"["};
        const ValueList &list = asList();
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (i > 0) output += ", ";
            output += list[i].toString();
        }
        output += "]";
        return output;
    }

    if (isMap()) {
        std::string output{"{"};
        std::size_t index = 0;
        for (const auto &[key, value] : asMap()) {
            if (index++ > 0) output += ", ";
            output += "'"; output += key; output += "': "; output += value.toString();
        }
        output += "}";
        return output;
    }

    return {};
}

std::size_t Value::length() const
{
    if (isString()) return asString().size();
    if (isList())   return asList().size();
    if (isMap())    return asMap().size();
    return 0;
}

Value Value::getItem(const Value &key) const
{
    if (isMap()) {
        const ValueMap &map = asMap();
        auto it = map.find(key.toString());
        return it != map.end() ? it->second : Value{};
    }

    if (isList()) {
        const ValueList &list = asList();
        std::int64_t index = key.asInt();
        if (index < 0) index += static_cast<std::int64_t>(list.size());
        if (index >= 0 && static_cast<std::size_t>(index) < list.size())
            return list[static_cast<std::size_t>(index)];
        return {};
    }

    if (isString()) {
        const std::string &string = asString();
        std::int64_t index = key.asInt();
        if (index < 0) index += static_cast<std::int64_t>(string.size());
        if (index >= 0 && static_cast<std::size_t>(index) < string.size())
            return Value{std::string{1, string[static_cast<std::size_t>(index)]}};
        return {};
    }

    return {};
}

void Value::setItem(const Value &key, Value value)
{
    if (isMap()) {
        asMap()[key.toString()] = std::move(value);
        return;
    }

    if (!isList())
        return;

    ValueList &list = asList();
    std::int64_t index = key.asInt();
    if (index < 0) index += static_cast<std::int64_t>(list.size());
    if (index >= 0 && static_cast<std::size_t>(index) < list.size())
        list[static_cast<std::size_t>(index)] = std::move(value);
}

bool Value::operator==(const Value &other) const
{
    if (type() != other.type()) {
        if (isNumber() && other.isNumber())
            return std::abs(asFloat() - other.asFloat()) < 1e-9;
        return false;
    }

    if (isNone())   return true;
    if (isBool())   return asBool() == other.asBool();
    if (isInt())    return asInt() == other.asInt();
    if (isFloat())  return std::abs(asFloat() - other.asFloat()) < 1e-9;
    if (isString()) return asString() == other.asString();
    if (isList())   return asList() == other.asList();
    if (isMap())    return asMap() == other.asMap();
    return false;
}

bool Value::operator!=(const Value &other) const { return !(*this == other); }

bool Value::operator<(const Value &other) const
{
    if (isNumber() && other.isNumber()) return asFloat() < other.asFloat();
    if (isString() && other.isString()) return asString() < other.asString();
    return false;
}

bool Value::operator<=(const Value &other) const { return *this < other || *this == other; }
bool Value::operator>(const Value &other) const  { return other < *this; }
bool Value::operator>=(const Value &other) const { return other <= *this; }

// ============================================================================
// JinjaVM
// ============================================================================

JinjaVM::JinjaVM()
{
    registerDefaultBuiltins();
}

void JinjaVM::registerFilter(std::string name, FilterFn function)
{
    m_filters[std::move(name)] = std::move(function);
}

void JinjaVM::registerFunction(std::string name, FunctionFn function)
{
    m_functions[std::move(name)] = std::move(function);
}

void JinjaVM::registerDefaultBuiltins()
{
    registerFilter("trim", [](const Value &target, const std::vector<Value> &, const ValueMap &) -> Value {
        std::string value = target.toString();
        const std::size_t start = value.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return Value{""};
        const std::size_t end = value.find_last_not_of(" \t\n\r");
        return Value{value.substr(start, end - start + 1)};
    });

    registerFilter("lower", [](const Value &target, const std::vector<Value> &, const ValueMap &) -> Value {
        std::string value = target.toString();
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return Value{std::move(value)};
    });

    registerFilter("upper", [](const Value &target, const std::vector<Value> &, const ValueMap &) -> Value {
        std::string value = target.toString();
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return Value{std::move(value)};
    });

    registerFilter("length", [](const Value &target, const std::vector<Value> &, const ValueMap &) -> Value {
        return Value{static_cast<std::int64_t>(target.length())};
    });

    registerFilter("count", m_filters["length"]);

    registerFilter("first", [](const Value &target, const std::vector<Value> &, const ValueMap &) -> Value {
        if (target.isList() && !target.asList().empty())     return target.asList().front();
        if (target.isString() && !target.asString().empty()) return Value{std::string{1, target.asString().front()}};
        return {};
    });

    registerFilter("last", [](const Value &target, const std::vector<Value> &, const ValueMap &) -> Value {
        if (target.isList() && !target.asList().empty())     return target.asList().back();
        if (target.isString() && !target.asString().empty()) return Value{std::string{1, target.asString().back()}};
        return {};
    });

    registerFilter("default", [](const Value &target, const std::vector<Value> &args, const ValueMap &) -> Value {
        if (target.isNone() || !target.isTruthy())
            return args.empty() ? Value{""} : args.front();
        return target;
    });

    registerFilter("join", [](const Value &target, const std::vector<Value> &args, const ValueMap &) -> Value {
        if (!target.isList())
            return target;

        const std::string delimiter = args.empty() ? "" : args.front().toString();
        std::string output;
        const ValueList &list = target.asList();

        for (std::size_t i = 0; i < list.size(); ++i) {
            if (i > 0) output += delimiter;
            output += list[i].toString();
        }

        return Value{std::move(output)};
    });

    registerFunction("range", [](const std::vector<Value> &posArgs, const ValueMap &) -> Value {
        std::int64_t start = 0;
        std::int64_t stop = 0;
        std::int64_t step = 1;

        if (posArgs.size() == 1) {
            stop = posArgs[0].asInt();
        } else if (posArgs.size() >= 2) {
            start = posArgs[0].asInt();
            stop = posArgs[1].asInt();
            if (posArgs.size() >= 3) step = posArgs[2].asInt();
        }

        if (step == 0) step = 1;

        ValueList result;
        if (step > 0) { for (std::int64_t i = start; i < stop; i += step) result.emplace_back(i); }
        else          { for (std::int64_t i = start; i > stop; i += step) result.emplace_back(i); }

        return Value{std::move(result)};
    });
}

std::string JinjaVM::execute(const Node &root, const ValueMap &context)
{
    m_outputBuffer.clear();
    m_scopes.clear();

    pushScope(context);
    root.accept(*this);
    popScope();

    return m_outputBuffer;
}

Value JinjaVM::evaluate(const ExprNode &expression, const ValueMap &context)
{
    m_scopes.clear();
    pushScope(context);

    Value result = eval(expression);

    popScope();
    return result;
}

Value JinjaVM::eval(const ExprNode &expression)
{
    expression.accept(*this);
    return m_lastExpressionValue;
}

void JinjaVM::pushScope() { m_scopes.emplace_back(); }
void JinjaVM::pushScope(ValueMap scope) { m_scopes.push_back(std::move(scope)); }
void JinjaVM::popScope() { if (!m_scopes.empty()) m_scopes.pop_back(); }

void JinjaVM::setVariable(const std::string &name, Value value)
{
    if (m_scopes.empty())
        pushScope();
    m_scopes.back()[name] = std::move(value);
}

Value JinjaVM::getVariable(const std::string &name) const
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return {};
}

bool JinjaVM::hasVariable(const std::string &name) const
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        if (it->contains(name)) return true;
    return false;
}

// ============================================================================
// Visitor
// ============================================================================

void JinjaVM::visit(const TextNode &node) { m_outputBuffer += node.text; }

void JinjaVM::visit(const OutputNode &node)
{
    if (node.expression)
        m_outputBuffer += eval(*node.expression).toString();
}

void JinjaVM::visit(const BodyNode &node)
{
    for (const auto &statement : node.statements)
        if (statement) statement->accept(*this);
}

void JinjaVM::visit(const IfNode &node)
{
    for (const auto &branch : node.branches) {
        if (!branch.condition || eval(*branch.condition).isTruthy()) {
            if (branch.body) branch.body->accept(*this);
            break;
        }
    }
}

void JinjaVM::visit(const ForNode &node)
{
    if (!node.iterable)
        return;

    Value iterable = eval(*node.iterable);
    ValueList items;

    if (iterable.isList()) {
        items = iterable.asList();
    } else if (iterable.isMap()) {
        // Direct map iteration yields keys.
        // map.items() is handled by callMember() and returns key/value pairs.
        for (const auto &entry : iterable.asMap())
            items.emplace_back(Value{entry.first});
    }

    if (items.empty()) {
        if (node.elseBody)
            node.elseBody->accept(*this);
        return;
    }

    const std::size_t total = items.size();
    for (std::size_t i = 0; i < total; ++i) {
        pushScope();

        ValueMap loop;
        loop["index0"] = Value{static_cast<std::int64_t>(i)};
        loop["index"]  = Value{static_cast<std::int64_t>(i + 1)};
        loop["first"]  = Value{i == 0};
        loop["last"]   = Value{i == total - 1};
        loop["length"] = Value{static_cast<std::int64_t>(total)};

        if (i > 0)
            loop["previtem"] = items[i - 1];

        if (i + 1 < total)
            loop["nextitem"] = items[i + 1];

        setVariable("loop", Value{std::move(loop)});

        if (node.targets.size() == 1) {
            setVariable(node.targets.front(), items[i]);
        } else if (items[i].isList()) {
            const ValueList &values = items[i].asList();

            for (std::size_t target = 0;
                 target < node.targets.size() && target < values.size();
                 ++target) {
                setVariable(node.targets[target], values[target]);
            }
        }

        if (node.body)
            node.body->accept(*this);

        popScope();
    }
}

void JinjaVM::visit(const SetNode &node)
{
    if (node.value)
        setVariable(node.name, eval(*node.value));
}

void JinjaVM::visit(const LiteralNode &node)
{
    std::visit([this](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) m_lastExpressionValue = Value{};
        else                                              m_lastExpressionValue = Value{value};
    }, node.value);
}

void JinjaVM::visit(const IdentifierNode &node) { m_lastExpressionValue = getVariable(node.name); }

void JinjaVM::visit(const UnaryOpNode &node)
{
    Value operand = node.operand ? eval(*node.operand) : Value{};

    switch (node.op) {
    case UnaryOp::Not:    m_lastExpressionValue = Value{!operand.isTruthy()}; break;
    case UnaryOp::Negate: m_lastExpressionValue = operand.isFloat() ? Value{-operand.asFloat()} : Value{-operand.asInt()}; break;
    case UnaryOp::Pos:    m_lastExpressionValue = operand; break;
    }
}

void JinjaVM::visit(const BinaryOpNode &node)
{
    if (node.op == BinaryOp::And) {
        Value left = node.left ? eval(*node.left) : Value{};
        m_lastExpressionValue = left.isTruthy() ? (node.right ? eval(*node.right) : Value{}) : left;
        return;
    }

    if (node.op == BinaryOp::Or) {
        Value left = node.left ? eval(*node.left) : Value{};
        m_lastExpressionValue = left.isTruthy() ? left : (node.right ? eval(*node.right) : Value{});
        return;
    }

    Value left = node.left ? eval(*node.left) : Value{};
    Value right = node.right ? eval(*node.right) : Value{};

    switch (node.op) {
    case BinaryOp::Add:
        if (left.isString() || right.isString())    m_lastExpressionValue = Value{left.toString() + right.toString()};
        else if (left.isFloat() || right.isFloat())  m_lastExpressionValue = Value{left.asFloat() + right.asFloat()};
        else                                          m_lastExpressionValue = Value{left.asInt() + right.asInt()};
        break;

    case BinaryOp::Sub:
        m_lastExpressionValue = left.isFloat() || right.isFloat() ? Value{left.asFloat() - right.asFloat()} : Value{left.asInt() - right.asInt()};
        break;

    case BinaryOp::Mul:
        m_lastExpressionValue = left.isFloat() || right.isFloat() ? Value{left.asFloat() * right.asFloat()} : Value{left.asInt() * right.asInt()};
        break;

    case BinaryOp::Div:
        m_lastExpressionValue = right.asFloat() != 0.0 ? Value{left.asFloat() / right.asFloat()} : Value{0.0};
        break;

    case BinaryOp::FloorDiv:
        m_lastExpressionValue = right.asFloat() != 0.0 ? Value{std::floor(left.asFloat() / right.asFloat())} : Value{0.0};
        break;

    case BinaryOp::Mod:
        m_lastExpressionValue = right.asInt() != 0 ? Value{left.asInt() % right.asInt()} : Value{std::int64_t{0}};
        break;

    case BinaryOp::Pow:
        m_lastExpressionValue = Value{std::pow(left.asFloat(), right.asFloat())};
        break;

    case BinaryOp::Concat:
        m_lastExpressionValue = Value{left.toString() + right.toString()};
        break;

    case BinaryOp::Eq: m_lastExpressionValue = Value{left == right}; break;
    case BinaryOp::Ne: m_lastExpressionValue = Value{left != right}; break;
    case BinaryOp::Lt: m_lastExpressionValue = Value{left < right};  break;
    case BinaryOp::Le: m_lastExpressionValue = Value{left <= right}; break;
    case BinaryOp::Gt: m_lastExpressionValue = Value{left > right};  break;
    case BinaryOp::Ge: m_lastExpressionValue = Value{left >= right}; break;

    case BinaryOp::In:
    case BinaryOp::NotIn: {
        bool found = false;
        if (right.isList())        found = std::find(right.asList().begin(), right.asList().end(), left) != right.asList().end();
        else if (right.isMap())    found = right.asMap().contains(left.toString());
        else if (right.isString()) found = right.asString().find(left.toString()) != std::string::npos;
        m_lastExpressionValue = Value{node.op == BinaryOp::NotIn ? !found : found};
        break;
    }

    case BinaryOp::Is:
    case BinaryOp::IsNot: {
        bool result = false;
        const std::string test = right.isString() ? right.asString() : right.toString();

        if (test == "defined")      result = !left.isNone();
        else if (test == "none")    result = left.isNone();
        else if (test == "even")    result = left.isInt() && left.asInt() % 2 == 0;
        else if (test == "odd")     result = left.isInt() && left.asInt() % 2 != 0;
        else if (test == "string")  result = left.isString();
        else if (test == "number")  result = left.isNumber();
        else if (test == "true")    result = left.isBool() && left.asBool();
        else if (test == "false")   result = left.isBool() && !left.asBool();
        else                        result = left == right;

        m_lastExpressionValue = Value{node.op == BinaryOp::IsNot ? !result : result};
        break;
    }

    case BinaryOp::And:
    case BinaryOp::Or:
        break;
    }
}

void JinjaVM::visit(const ConditionalNode &node)
{
    if (!node.condition) {
        m_lastExpressionValue = {};
        return;
    }

    if (eval(*node.condition).isTruthy())
        m_lastExpressionValue = node.trueValue ? eval(*node.trueValue) : Value{};
    else
        m_lastExpressionValue = node.falseValue ? eval(*node.falseValue) : Value{};
}

void JinjaVM::visit(const MemberAccessNode &node)
{
    Value object = node.object ? eval(*node.object) : Value{};
    m_lastExpressionValue = object.getItem(Value{node.member});
}

void JinjaVM::visit(const SubscriptNode &node)
{
    Value object = node.object ? eval(*node.object) : Value{};
    Value index = node.index ? eval(*node.index) : Value{};
    m_lastExpressionValue = object.getItem(index);
}

Value JinjaVM::callMember(const Value &object, const std::string &member,
                          const std::vector<Value> &, const ValueMap &)
{
    // One branch per supported method -- add more here (e.g. .keys(),
    // .values()) the same way. Args are unused today (items() takes
    // none) but kept in the signature so adding an arg-taking method
    // later doesn't change the call site in visit(CallNode&).
    if (member == "items" && object.isMap()) {
        ValueList pairs;
        pairs.reserve(object.asMap().size());
        for (const auto &[key, value] : object.asMap())
            pairs.emplace_back(ValueList{Value{key}, value});
        return Value{std::move(pairs)};
    }

    // Unrecognized member call -- degrades the same way an unrecognized
    // free function or filter already does (see below / visit(FilterNode)).
    return {};
}

void JinjaVM::visit(const CallNode &node)
{
    std::vector<Value> posArgs;
    posArgs.reserve(node.posArgs.size());
    for (const auto &argument : node.posArgs)
        posArgs.push_back(eval(*argument));

    ValueMap kwArgs;
    for (const auto &[name, expression] : node.kwArgs)
        kwArgs[name] = eval(*expression);

    // Member call: <object>.<method>(...), e.g. map.items(). The parser
    // already produces MemberAccessNode -> CallNode for this; this was
    // the missing dispatch -- only a bare Identifier callee (free
    // functions like range(...)) was ever recognized before.
    if (node.callee && node.callee->type == NodeType::MemberAccess) {
        const auto *memberAccess = static_cast<const MemberAccessNode *>(node.callee.get());
        Value object = memberAccess->object ? eval(*memberAccess->object) : Value{};
        m_lastExpressionValue = callMember(object, memberAccess->member, posArgs, kwArgs);
        return;
    }

    std::string functionName;
    if (node.callee && node.callee->type == NodeType::Identifier)
        functionName = static_cast<const IdentifierNode *>(node.callee.get())->name;

    auto it = m_functions.find(functionName);
    if (it == m_functions.end()) {
        m_lastExpressionValue = {};
        return;
    }

    m_lastExpressionValue = it->second(posArgs, kwArgs);
}

void JinjaVM::visit(const FilterNode &node)
{
    Value target = node.target ? eval(*node.target) : Value{};

    std::vector<Value> args;
    args.reserve(node.args.size());
    for (const auto &argument : node.args)
        args.push_back(eval(*argument));

    ValueMap kwArgs;
    for (const auto &[name, expression] : node.kwArgs)
        kwArgs[name] = eval(*expression);

    auto it = m_filters.find(node.filterName);
    if (it == m_filters.end()) {
        m_lastExpressionValue = target;
        return;
    }

    m_lastExpressionValue = it->second(target, args, kwArgs);
}

void JinjaVM::visit(const ListNode &node)
{
    ValueList elements;
    elements.reserve(node.elements.size());
    for (const auto &element : node.elements)
        elements.push_back(eval(*element));
    m_lastExpressionValue = Value{std::move(elements)};
}

void JinjaVM::visit(const DictNode &node)
{
    ValueMap map;
    for (const auto &[keyExpression, valueExpression] : node.pairs) {
        Value key = eval(*keyExpression);
        Value value = eval(*valueExpression);
        map[key.toString()] = std::move(value);
    }
    m_lastExpressionValue = Value{std::move(map)};
}

} // namespace job::token