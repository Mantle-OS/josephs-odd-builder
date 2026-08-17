#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "jobtoken_export.h"

namespace job::token {

enum class JinjaType : uint8_t {
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
    KwTrue,              // true / True
    KwFalse,             // false / False
    KwNone,              // none / None / null

    // Literals & identifiers
    Identifier,          // variable or field name
    StringLiteral,       // "..." or '...'
    NumberLiteral,       // 123 or 123.45

    // Operators & punctuation
    Dot,                 // .
    Comma,               // ,
    Colon,               // :
    Semicolon,           // ;
    Pipe,                // |
    Tilde,               // ~
    Plus,                // +
    Minus,               // -
    Mul,                 // *
    Div,                 // /
    FloorDiv,            // //
    Mod,                 // %
    Pow,                 // **
    Eq,                  // ==
    Ne,                  // !=
    Lt,                  // <
    Le,                  // <=
    Gt,                  // >
    Ge,                  // >=
    Assign,              // =
    ParenOpen,           // (
    ParenClose,          // )
    BracketOpen,         // [
    BracketClose,        // ]
    BraceOpen,           // {
    BraceClose,          // }

    Unknown
};

[[nodiscard]] constexpr std::string_view jinjaTypeString(JinjaType type) noexcept
{
    switch (type) {
    case JinjaType::Eof:           return "EOF";
    case JinjaType::Text:          return "Text";
    case JinjaType::ExprBegin:     return "{{";
    case JinjaType::ExprEnd:       return "}}";
    case JinjaType::BlockBegin:    return "{%";
    case JinjaType::BlockEnd:      return "%}";
    case JinjaType::KwFor:         return "for";
    case JinjaType::KwIn:          return "in";
    case JinjaType::KwEndFor:      return "endfor";
    case JinjaType::KwIf:          return "if";
    case JinjaType::KwElif:        return "elif";
    case JinjaType::KwElse:        return "else";
    case JinjaType::KwEndIf:       return "endif";
    case JinjaType::KwSet:         return "set";
    case JinjaType::KwAnd:         return "and";
    case JinjaType::KwOr:          return "or";
    case JinjaType::KwNot:         return "not";
    case JinjaType::KwIs:          return "is";
    case JinjaType::KwTrue:        return "true";
    case JinjaType::KwFalse:       return "false";
    case JinjaType::KwNone:        return "none";
    case JinjaType::Identifier:    return "Identifier";
    case JinjaType::StringLiteral: return "String";
    case JinjaType::NumberLiteral: return "Number";
    case JinjaType::Dot:           return ".";
    case JinjaType::Comma:         return ",";
    case JinjaType::Colon:         return ":";
    case JinjaType::Semicolon:     return ";";
    case JinjaType::Pipe:          return "|";
    case JinjaType::Tilde:         return "~";
    case JinjaType::Plus:          return "+";
    case JinjaType::Minus:         return "-";
    case JinjaType::Mul:           return "*";
    case JinjaType::Div:           return "/";
    case JinjaType::FloorDiv:      return "//";
    case JinjaType::Mod:           return "%";
    case JinjaType::Pow:           return "**";
    case JinjaType::Eq:            return "==";
    case JinjaType::Ne:            return "!=";
    case JinjaType::Lt:            return "<";
    case JinjaType::Le:            return "<=";
    case JinjaType::Gt:            return ">";
    case JinjaType::Ge:            return ">=";
    case JinjaType::Assign:        return "=";
    case JinjaType::ParenOpen:     return "(";
    case JinjaType::ParenClose:    return ")";
    case JinjaType::BracketOpen:   return "[";
    case JinjaType::BracketClose:  return "]";
    case JinjaType::BraceOpen:     return "{";
    case JinjaType::BraceClose:    return "}";
    case JinjaType::Unknown:
    default:
        return "Unknown";
    }
}

struct JOBTOKEN_EXPORT JinjaToken {
    JinjaType        type{JinjaType::Eof};
    std::string_view text;
    std::size_t      line{1};
    std::size_t      column{1};

    [[nodiscard]] constexpr bool is(JinjaType tokenType) const noexcept
    {
        return type == tokenType;
    }
};

} // namespace job::token