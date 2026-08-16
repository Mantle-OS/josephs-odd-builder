#include "template/jinja_vm.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace job::token {


// Value Implementation

Value::Value() noexcept :
    data_(std::monostate{})
{

}

Value::Value(std::nullptr_t) noexcept :
    data_(std::monostate{})
{

}

Value::Value(bool val) noexcept :
    data_(val)
{

}

Value::Value(int32_t val) noexcept :
    data_(static_cast<int64_t>(val))
{

}

Value::Value(int64_t val) noexcept :
    data_(val)
{

}

Value::Value(double val) noexcept :
    data_(val)
{

}

Value::Value(std::string val) :
    data_(std::move(val))
{

}

Value::Value(std::string_view val) :
    data_(std::string(val))
{

}

Value::Value(const char* val) :
    data_(std::string(val ? val : ""))
{

}

Value::Value(ValueList val) :
    data_(std::make_shared<ValueList>(std::move(val)))
{

}

Value::Value(ValueMap val) :
    data_(std::make_shared<ValueMap>(std::move(val)))
{

}

ValueType Value::type() const noexcept
{
    switch (data_.index()) {
    case 0:
        return ValueType::None;
    case 1:
        return ValueType::Bool;
    case 2:
        return ValueType::Int;
    case 3:
        return ValueType::Float;
    case 4:
        return ValueType::String;
    case 5:
        return ValueType::List;
    case 6:
        return ValueType::Map;
    default:
        return ValueType::None;
    }
}

bool Value::isNone() const noexcept
{
    return std::holds_alternative<std::monostate>(data_);
}

bool Value::isBool() const noexcept
{
    return std::holds_alternative<bool>(data_);
}

bool Value::isInt() const noexcept
{
    return std::holds_alternative<int64_t>(data_);
}

bool Value::isFloat() const noexcept
{
    return std::holds_alternative<double>(data_);
}

bool Value::isNumber() const noexcept
{
    return isInt() || isFloat();
}

bool Value::isString() const noexcept
{
    return std::holds_alternative<std::string>(data_);
}

bool Value::isList() const noexcept
{
    return std::holds_alternative<std::shared_ptr<ValueList>>(data_);
}

bool Value::isMap() const noexcept
{
    return std::holds_alternative<std::shared_ptr<ValueMap>>(data_);
}

bool Value::asBool() const noexcept
{
    if (isBool())
        return std::get<bool>(data_);

    if (isInt())
        return std::get<int64_t>(data_) != 0;

    if (isFloat())
        return std::get<double>(data_) != 0.0;
    return false;
}

int64_t Value::asInt() const noexcept
{
    if (isInt())
        return std::get<int64_t>(data_);

    if (isFloat())
        return static_cast<int64_t>(std::get<double>(data_));

    if (isBool())
        return std::get<bool>(data_) ? 1 : 0;
    return 0;
}

double Value::asFloat() const noexcept
{
    if (isFloat())
        return std::get<double>(data_);

    if (isInt())
        return static_cast<double>(std::get<int64_t>(data_));

    if (isBool())
        return std::get<bool>(data_) ? 1.0 : 0.0;
    return 0.0;
}

const std::string& Value::asString() const
{
    static const std::string empty;
    if (isString())
        return std::get<std::string>(data_);
    return empty;
}

const ValueList& Value::asList() const
{
    static const ValueList empty;
    if (isList())
        return *std::get<std::shared_ptr<ValueList>>(data_);
    return empty;
}

ValueList& Value::asList()
{
    if (!isList())
        data_ = std::make_shared<ValueList>();

    return *std::get<std::shared_ptr<ValueList>>(data_);
}

const ValueMap& Value::asMap() const
{
    static const ValueMap empty;
    if (isMap())
        return *std::get<std::shared_ptr<ValueMap>>(data_);
    return empty;
}

ValueMap& Value::asMap()
{
    if (!isMap())
        data_ = std::make_shared<ValueMap>();
    return *std::get<std::shared_ptr<ValueMap>>(data_);
}

bool Value::isTruthy() const noexcept
{
    if (isNone())
        return false;

    if (isBool())
        return std::get<bool>(data_);

    if (isInt())
        return std::get<int64_t>(data_) != 0;

    if (isFloat())
        return std::get<double>(data_) != 0.0;

    if (isString())
        return !std::get<std::string>(data_).empty();

    if (isList())
        return !asList().empty();

    if (isMap())
        return !asMap().empty();
    return false;
}

