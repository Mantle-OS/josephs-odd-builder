#pragma once

#include <string>
#include <string_view>

#include "job_json_escape_kernel.h"
#include "job_json_lex.h"

namespace job::json {

class JsonStringKernel
{
public:
    [[nodiscard]] static constexpr std::string_view content(const JsonLex &lex, std::string_view source) noexcept
        pre(lex.type() == JsonLexType::String)
        pre(lex.range().validFor(source))
        pre(lex.range().size() >= 2)
    {
        return lex.view(source).subview(1, lex.range().size() - 2);
    }

    [[nodiscard]] static constexpr std::string_view borrowed(const JsonLex &lex, std::string_view source) noexcept
        pre(lex.type() == JsonLexType::String)
        pre(!lex.hasEscapes())
    {
        return content(lex, source);
    }

    [[nodiscard]] static bool decode(const JsonLex &lex, std::string_view source, std::string &result)
        pre(lex.type() == JsonLexType::String)
        pre(lex.hasEscapes())
    {
        return JsonEscapeKernel::decode(content(lex, source), result);
    }
};
} // namespace job::json
