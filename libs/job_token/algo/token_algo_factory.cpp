#include "algo/token_algo_factory.h"

#include "algo/bpe.h"
#include "algo/unigram.h"
#include "algo/wordpiece.h"

namespace job::token {

ITokenAlgo::UPtr TokenAlgoFactory::create(TokenType type, const Vocab *vocab)
{
    if (!vocab)
        return nullptr;
    switch (type) {
    case TokenType::BPE:
        return Bpe::createUniq(vocab);
    case TokenType::Unigram:
        return Unigram::createUniq(vocab);
    case TokenType::WordPiece:
        return Wordpiece::createUniq(vocab);
    case TokenType::Unknown:
    default:
        return nullptr;
    }
}

} // namespace job::token