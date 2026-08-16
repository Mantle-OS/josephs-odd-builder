#include "formats/gguf_tokenizer_reader.h"

#include <algorithm>
#include <job_logger.h>

namespace job::token {

namespace detail {

[[nodiscard]] inline int64_t readScalarInt(const ggml::JobGgufKv& kv, int64_t def = -1)
{
    if (!kv.isScalar()) return def;

    switch (kv.type()) {
    case ggml::JobGgufType::UInt8:  return static_cast<int64_t>(kv.value<uint8_t>());
    case ggml::JobGgufType::Int8:   return static_cast<int64_t>(kv.value<int8_t>());
    case ggml::JobGgufType::UInt16: return static_cast<int64_t>(kv.value<uint16_t>());
    case ggml::JobGgufType::Int16:  return static_cast<int64_t>(kv.value<int16_t>());
    case ggml::JobGgufType::UInt32: return static_cast<int64_t>(kv.value<uint32_t>());
    case ggml::JobGgufType::Int32:  return static_cast<int64_t>(kv.value<int32_t>());
    case ggml::JobGgufType::UInt64: return static_cast<int64_t>(kv.value<uint64_t>());
    case ggml::JobGgufType::Int64:  return kv.value<int64_t>();
    default:                        return def;
    }
}

[[nodiscard]] inline bool readScalarBool(const ggml::JobGgufKv& kv, bool def = false)
{
    if (!kv.isScalar()) return def;
    if (kv.isBoolean()) return kv.value<bool>();
    return readScalarInt(kv, def ? 1 : 0) != 0;
}

[[nodiscard]] inline std::string readScalarString(const ggml::JobGgufKv& kv, const std::string& def = "")
{
    if (kv.isScalar() && kv.isString()) {
        return kv.value<std::string>();
    }
    return def;
}

[[nodiscard]] inline HfModelType mapGgufModelToHf(std::string_view model) noexcept
{
    if (model == "gpt2" || model == "llama" || model == "falcon" || model == "qwen" || model == "qwen2") {
        return HfModelType::BPE;
    }
    if (model == "bert" || model == "nomic-bert") {
        return HfModelType::WordPiece;
    }
    if (model == "t5" || model == "command-r") {
        return HfModelType::Unigram;
    }
    return HfModelType::BPE;
}

} // namespace detail

bool GgufTokenizerReader::loadFromFile(const std::filesystem::path &path)
{
    clear();

    if (!std::filesystem::exists(path)) {
        JOB_LOG_ERROR("GGUF file does not exist: {}", path.string());
        return false;
    }

    ggml::JobGguf gguf;
    if (!gguf.open(path)) {
        JOB_LOG_ERROR("Failed to open GGUF file '{}': {}", path.string(), gguf.errorString());
        return false;
    }

    return loadFromGguf(gguf);
}

bool GgufTokenizerReader::loadFromMemory(const void* data, size_t size)
{
    clear();

    if (!data || size == 0) {
        JOB_LOG_ERROR("Invalid pointer or zero size passed to GgufTokenizerReader::loadFromMemory");
        return false;
    }

    ggml::JobGguf gguf;
    if (!gguf.open(data, size)) {
        JOB_LOG_ERROR("Failed to read GGUF from memory buffer: {}", gguf.errorString());
        return false;
    }

    return loadFromGguf(gguf);
}

bool GgufTokenizerReader::loadFromMemory(std::span<const std::byte> buffer)
{
    return loadFromMemory(buffer.data(), buffer.size_bytes());
}

bool GgufTokenizerReader::loadFromGguf(const ggml::JobGguf& gguf)
{
    clear();

    if (!gguf.isValid()) {
        JOB_LOG_ERROR("Provided JobGguf instance is invalid");
        return false;
    }

    // 1. Model architecture and tokenizer model type
    if (gguf.hasKey("tokenizer.ggml.model")) {
        auto kv = gguf.keyValue("tokenizer.ggml.model");
        if (kv) {
            m_data.modelName = detail::readScalarString(*kv);
            m_data.modelType = detail::mapGgufModelToHf(m_data.modelName);
        }
    }

    if (gguf.hasKey("tokenizer.ggml.pre")) {
        auto kv = gguf.keyValue("tokenizer.ggml.pre");
        if (kv) m_data.preTokenizer = detail::readScalarString(*kv);
    }

    // 2. Chat Template
    if (gguf.hasKey("tokenizer.chat_template")) {
        auto kv = gguf.keyValue("tokenizer.chat_template");
        if (kv) m_data.chatTemplate = detail::readScalarString(*kv);
    }

    // 3. Special Token IDs
    auto readId = [&](const std::string& key) -> int32_t {
        if (gguf.hasKey(key)) {
            auto kv = gguf.keyValue(key);
            if (kv) return static_cast<int32_t>(detail::readScalarInt(*kv, -1));
        }
        return -1;
    };

    m_data.bosId  = readId("tokenizer.ggml.bos_token_id");
    m_data.eosId  = readId("tokenizer.ggml.eos_token_id");
    m_data.unkId  = readId("tokenizer.ggml.unknown_token_id");
    m_data.padId  = readId("tokenizer.ggml.padding_token_id");
    m_data.clsId  = readId("tokenizer.ggml.cls_token_id");
    m_data.sepId  = readId("tokenizer.ggml.seperator_token_id");
    if (m_data.sepId < 0) {
        m_data.sepId = readId("tokenizer.ggml.sep_token_id");
    }
    m_data.maskId = readId("tokenizer.ggml.mask_token_id");

    // 4. Tokenizer flags
    if (gguf.hasKey("tokenizer.ggml.add_bos_token")) {
        auto kv = gguf.keyValue("tokenizer.ggml.add_bos_token");
        if (kv) m_data.addBosToken = detail::readScalarBool(*kv, true);
    }
    if (gguf.hasKey("tokenizer.ggml.add_eos_token")) {
        auto kv = gguf.keyValue("tokenizer.ggml.add_eos_token");
        if (kv) m_data.addEosToken = detail::readScalarBool(*kv, false);
    }
    if (gguf.hasKey("tokenizer.ggml.add_space_prefix")) {
        auto kv = gguf.keyValue("tokenizer.ggml.add_space_prefix");
        if (kv) m_data.addPrefixSpace = detail::readScalarBool(*kv, false);
    }

    // 5. Vocab Tokens
    if (!gguf.hasKey("tokenizer.ggml.tokens")) {
        JOB_LOG_ERROR("GGUF model is missing required 'tokenizer.ggml.tokens' array");
        return false;
    }

    auto tokensKv = gguf.keyValue("tokenizer.ggml.tokens");
    if (!tokensKv || !tokensKv->isArray() || !tokensKv->isString()) {
        JOB_LOG_ERROR("GGUF 'tokenizer.ggml.tokens' is not a valid string array");
        return false;
    }

    std::vector<std::string> tokenStrings = tokensKv->values<std::string>();
    size_t numTokens = tokenStrings.size();

    m_data.vocab.resize(numTokens);
    m_data.tokenToId.reserve(numTokens);

    for (size_t i = 0; i < numTokens; ++i) {
        m_data.tokenToId[tokenStrings[i]] = static_cast<int32_t>(i);
        m_data.vocab[i].text = std::move(tokenStrings[i]);
    }

    // 6. Token Scores (optional)
    if (gguf.hasKey("tokenizer.ggml.scores")) {
        auto scoresKv = gguf.keyValue("tokenizer.ggml.scores");
        if (scoresKv && scoresKv->isArray() && scoresKv->isFloatingPoint()) {
            std::vector<float> scores = scoresKv->values<float>();
            size_t count = std::min(m_data.vocab.size(), scores.size());
            for (size_t i = 0; i < count; ++i) {
                m_data.vocab[i].score = scores[i];
            }
        }
    }

    // 7. Token Types (optional)
    if (gguf.hasKey("tokenizer.ggml.token_type")) {
        auto typeKv = gguf.keyValue("tokenizer.ggml.token_type");
        if (typeKv && typeKv->isArray()) {
            if (typeKv->type() == ggml::JobGgufType::Int32) {
                auto types = typeKv->values<int32_t>();
                size_t count = std::min(m_data.vocab.size(), types.size());
                for (size_t i = 0; i < count; ++i) {
                    m_data.vocab[i].type = static_cast<GgufTokenType>(types[i]);
                }
            } else if (typeKv->type() == ggml::JobGgufType::UInt32) {
                auto types = typeKv->values<uint32_t>();
                size_t count = std::min(m_data.vocab.size(), types.size());
                for (size_t i = 0; i < count; ++i) {
                    m_data.vocab[i].type = static_cast<GgufTokenType>(types[i]);
                }
            } else if (typeKv->type() == ggml::JobGgufType::Int8) {
                auto types = typeKv->values<int8_t>();
                size_t count = std::min(m_data.vocab.size(), types.size());
                for (size_t i = 0; i < count; ++i) {
                    m_data.vocab[i].type = static_cast<GgufTokenType>(types[i]);
                }
            }
        }
    }

    // 8. Merges (for BPE models)
    if (gguf.hasKey("tokenizer.ggml.merges")) {
        auto mergesKv = gguf.keyValue("tokenizer.ggml.merges");
        if (mergesKv && mergesKv->isArray() && mergesKv->isString()) {
            std::vector<std::string> rawMerges = mergesKv->values<std::string>();
            m_data.merges.reserve(rawMerges.size());

            for (auto& mergeRule : rawMerges) {
                size_t spacePos = mergeRule.find(' ');
                if (spacePos != std::string::npos) {
                    m_data.merges.emplace_back(
                        mergeRule.substr(0, spacePos),
                        mergeRule.substr(spacePos + 1));
                }
            }
        }
    }

    // Resolve string representation of special tokens
    if (m_data.bosId >= 0 && static_cast<size_t>(m_data.bosId) < m_data.vocab.size()) {
        m_data.bosToken = m_data.vocab[static_cast<size_t>(m_data.bosId)].text;
    }
    if (m_data.eosId >= 0 && static_cast<size_t>(m_data.eosId) < m_data.vocab.size()) {
        m_data.eosToken = m_data.vocab[static_cast<size_t>(m_data.eosId)].text;
    }
    if (m_data.unkId >= 0 && static_cast<size_t>(m_data.unkId) < m_data.vocab.size()) {
        m_data.unkToken = m_data.vocab[static_cast<size_t>(m_data.unkId)].text;
    }
    if (m_data.padId >= 0 && static_cast<size_t>(m_data.padId) < m_data.vocab.size()) {
        m_data.padToken = m_data.vocab[static_cast<size_t>(m_data.padId)].text;
    }


    // SHUT UP !!! :P
    // JOB_LOG_INFO("Loaded GGUF tokenizer (Model: '{}', Vocab: {}, Merges: {}, ChatTemplate: {})",
    //              m_data.modelName,
    //              m_data.vocab.size(),
    //              m_data.merges.size(),
    //              !m_data.chatTemplate.empty() ? "yes" : "no");

    return !m_data.vocab.empty();
}

std::optional<int32_t> GgufTokenizerReader::findTokenId(std::string_view token) const noexcept
{
    auto it = m_data.tokenToId.find(std::string(token));
    if (it != m_data.tokenToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string_view> GgufTokenizerReader::findTokenString(int32_t id) const noexcept
{
    if (id >= 0 && static_cast<size_t>(id) < m_data.vocab.size()) {
        return m_data.vocab[static_cast<size_t>(id)].text;
    }
    return std::nullopt;
}

void GgufTokenizerReader::clear() noexcept
{
    m_data = GgufTokenizerData{};
}

} // namespace job::token