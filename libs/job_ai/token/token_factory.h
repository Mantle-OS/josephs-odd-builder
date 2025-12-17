#pragma once
#include <memory>

#include <job_logger.h>

// core token stuff
#include "itoken.h"
#include "token_type.h"

// The token types
#include "byte_token.h"
#include "ascii_token.h"
#include "char_token.h"
#include "motif_token.h"
// #include "bpe_token.h"

namespace job::ai::token {

[[nodiscard]] inline std::unique_ptr<IToken> makeTokenizer(TokenType type)
{
    switch (type) {
    case TokenType::Byte:
        return std::make_unique<ByteToken>();

    case TokenType::Ascii:
        return std::make_unique<AsciiToken>();

    case TokenType::Char:
        return std::make_unique<CharToken>();

    case TokenType::Motif:
        return std::make_unique<MotifToken>();

    case TokenType::BPE:
    default:
        JOB_LOG_ERROR("Unknown Tokenizer Type: {}", static_cast<uint8_t>(type));
        return nullptr;
    }
}

}
