#include "job_tokenizer.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <queue>
#include <job_logger.h>

namespace job::token {

namespace detail {

[[nodiscard]] inline std::string formatByteToken(uint8_t byteVal)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "<0x%02X>", byteVal);
    return std::string(buf);
}

[[nodiscard]] inline std::optional<uint8_t> parseByteToken(std::string_view token)
{
    if (token.size() == 6 && token.starts_with("<0x") && token.ends_with('>')) {
        auto hexSub = token.substr(3, 2);
        uint8_t val = 0;
        auto res = std::from_chars(hexSub.data(), hexSub.data() + 2, val, 16);
        if (res.ec == std::errc{}) {
            return val;
        }
    }
    return std::nullopt;
}

enum class CharClass : uint8_t {
    Whitespace,
    Letter,
    Digit,
    Punctuation,
    Other
};

[[nodiscard]] inline CharClass classifyChar(unsigned char c) noexcept
{
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return CharClass::Whitespace;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '\'') return CharClass::Letter;
    if (c >= '0' && c <= '9') return CharClass::Digit;
    if (c < 0x80) return CharClass::Punctuation;
    return CharClass::Other;
}

struct BpeSymbol {
    uint32_t start{0};
    uint32_t len{0};
    int32_t  prev{-1};
    int32_t  next{-1};
};

struct BpeMergePair {
    int32_t  rank{0};
    int32_t  left_idx{0};
    uint32_t left_len{0};
    uint32_t right_len{0};

    bool operator>(const BpeMergePair& other) const noexcept {
        if (rank != other.rank) return rank > other.rank;
        return left_idx > other.left_idx;
    }
};

} // namespace detail

JobTokenizer::JobTokenizer()
    : m_templateEngine(ChatTemplateEngine::create(ChatTemplateType::ChatML))
{
}

JobTokenizer::~JobTokenizer() = default;

void JobTokenizer::clear() noexcept
{
    m_vocab.clear();
    m_tokenToId.clear();
    m_mergeRanks.clear();
    m_specialTokenIds.clear();
    m_specialTokenStrings.clear();

    m_bosId = -1;
    m_eosId = -1;
    m_unkId = -1;
    m_padId = -1;

    m_bosToken.clear();
    m_eosToken.clear();
    m_unkToken.clear();
    m_padToken.clear();

    m_chatTemplateStr.clear();
    m_templateEngine = ChatTemplateEngine::create(ChatTemplateType::ChatML);
    m_modelType = HfModelType::BPE;
    m_byteFallback = false;
    m_addPrefixSpace = false;
}

// ============================================================================
// Hugging Face Loading
// ============================================================================

bool JobTokenizer::loadHf(
    const std::filesystem::path& tokenizerJsonPath,
    const std::filesystem::path& tokenizerConfigJsonPath)
{
    clear();

    HfTokenizerReader reader;
    if (!reader.loadFromFile(tokenizerJsonPath, tokenizerConfigJsonPath)) {
        JOB_LOG_ERROR("JobTokenizer failed to load HuggingFace tokenizer from '{}'", tokenizerJsonPath.string());
        return false;
    }

    return loadHf(reader);
}

bool JobTokenizer::loadHfFromMemory(
    std::string_view tokenizerJson,
    std::string_view tokenizerConfigJson)
{
    clear();

    HfTokenizerReader reader;
    if (!reader.loadFromMemory(tokenizerJson, tokenizerConfigJson)) {
        JOB_LOG_ERROR("JobTokenizer failed to load HuggingFace tokenizer from memory");
        return false;
    }

    return loadHf(reader);
}

