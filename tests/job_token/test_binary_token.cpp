#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <job_token_enums.h>
#include <job_token_types.h>
#include <token/binary_token.h>
#include <vocab/vocab.h>

#include "../transient_test_file.h"

using job::token::BinaryHeader;
using job::token::BinaryToken;
using job::token::BinaryTokenType;
using job::token::IToken;
using job::token::SplitPattern;
using job::token::StructuralType;
using job::token::TokenId;
using job::token::TokenType;
using job::token::kInvalidToken;

namespace job::token {

class BinaryTokenTestData
{
public:
    struct Token
    {
        std::string text;
        float score{0.0f};
        BinaryTokenType type{BinaryTokenType::Normal};
    };

    using Merge = std::pair<std::string, std::string>;

    [[nodiscard]] static std::vector<std::uint8_t> make(
        BinaryHeader header,
        std::string_view chatTemplate,
        const std::vector<Token> &tokens,
        const std::vector<Merge> &merges)
    {
        header.chatTemplateLen = static_cast<std::uint32_t>(chatTemplate.size());
        header.vocabSize = static_cast<std::uint32_t>(tokens.size());
        header.mergesSize = static_cast<std::uint32_t>(merges.size());

        std::vector<std::uint8_t> output;
        output.reserve(sizeof(BinaryHeader) + chatTemplate.size() + tokens.size() * 16 + merges.size() * 16);

        append(output, header);
        appendBytes(output, chatTemplate.data(), chatTemplate.size());

        for (const Token &token : tokens) {
            const auto length = static_cast<std::uint16_t>(token.text.size());
            append(output, length);
            appendBytes(output, token.text.data(), token.text.size());
            append(output, token.score);
            append(output, static_cast<std::uint8_t>(token.type));
        }

        for (const Merge &merge : merges) {
            const auto leftLength = static_cast<std::uint16_t>(merge.first.size());
            append(output, leftLength);
            appendBytes(output, merge.first.data(), merge.first.size());

            const auto rightLength = static_cast<std::uint16_t>(merge.second.size());
            append(output, rightLength);
            appendBytes(output, merge.second.data(), merge.second.size());
        }

        return output;
    }

    [[nodiscard]] static BinaryHeader validHeader() noexcept
    {
        BinaryHeader header{};
        header.magic = BinaryToken::MAGIC;
        header.version = BinaryToken::CURRENT_VERSION;
        header.modelType = static_cast<std::uint8_t>(TokenType::BPE);
        header.splitPattern = static_cast<std::uint8_t>(SplitPattern::GPT2);
        return header;
    }

private:
    template<typename T>
    static void append(std::vector<std::uint8_t> &output, const T &value)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        const std::size_t offset = output.size();
        output.resize(offset + sizeof(T));
        std::memcpy(output.data() + offset, &value, sizeof(T));
    }

    static void appendBytes(std::vector<std::uint8_t> &output, const void *data, std::size_t size)
    {
        if (size == 0)
            return;

        const std::size_t offset = output.size();
        output.resize(offset + size);
        std::memcpy(output.data() + offset, data, size);
    }
};

} // namespace job::token

using job::token::BinaryTokenTestData;

//
// Block 1: usage / examples
//

TEST_CASE("BinaryToken starts as Binary provider", "[token][binary][usage]")
{
    BinaryToken token;

    REQUIRE(token.provider() == IToken::Provider::Binary);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.version() == 0);
    REQUIRE(token.vocab() != nullptr);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
}

TEST_CASE("BinaryToken loads a minimal tokenizer from byte span", "[token][binary][usage][span]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();

    const std::vector<BinaryTokenTestData::Token> tokens = {
        {"a", -1.0f, BinaryTokenType::Normal},
        {"b", -2.0f, BinaryTokenType::Normal},
        {"ab", -0.5f, BinaryTokenType::Normal}
    };
    const std::vector<BinaryTokenTestData::Merge> merges = {{"a", "b"}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, merges);

    BinaryToken token;

    REQUIRE(token.load(std::span<const std::uint8_t>{data}));
    REQUIRE(token.provider() == IToken::Provider::Binary);
    REQUIRE(token.version() == BinaryToken::CURRENT_VERSION);
    REQUIRE(token.tokenType() == TokenType::BPE);
    REQUIRE(token.splitPattern() == SplitPattern::GPT2);
    REQUIRE(token.vocabSize() == 3);
    REQUIRE(token.vocab()->tokenText(0) == "a");
    REQUIRE(token.vocab()->tokenText(1) == "b");
    REQUIRE(token.vocab()->tokenText(2) == "ab");
    REQUIRE(token.vocab()->tokenScore(0) == -1.0f);
    REQUIRE(token.vocab()->tokenScore(1) == -2.0f);
    REQUIRE(token.vocab()->tokenScore(2) == -0.5f);
    REQUIRE(token.merges().size() == 1);
    REQUIRE(token.merges()[0].first == "a");
    REQUIRE(token.merges()[0].second == "b");
}

