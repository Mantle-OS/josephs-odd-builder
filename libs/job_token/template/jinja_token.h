#pragma once

#include <cstdint>
#include <string_view>
#include "jobtoken_export.h"

namespace job::token {

enum class JinjaTokenType : uint8_t {
    Eof = 0,
    Text,               // Raw template text outside tags

    // Tag boundaries
    ExprBegin,          // {{ or {{-
    ExprEnd,            // }} or -}}
    BlockBegin,         // {% or {%-
    BlockEnd,           // %} or -%}

    // Keywords
    KwFor,              // for
    KwIn,               // in
    KwEndFor,           // endfor
    KwIf,               // if
    KwElif,             // elif
    KwElse,             // else
    KwEndIf,            // endif
    KwSet,              // set
    KwAnd,              // and
    KwOr,               // or
    KwNot,              // not
    KwIs,               // is
    KwTrue,             // true / True
    KwFalse,            // false / False
    KwNone,             // none / None / null

    // Literals & Identifiers
    Identifier,         // variable or field name (e.g. message, role, loop)
    StringLiteral,      // "..." or '...'
    NumberLiteral,      // 123 or 123.45

    // Operators & Punctuation
    Dot,                // .
    Comma,              // ,
    Colon,              // :
    Pipe,               // |
    Tilde,              // ~ (string concatenation)
    Plus,               // +
    Minus,              // -
    Mul,                // *
    Div,                // /
    Mod,                // %
    Eq,                 // ==
    Ne,                 // !=
    Lt,                 // <
    Le,                 // <=
    Gt,                 // >
    Ge,                 // >=
    Assign,             // =
    ParenOpen,          // (
    ParenClose,         // )
    BracketOpen,        // [
    BracketClose,       // ]
    BraceOpen,          // {
    BraceClose,         // }

    Unknown
};

[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view jinjaTokenTypeToString(JinjaTokenType type) noexcept
{
    switch (type) {
    case JinjaTokenType::Eof:           return "EOF";
    case JinjaTokenType::Text:          return "Text";
    case JinjaTokenType::ExprBegin:     return "{{";
    case JinjaTokenType::ExprEnd:       return "}}";
    case JinjaTokenType::BlockBegin:    return "{%";
    case JinjaTokenType::BlockEnd:      return "%}";
    case JinjaTokenType::KwFor:         return "for";
    case JinjaTokenType::KwIn:          return "in";
    case JinjaTokenType::KwEndFor:      return "endfor";
    case JinjaTokenType::KwIf:          return "if";
    case JinjaTokenType::KwElif:        return "elif";
    case JinjaTokenType::KwElse:        return "else";
    case JinjaTokenType::KwEndIf:       return "endif";
    case JinjaTokenType::KwSet:         return "set";
    case JinjaTokenType::KwAnd:         return "and";
    case JinjaTokenType::KwOr:          return "or";
    case JinjaTokenType::KwNot:         return "not";
    case JinjaTokenType::KwIs:          return "is";
    case JinjaTokenType::KwTrue:        return "true";
    case JinjaTokenType::KwFalse:       return "false";
    case JinjaTokenType::KwNone:        return "none";
    case JinjaTokenType::Identifier:    return "Identifier";
    case JinjaTokenType::StringLiteral: return "String";
    case JinjaTokenType::NumberLiteral: return "Number";
    case JinjaTokenType::Dot:           return ".";
    case JinjaTokenType::Comma:         return ",";
    case JinjaTokenType::Colon:         return ":";
    case JinjaTokenType::Pipe:          return "|";
    case JinjaTokenType::Tilde:         return "~";
    case JinjaTokenType::Plus:          return "+";
    case JinjaTokenType::Minus:         return "-";
    case JinjaTokenType::Mul:           return "*";
    case JinjaTokenType::Div:           return "/";
    case JinjaTokenType::Mod:           return "%";
    case JinjaTokenType::Eq:            return "==";
    case JinjaTokenType::Ne:            return "!=";
    case JinjaTokenType::Lt:            return "<";
    case JinjaTokenType::Le:            return "<=";
    case JinjaTokenType::Gt:            return ">";
    case JinjaTokenType::Ge:            return ">=";
    case JinjaTokenType::Assign:        return "=";
    case JinjaTokenType::ParenOpen:     return "(";
    case JinjaTokenType::ParenClose:    return ")";
    case JinjaTokenType::BracketOpen:   return "[";
    case JinjaTokenType::BracketClose:  return "]";
    case JinjaTokenType::BraceOpen:     return "{";
    case JinjaTokenType::BraceClose:    return "}";
    case JinjaTokenType::Unknown:
    default:                            return "Unknown";
    }
}

struct JOBTOKEN_EXPORT JinjaToken {
    JinjaTokenType   type{JinjaTokenType::Eof};
    std::string_view text;
    size_t           line{1};
    size_t           column{1};

    [[nodiscard]] constexpr bool is(JinjaTokenType t) const noexcept
    {
        return type == t;
    }
};

} // namespace job::token