bool JobTokenizer::loadHf(const HfTokenizerReader& reader)
{
    clear();

    const auto& data = reader.data();
    m_modelType      = data.modelType;
    m_byteFallback   = data.byteFallback;
    m_addPrefixSpace = data.addPrefixSpace;

    // Vocab
    m_vocab.resize(data.vocab.size());
    for (size_t i = 0; i < data.vocab.size(); ++i) {
        m_vocab[i] = data.vocab[i].first;
        m_tokenToId[m_vocab[i]] = static_cast<int32_t>(i);
    }

    // Merges
    for (size_t i = 0; i < data.merges.size(); ++i) {
        m_mergeRanks[{data.merges[i].first, data.merges[i].second}] = static_cast<int32_t>(i);
    }

    // Added/Special tokens
    for (const auto& tok : data.addedTokens) {
        if (tok.special && tok.id >= 0) {
            m_specialTokenIds.insert(tok.id);
        }
    }

    m_bosToken = data.bosToken;
    m_eosToken = data.eosToken;
    m_unkToken = data.unkToken;
    m_padToken = data.padToken;

    if (auto id = tokenToId(m_bosToken)) m_bosId = *id;
    if (auto id = tokenToId(m_eosToken)) m_eosId = *id;
    if (auto id = tokenToId(m_unkToken)) m_unkId = *id;
    if (auto id = tokenToId(m_padToken)) m_padId = *id;

    if (!data.chatTemplate.empty()) {
        setCustomChatTemplate(data.chatTemplate);
    }

    rebuildLookupStructures();
    JOB_LOG_INFO("JobTokenizer successfully loaded HF model (Vocab: {}, Merges: {})",
                 m_vocab.size(), m_mergeRanks.size());
    return true;
}

// ============================================================================
// GGUF Loading
// ============================================================================

bool JobTokenizer::loadGguf(const std::filesystem::path& ggufPath)
{
    clear();

    GgufTokenizerReader reader;
    if (!reader.loadFromFile(ggufPath)) {
        JOB_LOG_ERROR("JobTokenizer failed to load GGUF from '{}'", ggufPath.string());
        return false;
    }

    return loadGguf(reader);
}

bool JobTokenizer::loadGguf(const ggml::JobGguf& gguf)
{
    clear();

    GgufTokenizerReader reader;
    if (!reader.loadFromGguf(gguf)) {
        JOB_LOG_ERROR("JobTokenizer failed to ingest JobGguf instance");
        return false;
    }

    return loadGguf(reader);
}

bool JobTokenizer::loadGgufFromMemory(const void* data, size_t size)
{
    clear();

    GgufTokenizerReader reader;
    if (!reader.loadFromMemory(data, size)) {
        JOB_LOG_ERROR("JobTokenizer failed to load GGUF from memory buffer");
        return false;
    }

    return loadGguf(reader);
}

bool JobTokenizer::loadGguf(const GgufTokenizerReader& reader)
{
    clear();

    const auto& data = reader.data();
    m_modelType      = data.modelType;
    m_addPrefixSpace = data.addPrefixSpace;
    m_byteFallback   = true;

    m_vocab.resize(data.vocab.size());
    for (size_t i = 0; i < data.vocab.size(); ++i) {
        m_vocab[i] = data.vocab[i].text;
        m_tokenToId[m_vocab[i]] = static_cast<int32_t>(i);
        if (data.vocab[i].type == GgufTokenType::Control || data.vocab[i].type == GgufTokenType::UserDefined) {
            m_specialTokenIds.insert(static_cast<int32_t>(i));
        }
    }

    for (size_t i = 0; i < data.merges.size(); ++i) {
        m_mergeRanks[{data.merges[i].first, data.merges[i].second}] = static_cast<int32_t>(i);
    }

    m_bosId    = data.bosId;
    m_eosId    = data.eosId;
    m_unkId    = data.unkId;
    m_padId    = data.padId;
    m_bosToken = data.bosToken;
    m_eosToken = data.eosToken;
    m_unkToken = data.unkToken;
    m_padToken = data.padToken;

    if (!data.chatTemplate.empty()) {
        setCustomChatTemplate(data.chatTemplate);
    }

    rebuildLookupStructures();
    JOB_LOG_INFO("JobTokenizer loaded GGUF tokenizer (Vocab: {}, Merges: {})", m_vocab.size(), m_mergeRanks.size());
    return true;
}

// ============================================================================
// Binary Loading
// ============================================================================

bool JobTokenizer::loadBinary(const std::filesystem::path& binaryPath)
{
    clear();

    BinaryVocabReader reader;
    if (!reader.loadFromFile(binaryPath)) {
        JOB_LOG_ERROR("JobTokenizer failed to load binary vocab file from '{}'", binaryPath.string());
        return false;
    }

    return loadBinary(reader);
}

bool JobTokenizer::loadBinaryFromMemory(std::span<const uint8_t> buffer)
{
    clear();

    BinaryVocabReader reader;
    if (!reader.loadFromMemory(buffer)) {
        JOB_LOG_ERROR("JobTokenizer failed to load binary vocab buffer");
        return false;
    }

    return loadBinary(reader);
}