std::string Value::toString() const
{
    if (isNone())
        return "";

    if (isBool())
        return asBool() ? "True" : "False";

    if (isInt())
        return std::to_string(asInt());

    if (isFloat()) {
        std::string s = std::to_string(asFloat());
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.')
            s.pop_back();
        return s;
    }

    if (isString())
        return asString();

    if (isList()) {
        std::string out = "[";
        const auto& list = asList();
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) out += ", ";
            out += list[i].toString();
        }
        out += "]";
        return out;
    }

    if (isMap()) {
        std::string out = "{";
        const auto& map = asMap();
        size_t i = 0;
        for (const auto& [k, v] : map) {
            if (i > 0) out += ", ";
            out += "'" + k + "': " + v.toString();
            ++i;
        }
        out += "}";
        return out;
    }
    return "";
}

size_t Value::length() const
{
    if (isString())
        return asString().size();

    if (isList())
        return asList().size();

    if (isMap())
        return asMap().size();

    return 0;
}

Value Value::getItem(const Value& key) const
{
    if (isMap()) {
        const auto& map = asMap();
        auto it = map.find(key.toString());
        if (it != map.end()) return it->second;
        return Value();
    }

    if (isList()) {
        const auto& list = asList();
        int64_t idx = key.asInt();
        if (idx < 0) idx += static_cast<int64_t>(list.size());
        if (idx >= 0 && static_cast<size_t>(idx) < list.size()) {
            return list[static_cast<size_t>(idx)];
        }
        return Value();
    }

    if (isString()) {
        const auto& str = asString();
        int64_t idx = key.asInt();
        if (idx < 0) idx += static_cast<int64_t>(str.size());
        if (idx >= 0 && static_cast<size_t>(idx) < str.size()) {
            return Value(std::string(1, str[static_cast<size_t>(idx)]));
        }
        return Value();
    }

    return Value();
}

void Value::setItem(const Value& key, Value val)
{
    if (isMap()) {
        asMap()[key.toString()] = std::move(val);
    } else if (isList()) {
        int64_t idx = key.asInt();
        auto& list = asList();
        if (idx >= 0 && static_cast<size_t>(idx) < list.size()) {
            list[static_cast<size_t>(idx)] = std::move(val);
        }
    }
}

bool Value::operator==(const Value& other) const
{
    if (type() != other.type()) {
        if (isNumber() && other.isNumber())
            return std::abs(asFloat() - other.asFloat()) < 1e-9;
        return false;
    }
    if (isNone())
        return true;

    if (isBool())
        return asBool() == other.asBool();

    if (isInt())
        return asInt() == other.asInt();

    if (isFloat())
        return std::abs(asFloat() - other.asFloat()) < 1e-9;

    if (isString())
        return asString() == other.asString();

    if (isList())
        return asList() == other.asList();

    return false;
}

bool Value::operator!=(const Value& other) const
{
    return !(*this == other);
}

bool Value::operator<(const Value& other) const
{
    if (isNumber() && other.isNumber())
        return asFloat() < other.asFloat();

    if (isString() && other.isString())
        return asString() < other.asString();

    return false;
}

