#pragma once

#include <filesystem>

#include "itoken.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT TokenFactory
{
public:
    TokenFactory() = delete;
    ~TokenFactory() = delete;

    TokenFactory(const TokenFactory &) = delete;
    TokenFactory &operator=(const TokenFactory &) = delete;
    TokenFactory(TokenFactory &&) = delete;
    TokenFactory &operator=(TokenFactory &&) = delete;

    [[nodiscard]] static IToken::UPtr create(IToken::Provider provider, const std::filesystem::path &modelPath);
    [[nodiscard]] static IToken::UPtr create(IToken::Provider provider, const std::filesystem::path &modelPath, const std::filesystem::path &tokenizerPath);
};

} // namespace job::token