bool JobTokenizer::loadBinary(const BinaryVocabReader& reader)
{
    clear();

    const auto& data = reader.data();
    m_modelType      = data.modelType;
    m_byteFallback   = data.byteFallback;
    m_addPrefixSpace = data.addPrefixSpace;

    m_vocab.resize(data.vocab.size());
    for (size_t i = 0; i < data.vocab.size(); ++i) {
        m_vocab[i] = data.vocab[i].content;
        m_tokenToId[m_vocab[i]] = static_cast<int32_t>(i);
        if (data.vocab[i].tokenType == BinaryTokenType::Special || data.vocab[i].tokenType == BinaryTokenType::Control) {
            m_specialTokenIds.insert(static_cast<int32_t>(i));
        }
    }

    for (size_t i = 0; i < data.merges.size(); ++i) {
        m_mergeRanks[{data.merges[i].first, data.merges[i].second}] = static_cast<int32_t>(i);
    }

    m_bosId = data.bosId;
    m_eosId = data.eosId;
    m_unkId = data.unkId;
    m_padId = data.padId;

    if (auto t = idToToken(m_bosId)) m_bosToken = std::string(*t);
    if (auto t = idToToken(m_eosId)) m_eosToken = std::string(*t);
    if (auto t = idToToken(m_unkId)) m_unkToken = std::string(*t);
    if (auto t = idToToken(m_padId)) m_padToken = std::string(*t);

    if (!data.chatTemplate.empty()) {
        setCustomChatTemplate(data.chatTemplate);
    }

    rebuildLookupStructures();
    return true;
}

void JobTokenizer::rebuildLookupStructures()
{
    if (m_bosId >= 0) m_specialTokenIds.insert(m_bosId);
    if (m_eosId >= 0) m_specialTokenIds.insert(m_eosId);
    if (m_unkId >= 0) m_specialTokenIds.insert(m_unkId);
    if (m_padId >= 0) m_specialTokenIds.insert(m_padId);

    m_specialTokenStrings.clear();
    for (int32_t id : m_specialTokenIds) {
        if (auto str = idToToken(id)) {
            if (!str->empty()) {
                m_specialTokenStrings.emplace_back(*str);
            }
        }
    }

    if (!m_bosToken.empty()) m_specialTokenStrings.push_back(m_bosToken);
    if (!m_eosToken.empty()) m_specialTokenStrings.push_back(m_eosToken);
    if (!m_unkToken.empty()) m_specialTokenStrings.push_back(m_unkToken);
    if (!m_padToken.empty()) m_specialTokenStrings.push_back(m_padToken);

    std::sort(m_specialTokenStrings.begin(), m_specialTokenStrings.end());
    m_specialTokenStrings.erase(
        std::unique(m_specialTokenStrings.begin(), m_specialTokenStrings.end()),
        m_specialTokenStrings.end()
        );

    std::sort(m_specialTokenStrings.begin(), m_specialTokenStrings.end(),
              [](const std::string& a, const std::string& b) {
                  return a.size() > b.size();
              });
}

bool JobTokenizer::findFirstSpecialToken(
    std::string_view sv,
    size_t& outPos,
    size_t& outLen,
    int32_t& outId) const noexcept
{
    size_t bestPos = std::string_view::npos;
    size_t bestLen = 0;
    int32_t bestId = -1;

    for (const auto& sp : m_specialTokenStrings) {
        if (sp.empty()) continue;
        size_t pos = sv.find(sp);
        if (pos != std::string_view::npos) {
            if (pos < bestPos || (pos == bestPos && sp.size() > bestLen)) {
                bestPos = pos;
                bestLen = sp.size();
                auto id = tokenToId(sp);
                bestId = id.value_or(-1);
            }
        }
    }

    if (bestPos != std::string_view::npos && bestId != -1) {
        outPos = bestPos;
        outLen = bestLen;
        outId = bestId;
        return true;
    }
    return false;
}

void JobTokenizer::setCustomChatTemplate(std::string jinjaTemplate)
{
    m_chatTemplateStr = std::move(jinjaTemplate);
    m_templateEngine  = ChatTemplateEngine::createCustom(m_chatTemplateStr);
}

std::string JobTokenizer::applyChatTemplate(
    std::span<const ChatMessage> messages,
    bool addGenerationPrompt) const
{
    if (!isLoaded() || !m_templateEngine) {
        return "";
    }
    return m_templateEngine->apply(messages, addGenerationPrompt, m_bosToken, m_eosToken);
}