TEST_CASE("BinaryToken loads from raw memory buffer", "[token][binary][usage][memory]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 1.25f, BinaryTokenType::Normal}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data.data(), data.size()));
    REQUIRE(token.vocabSize() == 1);
    REQUIRE(token.vocab()->tokenText(0) == "hello");
    REQUIRE(token.vocab()->tokenScore(0) == 1.25f);
}

TEST_CASE("BinaryToken loads from file", "[token][binary][usage][io]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {
        {"hello", 0.0f, BinaryTokenType::Normal},
        {"world", -1.0f, BinaryTokenType::Normal}
    };
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    const std::vector<std::byte> fileData(
        reinterpret_cast<const std::byte *>(data.data()),
        reinterpret_cast<const std::byte *>(data.data() + data.size()));

    TransientTestFile file{"test_binary_token.jobv", fileData};

    BinaryToken token;

    REQUIRE(token.load(std::filesystem::path{file.path()}));
    REQUIRE(token.vocabSize() == 2);
    REQUIRE(token.vocab()->tokenText(0) == "hello");
    REQUIRE(token.vocab()->tokenText(1) == "world");
}

TEST_CASE("BinaryToken loads chat template", "[token][binary][usage][chat]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 0.0f, BinaryTokenType::Normal}};
    const std::string chatTemplate = "{{ messages }}";
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, chatTemplate, tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.chatTemplate() == chatTemplate);
}

TEST_CASE("BinaryToken loads tokenizer flags", "[token][binary][usage][flags]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.flags = static_cast<std::uint8_t>((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3));

    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 0.0f, BinaryTokenType::Normal}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.byteFallback());
    REQUIRE(token.addPrefixSpace());
    REQUIRE(token.addBosToken());
    REQUIRE(token.addEosToken());
}

TEST_CASE("BinaryToken leaves tokenizer flags disabled when header flags are clear", "[token][binary][usage][flags]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.flags = 0;

    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 0.0f, BinaryTokenType::Normal}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE_FALSE(token.addPrefixSpace());
    REQUIRE_FALSE(token.addBosToken());
    REQUIRE_FALSE(token.addEosToken());
}

TEST_CASE("BinaryToken maps binary token structural types", "[token][binary][usage][type]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {
        {"normal", 0.0f, BinaryTokenType::Normal},
        {"special", 0.0f, BinaryTokenType::Special},
        {"control", 0.0f, BinaryTokenType::Control},
        {"byte", 0.0f, BinaryTokenType::Byte},
        {"unused", 0.0f, BinaryTokenType::Unused}
    };
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.vocab()->record(0)->type() == StructuralType::Normal);
    REQUIRE(token.vocab()->record(1)->type() == StructuralType::UserDefined);
    REQUIRE(token.vocab()->record(2)->type() == StructuralType::Control);
    REQUIRE(token.vocab()->record(3)->type() == StructuralType::Byte);
    REQUIRE(token.vocab()->record(4)->type() == StructuralType::Unused);
    REQUIRE(token.vocab()->record(1)->isSpecial());
    REQUIRE(token.vocab()->record(2)->isSpecial());
    REQUIRE(token.vocab()->record(3)->isByte());
    REQUIRE(token.vocab()->record(4)->isUnused());
}

