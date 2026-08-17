#include "job_token.h"

#include <utility>

#include "encoder/byte_encoder_factory.h"
#include "bpe.h"
#include <job_logger.h>

namespace job::token {

bool JobToken::load(IToken::Provider provider, const std::filesystem::path &modelPath)
{
    IToken::UPtr token = TokenFactory::create(
            provider,
            modelPath);

    if (!token) {
        JOB_LOG_ERROR("Failed to create token provider for '{}'", modelPath.string());
        return false;
    }

    return setToken(
        std::move(token));
}

bool JobToken::load(IToken::Provider provider,
                    const std::filesystem::path &modelPath,
                    const std::filesystem::path &tokenizerPath)
{
    IToken::UPtr token = TokenFactory::create(
            provider,
            modelPath,
            tokenizerPath);

    if (!token) {
        JOB_LOG_ERROR(
            "Failed to create token provider "
            "(Model: '{}', Tokenizer: '{}')",
            modelPath.string(),
            tokenizerPath.string());

        return false;
    }

    return setToken(
        std::move(token));
}

bool JobToken::setToken(
    IToken::UPtr token)
{
    clearRuntime();
    m_token.reset();

    if (!token) {
        JOB_LOG_ERROR("Cannot configure JobToken with a null IToken");

        return false;
    }

    m_token = std::move(token);

    if (!configure()) {
        JOB_LOG_ERROR("Failed to configure JobToken runtime");

        clearRuntime();
        m_token.reset();

        return false;
    }

    return true;
}

bool JobToken::configure()
{
    clearRuntime();

    if (!m_token) {
        JOB_LOG_ERROR("Cannot configure JobToken without token data");
        return false;
    }

    // ------------------------------------------------------------------------
    // Normalizer
    // ------------------------------------------------------------------------

    m_normalizer = UnicodeNormalizer::createUniq();
    if (!m_normalizer) {
        JOB_LOG_ERROR(
            "Failed to create UnicodeNormalizer");

        return false;
    }

    // ------------------------------------------------------------------------
    // Pre-tokenizer
    // ------------------------------------------------------------------------
    m_splitter = RegexSplitter::createUniq(m_token->splitPattern());
    if (!m_splitter) {
        JOB_LOG_ERROR("Failed to create RegexSplitter");

        return false;
    }

    if (m_token->splitPattern() == SplitPattern::Custom) {
        if (m_token->customSplitPattern().empty()) {
            JOB_LOG_ERROR("Tokenizer requests a custom split pattern but no pattern was provided");
            return false;
        }

        m_splitter->setCustomPattern(m_token->customSplitPattern());
    }

    m_byteEncoder = ByteEncoderFactory::create(m_token->byteEncoding());
    if (!m_byteEncoder)
        return false;

    // ------------------------------------------------------------------------
    // Token algorithm
    // ------------------------------------------------------------------------
    m_algorithm = TokenAlgoFactory::create(m_token->tokenType(), m_token->vocab());
    if (!m_algorithm) {
        JOB_LOG_ERROR("Failed to create token algorithm for type {}", static_cast<std::uint32_t>(m_token->tokenType()));
        return false;
    }

    // ------------------------------------------------------------------------
    // BPE Merges
    // ------------------------------------------------------------------------
    if (m_token->tokenType() == TokenType::BPE) {
        auto *bpe = dynamic_cast<Bpe *>(m_algorithm.get());

        if (!bpe) {
            JOB_LOG_ERROR("Token algorithm reports BPE but runtime is not Bpe");
            return false;
        }

        std::vector<Bpe::MergeRule> rules;
        rules.reserve(m_token->merges().size());
        for (const auto &[leftText, rightText] : m_token->merges()) {

            const TokenId left = m_token->vocab()->findId(leftText);
            const TokenId right = m_token->vocab()->findId(rightText);

            if (left == kInvalidToken || right == kInvalidToken) {
                continue;
            }

            std::string mergedText;
            mergedText.reserve(leftText.size() + rightText.size());

            mergedText += leftText;
            mergedText += rightText;

            const TokenId merged = m_token->vocab()->findId(mergedText);

            if (merged == kInvalidToken)
                continue;

            rules.push_back({left, right,merged});
        }

        bpe->setMergeRules(std::move(rules));
    }

    // ------------------------------------------------------------------------
    // Chat runtime
    // ------------------------------------------------------------------------
    m_chatEngine =
        ChatEngine::createUniq();

    if (!m_chatEngine) {
        JOB_LOG_ERROR(
            "Failed to create ChatEngine");

        return false;
    }

    if (!m_token->chatTemplate().empty()) {
        m_chatEngine->setTemplate(
            m_token->chatTemplate());
    }

#ifndef NDEBUG
    JOB_LOG_DEBUG(
        "Configured JobToken "
        "(Provider: {}, Type: {}, Vocab: {}, SplitPattern: {}, ChatTemplate: {})",
        static_cast<std::uint32_t>(
            m_token->provider()),
        static_cast<std::uint32_t>(
            m_token->tokenType()),
        m_token->vocabSize(),
        static_cast<std::uint32_t>(
            m_token->splitPattern()),
        !m_token->chatTemplate().empty()
            ? "yes"
            : "no");
#endif

    return true;
}

std::vector<TokenId> JobToken::encode(std::string_view text) const
{
    if (!isReady()) {
        JOB_LOG_ERROR(
            "Cannot encode: JobToken is not configured");
        return {};
    }

    if (text.empty()) {
        std::vector<TokenId> result;
        if (m_token->addBosToken()) {
            const TokenId bos =
                m_token->specialTokens()->bosId();

            if (bos != kInvalidToken)
                result.push_back(bos);
        }

        if (m_token->addEosToken()) {
            const TokenId eos = m_token->specialTokens()->eosId();
            if (eos != kInvalidToken)
                result.push_back(eos);
        }
        return result;
    }

    // ------------------------------------------------------------------------
    // Normalize
    // ------------------------------------------------------------------------
    UnicodeNormalizer::Options options;

    options.addDummyPrefixSpace =
        m_token->addPrefixSpace();

    // UNRESOLVED:
    // SentencePiece-style space-marker policy should eventually be described
    // explicitly by IToken/provider configuration rather than inferred here.
    options.replaceSpacesWithMarker = false;
    options.cleanExtraSpaces = false;

    const std::string normalized =
        m_normalizer->normalize(
            text,
            options);

    // ------------------------------------------------------------------------
    // Pre-tokenize
    // ------------------------------------------------------------------------
    const std::vector<std::string_view> chunks = m_splitter->split(normalized);
    std::vector<TokenId> result;

    // ------------------------------------------------------------------------
    // Sequence prefix
    // ------------------------------------------------------------------------
    if (m_token->addBosToken()) {
        const TokenId bos =
            m_token->specialTokens()->bosId();

        if (bos != kInvalidToken)
            result.push_back(bos);
    }

    // ------------------------------------------------------------------------
    // Byte encode + algorithm
    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    // Byte encode + algorithm
    // ------------------------------------------------------------------------
    if (chunks.empty()) {
        const ByteSymbols symbols =
            m_byteEncoder->encode(
                normalized);

        std::vector<TokenId> encoded =
            m_algorithm->encode(
                symbols);

        result.insert(
            result.end(),
            encoded.begin(),
            encoded.end());

    } else {
        for (const std::string_view chunk : chunks) {
            if (chunk.empty())
                continue;

            const ByteSymbols symbols =
                m_byteEncoder->encode(
                    chunk);

            std::vector<TokenId> encoded =
                m_algorithm->encode(
                    symbols);

            result.insert(
                result.end(),
                encoded.begin(),
                encoded.end());
        }
    }

    // ------------------------------------------------------------------------
    // Sequence suffix
    // ------------------------------------------------------------------------
    if (m_token->addEosToken()) {
        const TokenId eos =
            m_token->specialTokens()->eosId();

        if (eos != kInvalidToken)
            result.push_back(eos);
    }

    return result;
}

std::string JobToken::decode(std::span<const TokenId> tokens) const
{
    if (!isReady()) {
        JOB_LOG_ERROR("Cannot decode: JobToken is not configured");
        return {};
    }

    if (tokens.empty())
        return {};

    const ByteSymbols symbols = m_algorithm->decodeSymbols(tokens);

    if (symbols.empty())
        return {};

    return m_byteEncoder->decode(symbols);
}

} // namespace job::token