std::vector<int32_t> JobTokenizer::encodeChat(
    std::span<const ChatMessage> messages,
    bool addGenerationPrompt) const
{
    if (!isLoaded()) {
        return {};
    }
    std::string prompt = applyChatTemplate(messages, addGenerationPrompt);
    return encode(prompt, false, false);
}

std::vector<int32_t> JobTokenizer::encode(
    std::string_view text,
    bool addBos,
    bool addEos) const
{
    std::vector<int32_t> result;
    if (!isLoaded()) {
        JOB_LOG_WARN("JobTokenizer::encode called on uninitialized tokenizer");
        return result;
    }

    if (addBos && m_bosId >= 0) {
        result.push_back(m_bosId);
    }

    std::string_view remaining = text;
    while (!remaining.empty()) {
        size_t pos = 0;
        size_t len = 0;
        int32_t specialId = -1;

        if (findFirstSpecialToken(remaining, pos, len, specialId)) {
            if (pos > 0) {
                tokenizeSegment(remaining.substr(0, pos), result);
            }
            result.push_back(specialId);
            remaining.remove_prefix(pos + len);
        } else {
            tokenizeSegment(remaining, result);
            break;
        }
    }

    if (addEos && m_eosId >= 0) {
        result.push_back(m_eosId);
    }

    return result;
}

void JobTokenizer::tokenizeSegment(std::string_view segment, std::vector<int32_t>& out) const
{
    if (segment.empty()) return;

    size_t start = 0;
    while (start < segment.size()) {
        detail::CharClass cls = detail::classifyChar(static_cast<unsigned char>(segment[start]));
        size_t end = start + 1;

        while (end < segment.size() &&
               detail::classifyChar(static_cast<unsigned char>(segment[end])) == cls) {
            ++end;
        }

        bpeTokenizeChunk(segment.substr(start, end - start), out);
        start = end;
    }
}

void JobTokenizer::emitToken(std::string_view piece, std::vector<int32_t>& out) const
{
    if (auto id = tokenToId(piece)) {
        out.push_back(*id);
    } else if (m_byteFallback) {
        for (unsigned char b : piece) {
            std::string byteStr = detail::formatByteToken(b);
            if (auto byteId = tokenToId(byteStr)) {
                out.push_back(*byteId);
            } else if (m_unkId >= 0) {
                out.push_back(m_unkId);
            }
        }
    } else if (m_unkId >= 0) {
        out.push_back(m_unkId);
    }
}

