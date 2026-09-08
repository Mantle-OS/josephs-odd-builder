#pragma once

#include <cstdint>

namespace job::json {

enum class JsonLexType : std::uint8_t
{
    Invalid = 0,
    End,

    BeginObject,
    EndObject,

    BeginArray,
    EndArray,

    NameSeparator,
    ValueSeparator,

    String,
    Number,

    True,
    False,
    Null,
};

} // namespace job::json