bool Value::operator<=(const Value& other) const
{
    return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const
{
    return !(*this <= other);
}

bool Value::operator>=(const Value& other) const
{
    return !(*this < other);
}


// JinjaVM Impl
JinjaVM::JinjaVM()
{
    registerDefaultBuiltins();
}

void JinjaVM::registerFilter(std::string name, FilterFn fn)
{
    filters_[std::move(name)] = std::move(fn);
}

void JinjaVM::registerFunction(std::string name, FunctionFn fn)
{
    functions_[std::move(name)] = std::move(fn);
}

void JinjaVM::registerDefaultBuiltins()
{
    // Filters
    registerFilter("trim", [](const Value& target, const std::vector<Value>&) -> Value {
        std::string s = target.toString();
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return Value("");
        size_t end = s.find_last_not_of(" \t\n\r");
        return Value(s.substr(start, end - start + 1));
    });

    registerFilter("lower", [](const Value& target, const std::vector<Value>&) -> Value {
        std::string s = target.toString();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return Value(s);
    });

    registerFilter("upper", [](const Value& target, const std::vector<Value>&) -> Value {
        std::string s = target.toString();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
        return Value(s);
    });

    registerFilter("length", [](const Value& target, const std::vector<Value>&) -> Value {
        return Value(static_cast<int64_t>(target.length()));
    });

    registerFilter("count", filters_["length"]);

    registerFilter("first", [](const Value& target, const std::vector<Value>&) -> Value {
        if (target.isList() && !target.asList().empty()) return target.asList().front();
        if (target.isString() && !target.asString().empty()) return Value(std::string(1, target.asString().front()));
        return Value();
    });

    registerFilter("last", [](const Value& target, const std::vector<Value>&) -> Value {
        if (target.isList() && !target.asList().empty()) return target.asList().back();
        if (target.isString() && !target.asString().empty()) return Value(std::string(1, target.asString().back()));
        return Value();
    });

    registerFilter("default", [](const Value& target, const std::vector<Value>& args) -> Value {
        if (target.isNone() || !target.isTruthy())
            return args.empty() ? Value("") : args[0];
        return target;
    });

    registerFilter("join", [](const Value& target, const std::vector<Value>& args) -> Value {
        if (!target.isList()) return target;
        std::string delim = args.empty() ? "" : args[0].toString();
        std::string result;
        const auto& list = target.asList();
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) result += delim;
            result += list[i].toString();
        }
        return Value(result);
    });

    // Functions
    registerFunction("range", [](const std::vector<Value>& pos_args, const ValueMap&) -> Value {
        int64_t start = 0, stop = 0, step = 1;
        if (pos_args.size() == 1) {
            stop = pos_args[0].asInt();
        } else if (pos_args.size() >= 2) {
            start = pos_args[0].asInt();
            stop = pos_args[1].asInt();
            if (pos_args.size() >= 3) step = pos_args[2].asInt();
        }

        if (step == 0)
            step = 1;

        ValueList result;

        if (step > 0) {
            for (int64_t i = start; i < stop; i += step) {
                result.emplace_back(i);
            }
        } else {
            for (int64_t i = start; i > stop; i += step) {
                result.emplace_back(i);
            }
        }

        return Value(std::move(result));
    });
}

std::string JinjaVM::execute(const ast::Node& root, const ValueMap& context)
{
    output_buffer_.clear();
    scopes_.clear();
    pushScope(context);

    root.accept(*this);

    popScope();
    return output_buffer_;
}

Value JinjaVM::evaluate(const ast::ExprNode& expr, const ValueMap& context)
{
    scopes_.clear();
    pushScope(context);
    Value res = eval(expr);
    popScope();
    return res;
}

Value JinjaVM::eval(const ast::ExprNode& expr)
{
    expr.accept(*this);
    return last_expr_value_;
}

void JinjaVM::pushScope()
{
    scopes_.emplace_back();
}

void JinjaVM::pushScope(ValueMap scope)
{
    scopes_.push_back(std::move(scope));
}

void JinjaVM::popScope()
{
    if (!scopes_.empty())
        scopes_.pop_back();
}

void JinjaVM::setVariable(const std::string& name, Value val)
{
    if (scopes_.empty())
        pushScope();
    scopes_.back()[name] = std::move(val);
}

Value JinjaVM::getVariable(const std::string& name) const
{
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return Value();
}

bool JinjaVM::hasVariable(const std::string& name) const
{
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if (it->find(name) != it->end()) {
            return true;
        }
    }
    return false;
}


// AST Visitor Impl
void JinjaVM::visit(const ast::TextNode& node)
{
    output_buffer_ += node.text;
}

void JinjaVM::visit(const ast::OutputNode& node)
{
    if (node.expression) {
        Value val = eval(*node.expression);
        output_buffer_ += val.toString();
    }
}