void JobTokenizer::bpeTokenizeChunk(std::string_view chunk, std::vector<int32_t>& out) const
{
    if (chunk.empty()) return;

    // Fast-path: whole chunk matches a vocab token
    if (auto id = tokenToId(chunk)) {
        out.push_back(*id);
        return;
    }

    // Split chunk into UTF-8 codepoints / bytes
    std::vector<detail::BpeSymbol> symbols;
    symbols.reserve(chunk.size());

    for (size_t i = 0; i < chunk.size(); ) {
        unsigned char c = static_cast<unsigned char>(chunk[i]);
        size_t charLen = 1;
        if ((c & 0x80) == 0x00) charLen = 1;
        else if ((c & 0xE0) == 0xC0) charLen = 2;
        else if ((c & 0xF0) == 0xE0) charLen = 3;
        else if ((c & 0xF8) == 0xF0) charLen = 4;

        if (i + charLen > chunk.size()) charLen = chunk.size() - i;

        int32_t idx = static_cast<int32_t>(symbols.size());
        int32_t prev_idx = idx - 1;
        symbols.push_back(detail::BpeSymbol{
            static_cast<uint32_t>(i),
            static_cast<uint32_t>(charLen),
            prev_idx,
            -1
        });
        if (prev_idx >= 0) {
            symbols[prev_idx].next = idx;
        }
        i += charLen;
    }

    if (symbols.empty()) return;
    if (symbols.size() == 1) {
        emitToken(chunk.substr(symbols[0].start, symbols[0].len), out);
        return;
    }

    // Min-heap tracking of lowest rank adjacent pairs
    std::priority_queue<detail::BpeMergePair, std::vector<detail::BpeMergePair>, std::greater<detail::BpeMergePair>> pq;

    auto checkAndPushPair = [&](int32_t left_idx) {
        if (left_idx < 0 || left_idx >= static_cast<int32_t>(symbols.size())) return;
        int32_t right_idx = symbols[left_idx].next;
        if (right_idx < 0 || right_idx >= static_cast<int32_t>(symbols.size())) return;

        std::string_view l_sv = chunk.substr(symbols[left_idx].start, symbols[left_idx].len);
        std::string_view r_sv = chunk.substr(symbols[right_idx].start, symbols[right_idx].len);

        auto it = m_mergeRanks.find(std::pair{l_sv, r_sv});
        if (it != m_mergeRanks.end()) {
            pq.push(detail::BpeMergePair{
                it->second,
                left_idx,
                symbols[left_idx].len,
                symbols[right_idx].len
            });
        }
    };

    for (size_t i = 0; i < symbols.size() - 1; ++i) {
        checkAndPushPair(static_cast<int32_t>(i));
    }

    while (!pq.empty()) {
        auto top = pq.top();
        pq.pop();

        int32_t left_idx = top.left_idx;
        if (left_idx < 0 || left_idx >= static_cast<int32_t>(symbols.size())) continue;
        auto& left = symbols[left_idx];

        if (left.len != top.left_len) continue;
        int32_t right_idx = left.next;
        if (right_idx < 0 || right_idx >= static_cast<int32_t>(symbols.size())) continue;
        auto& right = symbols[right_idx];
        if (right.len != top.right_len) continue;

        // Perform in-place merge
        left.len += right.len;
        left.next = right.next;
        if (right.next >= 0) {
            symbols[right.next].prev = left_idx;
        }

        if (left.prev >= 0) {
            checkAndPushPair(left.prev);
        }
        if (left.next >= 0) {
            checkAndPushPair(left_idx);
        }
    }

    // Traverse result nodes
    int32_t curr = 0;
    while (curr >= 0 && symbols[curr].prev >= 0) {
        curr = symbols[curr].prev;
    }

    while (curr >= 0) {
        std::string_view piece = chunk.substr(symbols[curr].start, symbols[curr].len);
        emitToken(piece, out);
        curr = symbols[curr].next;
    }
}

std::string JobTokenizer::decode(int32_t tokenId, bool stripSpecialTokens) const
{
    return decode(std::span<const int32_t>(&tokenId, 1), stripSpecialTokens);
}

std::string JobTokenizer::decode(std::span<const int32_t> tokens, bool stripSpecialTokens) const
{
    if (!isLoaded() || tokens.empty()) {
        return "";
    }

    std::string result;
    result.reserve(tokens.size() * 4);

    for (int32_t id : tokens) {
        if (stripSpecialTokens && isSpecialToken(id)) {
            continue;
        }

        auto tokenStr = idToToken(id);
        if (!tokenStr) continue;

        if (tokenStr->starts_with("<0x") && tokenStr->ends_with('>')) {
            if (auto byteVal = detail::parseByteToken(*tokenStr)) {
                result.push_back(static_cast<char>(*byteVal));
                continue;
            }
        }

        for (size_t i = 0; i < tokenStr->size();) {
            if (i + 1 < tokenStr->size() &&
                static_cast<unsigned char>((*tokenStr)[i]) == 0xC4 &&
                static_cast<unsigned char>((*tokenStr)[i + 1]) == 0xA0) {
                result.push_back(' ');
                i += 2;
            } else if (i + 2 < tokenStr->size() &&
                       static_cast<unsigned char>((*tokenStr)[i]) == 0xE2 &&
                       static_cast<unsigned char>((*tokenStr)[i + 1]) == 0x96 &&
                       static_cast<unsigned char>((*tokenStr)[i + 2]) == 0x81) {
                result.push_back(' ');
                i += 3;
            } else {
                result.push_back((*tokenStr)[i]);
                i += 1;
            }
        }
    }

    return result;
}

std::optional<int32_t> JobTokenizer::tokenToId(std::string_view token) const noexcept
{
    auto it = m_tokenToId.find(token);
    if (it != m_tokenToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string_view> JobTokenizer::idToToken(int32_t id) const noexcept
{
    if (id >= 0 && static_cast<size_t>(id) < m_vocab.size()) {
        return m_vocab[static_cast<size_t>(id)];
    }
    return std::nullopt;
}

bool JobTokenizer::isSpecialToken(int32_t id) const noexcept
{
    return m_specialTokenIds.find(id) != m_specialTokenIds.end();
}

} // namespace job::token