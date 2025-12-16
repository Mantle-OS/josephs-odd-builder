#pragma once
#include <memory>

#include <job_logger.h>

#include "ascii_token.h"
#include "itoken.h"
#include "token_type.h"

#include "char_token.h"
#include "motif_token.h"

namespace job::ai::token {

[[nodiscard]] inline std::unique_ptr<IToken> makeTokenizer(TokenType type)
{
    switch (type) {
    case TokenType::Ascii:
        return std::make_unique<AsciiToken>();

    case TokenType::Char:
        return std::make_unique<CharToken>();

    case TokenType::Motif:
        return std::make_unique<MotifToken>();

    default:
        JOB_LOG_ERROR("Unknown Tokenizer Type: {}", static_cast<uint8_t>(type));
        return nullptr;
    }
}

}
