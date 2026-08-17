#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

namespace job::token {

// Standard discrete token identifier
using TokenId = int32_t;
inline constexpr TokenId kInvalidToken = -1;

// Maximum 21-bit coordinate space representation (7 bits per axis: X, Y, Z)
inline constexpr uint32_t kMax21BitTokenId = (1u << 21) - 1u;

// Token encoding options
struct TokenizeOptions {
    bool addBos          = true;
    bool addEos          = false;
    bool parseSpecial    = true;
    float defaultMass    = 1.0f;
};

// Character/byte offset slice
struct TokenSpan {
    size_t byteBegin = 0;
    size_t byteEnd   = 0;

    [[nodiscard]] constexpr size_t length() const noexcept
    {
        return (byteEnd >= byteBegin) ? (byteEnd - byteBegin) : 0;
    }
};

// Detailed encoding result
struct TokenizeResult {
    std::vector<TokenId>   tokens;
    std::vector<TokenSpan> spans;
};

struct StringHash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(std::string_view sv) const noexcept
    {
        return std::hash<std::string_view>{}(sv);
    }
};


struct StringPair {
    std::string left;
    std::string right;
};

struct StringPairHash {
    using is_transparent = void;

    size_t operator()(const StringPair& p) const noexcept
    {
        return hash(p.left, p.right);
    }
    size_t operator()(const std::pair<std::string_view, std::string_view>& p) const noexcept
    {
        return hash(p.first, p.second);
    }

private:
    // okay I have siphash and also all sorts of hash's all over the place lol
    static size_t hash(std::string_view l, std::string_view r) noexcept
    {
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

    bool operator()(const StringPair& a, const StringPair& b) const noexcept
    {
        return a.left == b.left && a.right == b.right;
    }

    bool operator()(const StringPair& a, const std::pair<std::string_view, std::string_view>& b) const noexcept
    {
        return a.left == b.first && a.right == b.second;
    }

    bool operator()(const std::pair<std::string_view, std::string_view>& a, const StringPair& b) const noexcept
    {
        return a.first == b.left && a.second == b.right;
    }
};

struct TransparentStringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept
    {
        return std::hash<std::string_view>{}(sv);
    }
};

struct ByteStringTable {
    char storage[256][7]{};

    constexpr ByteStringTable() noexcept {
        constexpr char hexDigits[] = "0123456789ABCDEF";
        for (size_t i = 0; i < 256; ++i) {
            storage[i][0] = '<';
            storage[i][1] = '0';
            storage[i][2] = 'x';
            storage[i][3] = hexDigits[(i >> 4) & 0x0F];
            storage[i][4] = hexDigits[i & 0x0F];
            storage[i][5] = '>';
            storage[i][6] = '\0';
        }
    }
};
inline constexpr ByteStringTable kByteHexStorage{};

// =================================================================

struct BinaryHeader
{
    // File / table sizes
    std::uint32_t magic{0};
    std::uint32_t vocabSize{0};
    std::uint32_t mergesSize{0};
    std::uint32_t chatTemplateLen{0};

    // Canonical special-token IDs
    std::int32_t bosId{-1};
    std::int32_t eosId{-1};
    std::int32_t eotId{-1};
    std::int32_t padId{-1};
    std::int32_t unkId{-1};
    std::int32_t maskId{-1};
    std::int32_t clsId{-1};
    std::int32_t sepId{-1};
    std::int32_t prefixId{-1};
    std::int32_t suffixId{-1};
    std::int32_t middleId{-1};

    // Small configuration values belong together at the end.
    std::uint8_t version{0};
    std::uint8_t modelType{0};
    std::uint8_t splitPattern{0};
    std::uint8_t flags{0};
};

static_assert(sizeof(BinaryHeader) == 64);
static_assert(std::is_trivially_copyable_v<BinaryHeader>);

} // namespace job::token