TEST_CASE("BinaryToken loads canonical special token IDs", "[token][binary][usage][special]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.bosId = 0;
    header.eosId = 1;
    header.eotId = 2;
    header.padId = 3;
    header.unkId = 4;
    header.maskId = 5;
    header.clsId = 6;
    header.sepId = 7;
    header.prefixId = 8;
    header.suffixId = 9;
    header.middleId = 10;

    const std::vector<BinaryTokenTestData::Token> tokens = {
        {"bos", 0.0f, BinaryTokenType::Control},
        {"eos", 0.0f, BinaryTokenType::Control},
        {"eot", 0.0f, BinaryTokenType::Control},
        {"pad", 0.0f, BinaryTokenType::Special},
        {"unk", 0.0f, BinaryTokenType::Special},
        {"mask", 0.0f, BinaryTokenType::Special},
        {"cls", 0.0f, BinaryTokenType::Special},
        {"sep", 0.0f, BinaryTokenType::Special},
        {"prefix", 0.0f, BinaryTokenType::Special},
        {"suffix", 0.0f, BinaryTokenType::Special},
        {"middle", 0.0f, BinaryTokenType::Special}
    };
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.specialTokens() != nullptr);
    REQUIRE(token.specialTokens()->bosId() == 0);
    REQUIRE(token.specialTokens()->eosId() == 1);
    REQUIRE(token.specialTokens()->eotId() == 2);
    REQUIRE(token.specialTokens()->padId() == 3);
    REQUIRE(token.specialTokens()->unkId() == 4);
    REQUIRE(token.specialTokens()->maskId() == 5);
    REQUIRE(token.specialTokens()->clsId() == 6);
    REQUIRE(token.specialTokens()->sepId() == 7);
    REQUIRE(token.specialTokens()->prefixId() == 8);
    REQUIRE(token.specialTokens()->suffixId() == 9);
    REQUIRE(token.specialTokens()->middleId() == 10);
}

TEST_CASE("BinaryToken rejects special token IDs outside vocabulary", "[token][binary][usage][special][edge]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.bosId = 0;
    header.eosId = 99;
    header.unkId = -1;

    const std::vector<BinaryTokenTestData::Token> tokens = {{"bos", 0.0f, BinaryTokenType::Control}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.specialTokens()->bosId() == 0);
    REQUIRE(token.specialTokens()->eosId() == kInvalidToken);
    REQUIRE(token.specialTokens()->unkId() == kInvalidToken);
}

TEST_CASE("BinaryToken preserves multiple merge rules in order", "[token][binary][usage][merges]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"a", 0.0f, BinaryTokenType::Normal}};
    const std::vector<BinaryTokenTestData::Merge> merges = {{"a", "b"}, {"ab", "c"}, {"abc", "d"}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, merges);

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.merges().size() == 3);
    REQUIRE(token.merges()[0].first == "a");
    REQUIRE(token.merges()[0].second == "b");
    REQUIRE(token.merges()[1].first == "ab");
    REQUIRE(token.merges()[1].second == "c");
    REQUIRE(token.merges()[2].first == "abc");
    REQUIRE(token.merges()[2].second == "d");
}

TEST_CASE("BinaryToken supports WordPiece model metadata", "[token][binary][usage][metadata]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.modelType = static_cast<std::uint8_t>(TokenType::WordPiece);
    header.splitPattern = static_cast<std::uint8_t>(SplitPattern::None);

    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 0.0f, BinaryTokenType::Normal}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    BinaryToken token;

    REQUIRE(token.load(data));
    REQUIRE(token.tokenType() == TokenType::WordPiece);
    REQUIRE(token.splitPattern() == SplitPattern::None);
}

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("BinaryToken rejects missing binary tokenizer file", "[token][binary][edge][io]")
{
    BinaryToken token;

    REQUIRE_FALSE(token.load(std::filesystem::path{"i_am_euler_can_you_point_me_in_the_right_direction.json"}));
}

TEST_CASE("BinaryToken rejects null memory pointer", "[token][binary][edge][memory]")
{
    BinaryToken token;

    REQUIRE_FALSE(token.load(nullptr, sizeof(BinaryHeader)));
}

TEST_CASE("BinaryToken rejects memory smaller than header", "[token][binary][edge][memory]")
{
    std::array<std::uint8_t, 8> data{};

    BinaryToken token;

    REQUIRE_FALSE(token.load(data.data(), data.size()));
}