void JinjaVM::visit(const ast::BlockNode& node)
{
    for (const auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void JinjaVM::visit(const ast::IfNode& node)
{
    for (const auto& branch : node.branches) {
        if (!branch.condition || eval(*branch.condition).isTruthy()) {
            if (branch.body) branch.body->accept(*this);
            break;
        }
    }
}

void JinjaVM::visit(const ast::ForNode& node)
{
    if (!node.iterable)
        return;

    Value iter_val = eval(*node.iterable);
    ValueList items;

    if (iter_val.isList()) {
        items = iter_val.asList();
    } else if (iter_val.isMap()) {
        for (const auto& [k, v] : iter_val.asMap()) {
            ValueList pair = {Value(k), v};
            items.emplace_back(std::move(pair));
        }
    }

    if (items.empty()) {
        if (node.else_body) node.else_body->accept(*this);
        return;
    }

    size_t total = items.size();
    for (size_t i = 0; i < total; ++i) {
        pushScope();

        // Populate standard Jinja `loop` context variable
        ValueMap loop_ctx;
        loop_ctx["index0"] = Value(static_cast<int64_t>(i));
        loop_ctx["index"] = Value(static_cast<int64_t>(i + 1));
        loop_ctx["first"] = Value(i == 0);
        loop_ctx["last"] = Value(i == total - 1);
        loop_ctx["length"] = Value(static_cast<int64_t>(total));
        if (i > 0) loop_ctx["previtem"] = items[i - 1];
        if (i + 1 < total) loop_ctx["nextitem"] = items[i + 1];
        setVariable("loop", Value(std::move(loop_ctx)));

        // Bind loop targets
        if (node.value_var && items[i].isList() && items[i].asList().size() >= 2) {
            setVariable(node.loop_var, items[i].asList()[0]);
            setVariable(*node.value_var, items[i].asList()[1]);
        } else {
            setVariable(node.loop_var, items[i]);
        }

        if (node.body) node.body->accept(*this);

        popScope();
    }
}

void JinjaVM::visit(const ast::SetNode& node)
{
    if (node.value) {
        setVariable(node.name, eval(*node.value));
    }
}

void JinjaVM::visit(const ast::LiteralNode& node)
{
    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            last_expr_value_ = Value();
        } else {
            last_expr_value_ = Value(arg);
        }
    }, node.value);
}

void JinjaVM::visit(const ast::IdentifierNode& node)
{
    last_expr_value_ = getVariable(node.name);
}

void JinjaVM::visit(const ast::UnaryOpNode& node)
{
    Value operand = node.operand ? eval(*node.operand) : Value();
    switch (node.op) {
    case ast::UnaryOp::Not:
        last_expr_value_ = Value(!operand.isTruthy());
        break;
    case ast::UnaryOp::Negate:
        if (operand.isInt()) last_expr_value_ = Value(-operand.asInt());
        else if (operand.isFloat()) last_expr_value_ = Value(-operand.asFloat());
        else last_expr_value_ = Value(0);
        break;
    case ast::UnaryOp::Pos:
        last_expr_value_ = operand;
        break;
    }
}

void JinjaVM::visit(const ast::BinaryOpNode& node)
{
    if (node.op == ast::BinaryOp::And) {
        Value lhs = node.left ? eval(*node.left) : Value();
        last_expr_value_ = lhs.isTruthy() ? (node.right ? eval(*node.right) : Value()) : lhs;
        return;
    }
    if (node.op == ast::BinaryOp::Or) {
        Value lhs = node.left ? eval(*node.left) : Value();
        last_expr_value_ = lhs.isTruthy() ? lhs : (node.right ? eval(*node.right) : Value());
        return;
    }

    Value lhs = node.left ? eval(*node.left) : Value();
    Value rhs = node.right ? eval(*node.right) : Value();

    switch (node.op) {
    case ast::BinaryOp::Add:
        if (lhs.isString() || rhs.isString()) {
            last_expr_value_ = Value(lhs.toString() + rhs.toString());
        } else if (lhs.isFloat() || rhs.isFloat()) {
            last_expr_value_ = Value(lhs.asFloat() + rhs.asFloat());
        } else {
            last_expr_value_ = Value(lhs.asInt() + rhs.asInt());
        }
        break;
    case ast::BinaryOp::Sub:
        if (lhs.isFloat() || rhs.isFloat()) {
            last_expr_value_ = Value(lhs.asFloat() - rhs.asFloat());
        } else {
            last_expr_value_ = Value(lhs.asInt() - rhs.asInt());
        }
        break;
    case ast::BinaryOp::Mul:
        if (lhs.isFloat() || rhs.isFloat()) {
            last_expr_value_ = Value(lhs.asFloat() * rhs.asFloat());
        } else {
            last_expr_value_ = Value(lhs.asInt() * rhs.asInt());
        }
        break;
    case ast::BinaryOp::Div:
        last_expr_value_ = Value(rhs.asFloat() != 0.0 ? lhs.asFloat() / rhs.asFloat() : 0.0);
        break;
    case ast::BinaryOp::Mod:
        last_expr_value_ = Value(rhs.asInt() != 0 ? lhs.asInt() % rhs.asInt() : 0);
        break;
    case ast::BinaryOp::Concat:
        last_expr_value_ = Value(lhs.toString() + rhs.toString());
        break;
    case ast::BinaryOp::Eq:
        last_expr_value_ = Value(lhs == rhs);
        break;
    case ast::BinaryOp::Ne:
        last_expr_value_ = Value(lhs != rhs);
        break;
    case ast::BinaryOp::Lt:
        last_expr_value_ = Value(lhs < rhs);
        break;
    case ast::BinaryOp::Le:
        last_expr_value_ = Value(lhs <= rhs);
        break;
    case ast::BinaryOp::Gt:
        last_expr_value_ = Value(lhs > rhs);
        break;
    case ast::BinaryOp::Ge:
        last_expr_value_ = Value(lhs >= rhs);
        break;
    case ast::BinaryOp::In: {
        bool found = false;
        if (rhs.isList()) {
            const auto& list = rhs.asList();
            found = std::find(list.begin(), list.end(), lhs) != list.end();
        } else if (rhs.isMap()) {
            found = rhs.asMap().find(lhs.toString()) != rhs.asMap().end();
        } else if (rhs.isString()) {
            found = rhs.asString().find(lhs.toString()) != std::string::npos;
        }
        last_expr_value_ = Value(found);
        break;
    }
    case ast::BinaryOp::Is: {
        std::string test_name = rhs.isString() ? rhs.asString() : "";
        if (test_name == "defined")
            last_expr_value_ = Value(!lhs.isNone());
        else if (test_name == "none")
            last_expr_value_ = Value(lhs.isNone());
        else if (test_name == "even")
            last_expr_value_ = Value(lhs.asInt() % 2 == 0);
        else if (test_name == "odd")
            last_expr_value_ = Value(lhs.asInt() % 2 != 0);
        else if (test_name == "string")
            last_expr_value_ = Value(lhs.isString());
        else if (test_name == "number")
            last_expr_value_ = Value(lhs.isNumber());
        else
            last_expr_value_ = Value(lhs == rhs);
        break;
    }
    default:
        last_expr_value_ = Value();
        break;
    }
}

