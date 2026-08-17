#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "formats/binary_vocab_reader.h"
#include "formats/gguf_tokenizer_reader.h"
#include "formats/hf_tokenizer_reader.h"
#include "job_gguf.h"
#include "jobtoken_export.h"
#include "template/chat_message.h"
#include "template/chat_template_engine.h"

namespace job::token {

namespace detail {

struct StringPair {
    std::string left;
    std::string right;
};

struct StringPairHash {
    using is_transparent = void;

    size_t operator()(const StringPair& p) const noexcept {
        return hash(p.left, p.right);
    }
    size_t operator()(const std::pair<std::string_view, std::string_view>& p) const noexcept {
        return hash(p.first, p.second);
    }

private:
    static size_t hash(std::string_view l, std::string_view r) noexcept {
        size_t h = 14695981039346656037ULL;
        for (unsigned char c : l) {
            h ^= static_cast<size_t>(c);
            h *= 1099511628211ULL;
        }
        h ^= static_cast<size_t>(' ');
        h *= 1099511628211ULL;
        for (unsigned char c : r) {
            h ^= static_cast<size_t>(c);
            h *= 1099511628211ULL;
        }
        return h;
    }
};

struct StringPairEqual {
    using is_transparent = void;

    bool operator()(const StringPair& a, const StringPair& b) const noexcept {
        return a.left == b.left && a.right == b.right;
    }
    bool operator()(const StringPair& a, const std::pair<std::string_view, std::string_view>& b) const noexcept {
        return a.left == b.first && a.right == b.second;
    }
    bool operator()(const std::pair<std::string_view, std::string_view>& a, const StringPair& b) const noexcept {
        return a.first == b.left && a.second == b.right;
    }
};

struct TransparentStringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

} // namespace detail

class JOBTOKEN_EXPORT JobTokenizer {
public:
    using Ptr  = std::shared_ptr<JobTokenizer>;
    using UPtr = std::unique_ptr<JobTokenizer>;

    JobTokenizer();
    ~JobTokenizer();

    JobTokenizer(const JobTokenizer&) = delete;
    JobTokenizer& operator=(const JobTokenizer&) = delete;
    JobTokenizer(JobTokenizer&&) noexcept = default;
    JobTokenizer& operator=(JobTokenizer&&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobTokenizer>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobTokenizer>();
    }

    // ========================================================================
    // Loading APIs
    // ========================================================================
    [[nodiscard]] bool loadHf(const std::filesystem::path& tokenizerJsonPath, const std::filesystem::path& tokenizerConfigJsonPath = {});
    [[nodiscard]] bool loadHfFromMemory(std::string_view tokenizerJson, std::string_view tokenizerConfigJson = {});
    [[nodiscard]] bool loadHf(const HfTokenizerReader& reader);

    [[nodiscard]] bool loadGguf(const std::filesystem::path& ggufPath);
    [[nodiscard]] bool loadGguf(const ggml::JobGguf& gguf);
    [[nodiscard]] bool loadGgufFromMemory(const void* data, size_t size);
    [[nodiscard]] bool loadGguf(const GgufTokenizerReader& reader);

    [[nodiscard]] bool loadBinary(const std::filesystem::path& binaryPath);
    [[nodiscard]] bool loadBinaryFromMemory(std::span<const uint8_t> buffer);
    [[nodiscard]] bool loadBinary(const BinaryVocabReader& reader);

    void clear() noexcept;
    [[nodiscard]] bool isLoaded() const noexcept { return !m_vocab.empty(); }

    // ========================================================================
    // Encode & Decode
    // ========================================================================
    [[nodiscard]] std::vector<int32_t> encode(std::string_view text, bool addBos = false, bool addEos = false) const;

    [[nodiscard]] std::string decode(std::span<const int32_t> tokens, bool stripSpecialTokens = false) const;
    [[nodiscard]] std::string decode(int32_t tokenId, bool stripSpecialTokens = false) const;

    // ========================================================================
    // Chat Template Formatting & Tokenization
    // ========================================================================
    [[nodiscard]] std::string applyChatTemplate(
        std::span<const ChatMessage> messages,
        bool addGenerationPrompt = true) const;

    [[nodiscard]] std::vector<int32_t> encodeChat(
        std::span<const ChatMessage> messages,
        bool addGenerationPrompt = true) const;

    void setCustomChatTemplate(std::string jinjaTemplate);
    [[nodiscard]] const std::string& chatTemplate() const noexcept { return m_chatTemplateStr; }

    // ========================================================================
    // Vocab & Special Token Queries
    // ========================================================================
    [[nodiscard]] size_t vocabSize() const noexcept { return m_vocab.size(); }
    [[nodiscard]] HfModelType modelType() const noexcept { return m_modelType; }

    [[nodiscard]] int32_t bosId() const noexcept { return m_bosId; }
    [[nodiscard]] int32_t eosId() const noexcept { return m_eosId; }
    [[nodiscard]] int32_t unkId() const noexcept { return m_unkId; }
    [[nodiscard]] int32_t padId() const noexcept { return m_padId; }

    [[nodiscard]] const std::string& bosToken() const noexcept { return m_bosToken; }
    [[nodiscard]] const std::string& eosToken() const noexcept { return m_eosToken; }
    [[nodiscard]] const std::string& unkToken() const noexcept { return m_unkToken; }
    [[nodiscard]] const std::string& padToken() const noexcept { return m_padToken; }

    [[nodiscard]] std::optional<int32_t> tokenToId(std::string_view token) const noexcept;
    [[nodiscard]] std::optional<std::string_view> idToToken(int32_t id) const noexcept;

    [[nodiscard]] bool isSpecialToken(int32_t id) const noexcept;

private:
    void rebuildLookupStructures();
    [[nodiscard]] bool findFirstSpecialToken(
        std::string_view sv,
        size_t& outPos,
        size_t& outLen,
        int32_t& outId) const noexcept;

    void tokenizeSegment(std::string_view segment, std::vector<int32_t>& out) const;
    void bpeTokenizeChunk(std::string_view chunk, std::vector<int32_t>& out) const;
    void emitToken(std::string_view piece, std::vector<int32_t>& out) const;

private:
    // Core Vocabulary
    std::vector<std::string> m_vocab;
    std::unordered_map<std::string, int32_t, detail::TransparentStringHash, std::equal_to<>> m_tokenToId;
    std::unordered_map<detail::StringPair, int32_t, detail::StringPairHash, detail::StringPairEqual> m_mergeRanks;
    std::unordered_set<int32_t> m_specialTokenIds;
    std::vector<std::string>    m_specialTokenStrings;

    // Special token identities
    int32_t m_bosId{-1};
    int32_t m_eosId{-1};
    int32_t m_unkId{-1};
    int32_t m_padId{-1};

    std::string m_bosToken;
    std::string m_eosToken;
    std::string m_unkToken;
    std::string m_padToken;

    // Model settings
    HfModelType m_modelType{HfModelType::BPE};
    bool        m_byteFallback{false};
    bool        m_addPrefixSpace{false};

    // Chat Template
    std::string              m_chatTemplateStr;
    ChatTemplateEngine::UPtr m_templateEngine;
};

} // namespace job::token