TEST_CASE("BinaryToken rejects empty byte span", "[token][binary][edge][span]")
{
    const std::span<const std::uint8_t> data;

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects invalid magic", "[token][binary][edge][header]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.magic = 0x344B5252;

    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", {}, {});

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects unsupported version", "[token][binary][edge][header]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.version = static_cast<std::uint8_t>(BinaryToken::CURRENT_VERSION + 1);

    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", {}, {});

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated chat template", "[token][binary][edge][truncated][chat]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.chatTemplateLen = 128;
    header.vocabSize = 1;

    std::vector<std::uint8_t> data(sizeof(BinaryHeader));
    std::memcpy(data.data(), &header, sizeof(header));
    data.push_back(static_cast<std::uint8_t>('x'));

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated token length", "[token][binary][edge][truncated][vocab]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.vocabSize = 1;

    std::vector<std::uint8_t> data(sizeof(BinaryHeader));
    std::memcpy(data.data(), &header, sizeof(header));
    data.push_back(0x01);

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated token content", "[token][binary][edge][truncated][vocab]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"abcdef", 1.0f, BinaryTokenType::Normal}};

    std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    REQUIRE(data.size() > sizeof(BinaryHeader));

    data.resize(sizeof(BinaryHeader) + sizeof(std::uint16_t) + 2);

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated token score", "[token][binary][edge][truncated][vocab]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 1.0f, BinaryTokenType::Normal}};

    std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});
    data.resize(sizeof(BinaryHeader) + sizeof(std::uint16_t) + 5 + 2);

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated token type", "[token][binary][edge][truncated][vocab]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 1.0f, BinaryTokenType::Normal}};

    std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    REQUIRE_FALSE(data.empty());

    data.pop_back();

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated merge left length", "[token][binary][edge][truncated][merge]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.mergesSize = 1;

    const std::vector<BinaryTokenTestData::Token> tokens = {{"a", 0.0f, BinaryTokenType::Normal}};

    std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, {});

    //
    // make() derives mergesSize from the supplied merge collection,
    // so patch the serialized header afterward for this malformed payload.
    //
    header.mergesSize = 1;
    std::memcpy(data.data(), &header, sizeof(header));
    data.push_back(0x01);

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken rejects truncated merge content", "[token][binary][edge][truncated][merge]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {{"a", 0.0f, BinaryTokenType::Normal}};
    const std::vector<BinaryTokenTestData::Merge> merges = {{"left", "right"}};

    std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "", tokens, merges);

    REQUIRE(data.size() > 4);

    data.resize(data.size() - 4);

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
}

TEST_CASE("BinaryToken failed span load clears previous tokenizer state", "[token][binary][edge][state][failure]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.flags = static_cast<std::uint8_t>((1u << 0) | (1u << 2));

    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 0.0f, BinaryTokenType::Normal}};
    const std::vector<std::uint8_t> valid = BinaryTokenTestData::make(header, "CHAT", tokens, {{"h", "e"}});

    BinaryToken token;

    REQUIRE(token.load(valid));
    REQUIRE(token.vocabSize() == 1);
    REQUIRE(token.byteFallback());
    REQUIRE(token.addBosToken());
    REQUIRE_FALSE(token.chatTemplate().empty());
    REQUIRE_FALSE(token.merges().empty());

    const std::array<std::uint8_t, 4> invalid = {0x52, 0x4b, 0x34, 0x21};

    REQUIRE_FALSE(token.load(std::span<const std::uint8_t>{invalid}));

    REQUIRE(token.provider() == IToken::Provider::Binary);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.version() == 0);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
    REQUIRE(token.chatTemplate().empty());
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE_FALSE(token.addPrefixSpace());
    REQUIRE_FALSE(token.addBosToken());
    REQUIRE_FALSE(token.addEosToken());
}

TEST_CASE("BinaryToken parse failure does not partially commit token data", "[token][binary][edge][state][atomic]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    const std::vector<BinaryTokenTestData::Token> tokens = {
        {"first", 1.0f, BinaryTokenType::Normal},
        {"second", 2.0f, BinaryTokenType::Normal}
    };

    std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "CHAT", tokens, {{"first", "second"}});

    REQUIRE(data.size() > 1);

    data.pop_back();

    BinaryToken token;

    REQUIRE_FALSE(token.load(data));
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
    REQUIRE(token.chatTemplate().empty());
    REQUIRE(token.version() == 0);
    REQUIRE(token.tokenType() == TokenType::Unknown);
}

TEST_CASE("BinaryToken clear restores Binary defaults", "[token][binary][edge][state]")
{
    BinaryHeader header = BinaryTokenTestData::validHeader();
    header.flags = static_cast<std::uint8_t>((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3));

    const std::vector<BinaryTokenTestData::Token> tokens = {{"hello", 0.0f, BinaryTokenType::Normal}};
    const std::vector<std::uint8_t> data = BinaryTokenTestData::make(header, "CHAT", tokens, {{"h", "e"}});

    BinaryToken token;

    REQUIRE(token.load(data));

    token.clear();

    REQUIRE(token.provider() == IToken::Provider::Binary);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.version() == 0);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
    REQUIRE(token.splitPattern() == SplitPattern::None);
    REQUIRE(token.customSplitPattern().empty());
    REQUIRE(token.chatTemplate().empty());
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE_FALSE(token.addPrefixSpace());
    REQUIRE_FALSE(token.addBosToken());
    REQUIRE_FALSE(token.addEosToken());
}