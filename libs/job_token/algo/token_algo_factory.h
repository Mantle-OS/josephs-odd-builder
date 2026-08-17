#pragma once

#include "algo/itoken_algo.h"
#include "job_token_enums.h"
#include "jobtoken_export.h"

namespace job::token {
class JOBTOKEN_EXPORT TokenAlgoFactory
{
public:
    TokenAlgoFactory() = delete;
    [[nodiscard]] static ITokenAlgo::UPtr create(TokenType type, const Vocab *vocab);
};

} // namespace job::token