void JinjaVM::visit(const ast::MemberAccessNode& node)
{
    Value obj = node.object ? eval(*node.object) : Value();
    last_expr_value_ = obj.getItem(Value(node.member));
}

void JinjaVM::visit(const ast::SubscriptNode& node)
{
    Value obj = node.object ? eval(*node.object) : Value();
    Value idx = node.index ? eval(*node.index) : Value();
    last_expr_value_ = obj.getItem(idx);
}

void JinjaVM::visit(const ast::CallNode& node)
{
    std::string func_name;
    if (node.callee && node.callee->type == ast::NodeType::Identifier)
        func_name = static_cast<const ast::IdentifierNode*>(node.callee.get())->name;

    std::vector<Value> pos_args;
    pos_args.reserve(node.pos_args.size());
    for (const auto& arg : node.pos_args) {
        pos_args.push_back(eval(*arg));
    }

    ValueMap kw_args;
    for (const auto& [k, v] : node.kw_args) {
        kw_args[k] = eval(*v);
    }

    auto it = functions_.find(func_name);
    if (it != functions_.end()) {
        last_expr_value_ = it->second(pos_args, kw_args);
    } else {
        last_expr_value_ = Value();
    }
}

void JinjaVM::visit(const ast::FilterNode& node)
{
    Value target = node.target ? eval(*node.target) : Value();
    std::vector<Value> args;
    args.reserve(node.args.size());
    for (const auto& arg : node.args) {
        args.push_back(eval(*arg));
    }

    auto it = filters_.find(node.filter_name);
    if (it != filters_.end()) {
        last_expr_value_ = it->second(target, args);
    } else {
        last_expr_value_ = target;
    }
}

void JinjaVM::visit(const ast::ListNode& node)
{
    ValueList elements;
    elements.reserve(node.elements.size());
    for (const auto& elem : node.elements) {
        elements.push_back(eval(*elem));
    }

    last_expr_value_ = Value(std::move(elements));
}

void JinjaVM::visit(const ast::DictNode& node)
{
    ValueMap map;
    for (const auto& [k, v] : node.pairs) {
        Value key_val = eval(*k);
        Value item_val = eval(*v);
        map[key_val.toString()] = std::move(item_val);
    }

    last_expr_value_ = Value(std::move(map));
}

} // namespace job::token