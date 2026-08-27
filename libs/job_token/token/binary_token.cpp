#include "binary_token.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <job_logger.h>

namespace job::token {

bool BinaryToken::load(const std::filesystem::path &path)
{
    clear();

    if (!std::filesystem::exists(path)) {
        JOB_LOG_ERROR("Binary tokenizer file does not exist: {}", path.string());
        return false;
    }

    const std::string data = readFile(path);
    if (data.size() < sizeof(BinaryHeader)) {
        JOB_LOG_ERROR("Binary tokenizer file is too small or invalid: {}", path.string());
        return false;
    }

    return load(
        data.data(),
        data.size());
}

bool BinaryToken::load(const void *data, std::size_t size)
{
    if (!data || size < sizeof(BinaryHeader)) {
        JOB_LOG_ERROR(
            "Invalid pointer or insufficient size passed to "
            "BinaryToken::load");

        return false;
    }

    return load(std::span<const std::uint8_t>{static_cast<const std::uint8_t *>(data), size});
}

bool BinaryToken::load(std::span<const std::uint8_t> buffer)
{
    clear();

    BufferReader reader{buffer};
    BinaryHeader header{};

    if (!reader.read(header)) {
        JOB_LOG_ERROR("Failed to read binary tokenizer header");
        return false;
    }

    if (header.magic != MAGIC) {
        JOB_LOG_ERROR(
            "Invalid binary tokenizer magic: 0x{:X} "
            "(expected 0x{:X})",
            header.magic,
            MAGIC);

        return false;
    }

    if (header.version != CURRENT_VERSION) {
        JOB_LOG_ERROR(
            "Unsupported binary tokenizer version: {} "
            "(expected {})",
            header.version,
            CURRENT_VERSION);

        return false;
    }

    // Parse into temporary state.
    // Nothing is committed to IToken/Vocab until the entire binary payload has been successfully consumed.
    struct ParsedToken
    {
        std::string text;
        float score{0.0f};
        StructuralType type{StructuralType::Normal};
    };

    std::string chatTemplate;

    if (header.chatTemplateLen > 0) {
        if (!reader.readString(chatTemplate, header.chatTemplateLen)) {
            JOB_LOG_ERROR(
                "Unexpected EOF while reading chat template "
                "(length {})",
                header.chatTemplateLen);

            return false;
        }
    }

    std::vector<ParsedToken> parsedTokens;
    parsedTokens.reserve(header.vocabSize);
    for (std::uint32_t i = 0; i < header.vocabSize; ++i) {
        std::uint16_t tokenLength{0};
        if (!reader.read(tokenLength)) {
            JOB_LOG_ERROR(
                "Unexpected EOF while reading token length "
                "at index {}",
                i);

            return false;
        }

        std::string text;

        if (!reader.readString(text, tokenLength)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token content at index {}", i);
            return false;
        }

        float score{0.0f};
        if (!reader.read(score)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token score at index {}", i);
            return false;
        }

        std::uint8_t rawType{0};
        if (!reader.read(rawType)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token type at index {}", i);
            return false;
        }

        parsedTokens.emplace_back(ParsedToken{
            std::move(text),
            score,
            mapTokenType( static_cast<BinaryTokenType>(rawType))
        });
    }

    Merges parsedMerges;
    parsedMerges.reserve(header.mergesSize);

    for (std::uint32_t i = 0; i < header.mergesSize; ++i) {
        std::uint16_t leftLength{0};
        if (!reader.read(leftLength)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge left length at index {}", i);
            return false;
        }

        std::string left;

        if (!reader.readString(left, leftLength)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge left string at index {}", i);
            return false;
        }

        std::uint16_t rightLength{0};
        if (!reader.read(rightLength)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge right length at index {}", i);
            return false;
        }

        std::string right;
        if (!reader.readString(right, rightLength)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge right string at index {}", i);
            return false;
        }
        parsedMerges.emplace_back(std::move(left), std::move(right));
    }

    // Header configuration
    const TokenType tokenType = static_cast<TokenType>(header.modelType);
    const SplitPattern splitPattern = static_cast<SplitPattern>(header.splitPattern);

    constexpr std::uint8_t kByteFallbackFlag   = 1u << 0;
    constexpr std::uint8_t kAddPrefixSpaceFlag = 1u << 1;
    constexpr std::uint8_t kAddBosTokenFlag    = 1u << 2;
    constexpr std::uint8_t kAddEosTokenFlag    = 1u << 3;

    const bool byteFallback     = (header.flags & kByteFallbackFlag) != 0;
    const bool addPrefixSpace   = (header.flags & kAddPrefixSpaceFlag) != 0;
    const bool addBosToken      = (header.flags & kAddBosTokenFlag) != 0;
    const bool addEosToken      = (header.flags & kAddEosTokenFlag) != 0;


    // Commit canonical tokenizer state
    m_version = header.version;
    m_merges = std::move(parsedMerges);

    setTokenType(tokenType);
    setSplitPattern(splitPattern);
    setByteFallback(byteFallback);
    setAddPrefixSpace(addPrefixSpace);
    setAddBosToken(addBosToken);
    setAddEosToken(addEosToken);
    setChatTemplate(std::move(chatTemplate));
    for (std::size_t i = 0; i < parsedTokens.size(); ++i) {
        ParsedToken &token = parsedTokens[i];
        vocab()->setToken(static_cast<TokenId>(i), std::move(token.text), token.score);
        records()[i].setType(token.type);
    }

    // Canonical special-token identities
    const auto tokenId = [this](std::int32_t id) noexcept -> TokenId {
        if (id < 0)
            return kInvalidToken;

        const TokenId value = static_cast<TokenId>(id);
        if (static_cast<std::size_t>(value) >= vocabSize())
            return kInvalidToken;

        return value;
    };

    SpecialTokens *special = specialTokens();
    if (special) {
        special->setBosId(tokenId(header.bosId));
        special->setEosId(tokenId(header.eosId));
        special->setEotId(tokenId(header.eotId));
        special->setPadId(tokenId(header.padId));
        special->setUnkId(tokenId(header.unkId));
        special->setMaskId(tokenId(header.maskId));
        special->setClsId(tokenId(header.clsId));
        special->setSepId(tokenId(header.sepId));
        special->setPrefixId(tokenId(header.prefixId));
        special->setSuffixId(tokenId(header.suffixId));
        special->setMiddleId(tokenId(header.middleId));
    }

#ifndef NDEBUG
    JOB_LOG_DEBUG(
        "Loaded binary tokenizer "
        "(Version: {}, Type: {}, Vocab: {}, Merges: {}, ChatTemplate: {})",
        m_version,
        static_cast<std::uint32_t>(tokenType),
        vocabSize(),
        m_merges.size(),
        !chatTemplate.empty()
            ? "yes"
            : "no");
#endif

    return vocabSize() > 0;
}

} // namespace job::token