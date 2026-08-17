#include "gguf_token.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <job_logger.h>

namespace job::token {

bool GgufToken::load(const std::filesystem::path &path)
{
    clear();

    if (!std::filesystem::exists(path)) {
        JOB_LOG_ERROR("GGUF file does not exist: {}", path.string());
        return false;
    }

    if (!m_gguf->open(path)) {
        JOB_LOG_ERROR("Failed to open GGUF file '{}': {}", path.string(), m_gguf->errorString());
        return false;
    }

    return load();
}

bool GgufToken::load(const void *data, std::size_t size)
{
    clear();
    if (!data || size == 0) {
        JOB_LOG_ERROR("Invalid pointer or zero size passed to " "GgufToken::load");
        return false;
    }

    if (!m_gguf->open(data, size)) {
        JOB_LOG_ERROR("Failed to read GGUF from memory buffer: {}", m_gguf->errorString());
        return false;
    }

    return load();
}

bool GgufToken::load(std::span<const std::byte> buffer)
{
    return load(buffer.data(), buffer.size_bytes());
}

bool GgufToken::load()
{
    if (!m_gguf->isValid()) {
        JOB_LOG_ERROR("Provided JobGguf instance is invalid");
        return false;
    }

    // ------------------------------------------------------------------------
    // Arch
    // ------------------------------------------------------------------------
    m_modelName = m_gguf->readString("tokenizer.ggml.model", "unknown");
    setTokenType(mapGgufModelToType(m_modelName));
    m_preTokenizer = m_gguf->readString("tokenizer.ggml.pre");
    setSplitPattern(mapPreTokenizer(m_preTokenizer));

    // ------------------------------------------------------------------------
    // Chat description
    // ------------------------------------------------------------------------
    setChatTemplate(m_gguf->readString("tokenizer.chat_template"));

    // ------------------------------------------------------------------------
    // Sequence / tokenizer configuration
    // ------------------------------------------------------------------------
    setAddBosToken(m_gguf->readBool("tokenizer.ggml.add_bos_token", true));
    setAddEosToken(m_gguf->readBool("tokenizer.ggml.add_eos_token", false));
    setAddPrefixSpace(m_gguf->readBool("tokenizer.ggml.add_space_prefix", false));
    setByteFallback(m_gguf->readBool( "tokenizer.ggml.byte_fallback", false));

    // ------------------------------------------------------------------------
    // Vocabulary
    // ------------------------------------------------------------------------
    const auto tokensKv = m_gguf->keyValue("tokenizer.ggml.tokens");
    if (!tokensKv) {
        JOB_LOG_ERROR("GGUF model is missing required 'tokenizer.ggml.tokens' array");
        return false;
    }

    if (!tokensKv->isArray() || !tokensKv->isString()) {
        JOB_LOG_ERROR("GGUF 'tokenizer.ggml.tokens' is not a valid string array");
        return false;
    }

    std::vector<std::string> tokenStrings = tokensKv->values<std::string>();
    std::vector<float> scores;
    const auto scoresKv = m_gguf->keyValue("tokenizer.ggml.scores");
    if (scoresKv && scoresKv->isArray() && scoresKv->isFloatingPoint())
        scores = scoresKv->values<float>();

    for (std::size_t i = 0; i < tokenStrings.size(); ++i) {
        const float score = i < scores.size() ? scores[i] : 0.0f;
        vocab()->setToken(static_cast<TokenId>(i), std::move(tokenStrings[i]), score);
    }

    // ------------------------------------------------------------------------
    // GGUF token structural types
    // ------------------------------------------------------------------------
    const auto typeKv = m_gguf->keyValue("tokenizer.ggml.token_type");
    if (typeKv && typeKv->isArray()) {
        const auto applyTypes = [this](const auto &types) {
                const std::size_t count = std::min(records().size(), types.size());

                for (std::size_t i = 0; i < count; ++i) {
                    const auto ggufType = static_cast<GgufTokenType>(types[i]);

                    // records()[i].setType(mapTokenType(ggufType));
                    vocab()->records()[i].setType(mapTokenType(ggufType));
                }
            };

        switch (typeKv->type()) {
        case ggml::JobGgufType::Int32: {
            const auto types = typeKv->values<std::int32_t>();
            applyTypes(types);
            break;
        }
        case ggml::JobGgufType::UInt32: {
            const auto types = typeKv->values<std::uint32_t>();
            applyTypes(types);
            break;
        }
        case ggml::JobGgufType::Int8: {
            const auto types = typeKv->values<std::int8_t>();
            applyTypes(types);
            break;
        }

        default:
            break;
        }
    }

    // ------------------------------------------------------------------------
    // BPE merge description
    // ------------------------------------------------------------------------
    const auto mergesKv = m_gguf->keyValue("tokenizer.ggml.merges");
    if (mergesKv && mergesKv->isArray() && mergesKv->isString()) {
        const std::vector<std::string> rawMerges = mergesKv->values<std::string>();
        m_merges.reserve(rawMerges.size());
        for (const std::string &mergeRule : rawMerges) {
            const std::size_t separator = mergeRule.find(' ');
            if (separator == std::string::npos)
                continue;

            m_merges.emplace_back(mergeRule.substr(0, separator),
                                  mergeRule.substr(separator + 1));
        }
    }

    // ------------------------------------------------------------------------
    // Special token identities
    // ------------------------------------------------------------------------
    const auto readSpecialId = [this](std::string_view key) -> TokenId {
        const std::int64_t id = m_gguf->readInt(std::string{key});
        if (id < 0)
            return kInvalidToken;

        if (static_cast<std::uint64_t>(id) > static_cast<std::uint64_t>(std::numeric_limits<TokenId>::max()))
            return kInvalidToken;

        return static_cast<TokenId>(id);
    };

    const TokenId bosId = readSpecialId("tokenizer.ggml.bos_token_id");
    const TokenId eosId = readSpecialId("tokenizer.ggml.eos_token_id");
    const TokenId unkId = readSpecialId("tokenizer.ggml.unknown_token_id");
    const TokenId padId = readSpecialId("tokenizer.ggml.padding_token_id");
    const TokenId clsId = readSpecialId("tokenizer.ggml.cls_token_id");
    TokenId sepId = readSpecialId("tokenizer.ggml.seperator_token_id");
    if (sepId == kInvalidToken)
        sepId = readSpecialId("tokenizer.ggml.sep_token_id");

    const TokenId maskId = readSpecialId("tokenizer.ggml.mask_token_id");
    SpecialTokens *special = specialTokens();
    if (special) {
        special->setBosId(bosId);
        special->setEosId(eosId);
        special->setUnkId(unkId);
        special->setPadId(padId);
        special->setClsId(clsId);
        special->setSepId(sepId);
        special->setMaskId(maskId);
    }

#ifndef NDEBUG
    JOB_LOG_INFO(
        "Loaded GGUF tokenizer "
        "(Model: '{}', Vocab: {}, Merges: {}, ChatTemplate: {})",
        m_modelName,
        vocabSize(),
        m_merges.size(),
        !chatTemplate().empty() ? "yes" : "no");
#endif

    return vocabSize() > 0;
}

} // namespace job::token