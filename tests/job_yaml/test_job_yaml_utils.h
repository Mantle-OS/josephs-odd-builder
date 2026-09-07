#pragma once

#include <job_yaml_lex.h>

#include <string_view>
#include <vector>
#include "../tests-fast-math-workaround.h"

namespace job::yaml::tests {

//
//  START LEX TEST
inline std::vector<YamlLexeme> lexAll(std::string_view source)
{
    YamlLex lex{source};
    std::vector<YamlLexeme> result;

    for (;;) {
        const YamlLexeme lexeme = lex.next();
        result.push_back(lexeme);

        if (lexeme.type == YamlLexType::End)
            break;
    }

    return result;
}
//  END LEX TEST


//
// START SCALAR_KERNEL TEST
template <typename T>
constexpr bool scalarEquals(T lhs, T rhs) noexcept
{
    if constexpr (std::floating_point<T>) {
        if constexpr (std::same_as<T, float> || std::same_as<T, double>)
            return (isSafeNaN(lhs) && isSafeNaN(rhs)) || lhs == rhs;
        else
            return lhs == rhs;
    } else {
        return lhs == rhs;
    }
}
// END SCALAR_KERNEL TEST

// START DISPACTER TO SLOW DOWN LOL
template <typename T>
[[nodiscard]] inline T benchmarkOpaque(T value) noexcept
{
    asm volatile("" : "+r"(value) : : "memory");
    return value;
}
template <typename T>
inline void benchmarkDoNotOptimize(const T &value) noexcept
{
    asm volatile("" : : "g"(&value) : "memory");
}

template <typename T>
inline void benchmarkClobber(T &value) noexcept
{
    asm volatile("" : "+m"(value) : : "memory");
}
///

} // namespace job::yaml::test
