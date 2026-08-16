#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "formats/binary_vocab_reader.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {

namespace detail {

class TransientTestFile {
public:
    TransientTestFile(std::string path, const std::vector<uint8_t>& data)
        : m_path(std::move(path))
    {
        std::ofstream stream(m_path, std::ios::binary);
        if (!stream) {
            throw std::runtime_error("Failed to create transient test file");
        }
        if (!data.empty()) {
            stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
    }

    ~TransientTestFile()
    {
        std::remove(m_path.c_str());
    }

    TransientTestFile(const TransientTestFile&) = delete;
    TransientTestFile& operator=(const TransientTestFile&) = delete;

    [[nodiscard]] const std::string& path() const noexcept { return m_path; }

private:
    std::string m_path;
};

inline std::vector<uint8_t> createSyntheticBinaryVocab(
    uint32_t magic = BinaryVocabReader::MAGIC,
    uint32_t version = BinaryVocabReader::CURRENT_VERSION,
    uint8_t modelType = static_cast<uint8_t>(HfModelType::BPE),
    uint8_t flags = 0x01) // 0x01 = byte fallback
{
    std::vector<uint8_t> buffer;

    auto appendRaw = [&buffer](const auto& val) {
        const auto* ptr = reinterpret_cast<const uint8_t*>(&val);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(val));
    };

    uint16_t reserved = 0;
    uint32_t vocabSize = 4;
    uint32_t mergesSize = 1;
    int32_t  bosId = 1;
    int32_t  eosId = 2;
    int32_t  unkId = 0;
    int32_t  padId = 3;
    int32_t  maskId = -1;
    std::string chatTemplate = "{{ bos_token }}{{ messages[0].content }}";
    uint32_t chatTemplateLen = static_cast<uint32_t>(chatTemplate.size());

    // 1. Header
    appendRaw(magic);
    appendRaw(version);
    appendRaw(modelType);
    appendRaw(flags);
    appendRaw(reserved);
    appendRaw(vocabSize);
    appendRaw(mergesSize);
    appendRaw(bosId);
    appendRaw(eosId);
    appendRaw(unkId);
    appendRaw(padId);
    appendRaw(maskId);
    appendRaw(chatTemplateLen);

    // 2. Chat Template
    buffer.insert(buffer.end(), chatTemplate.begin(), chatTemplate.end());

    // 3. Vocab Entries
    auto appendToken = [&](const std::string& str, float score, BinaryTokenType type) {
        uint16_t len = static_cast<uint16_t>(str.size());
        appendRaw(len);
        buffer.insert(buffer.end(), str.begin(), str.end());
        appendRaw(score);
        uint8_t rawType = static_cast<uint8_t>(type);
        appendRaw(rawType);
    };

    appendToken("<unk>", 0.0f, BinaryTokenType::Special); // ID 0
    appendToken("<s>", 0.0f, BinaryTokenType::Special);   // ID 1
    appendToken("</s>", 0.0f, BinaryTokenType::Special);  // ID 2
    appendToken("<pad>", 0.0f, BinaryTokenType::Special); // ID 3

    // 4. Merges
    auto appendMerge = [&](const std::string& left, const std::string& right) {
        uint16_t lLen = static_cast<uint16_t>(left.size());
        appendRaw(lLen);
        buffer.insert(buffer.end(), left.begin(), left.end());
        uint16_t rLen = static_cast<uint16_t>(right.size());
        appendRaw(rLen);
        buffer.insert(buffer.end(), right.begin(), right.end());
    };

    appendMerge("<", "s");

    return buffer;
}

} // namespace detail

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("BinaryVocabReader loads binary format from memory and disk", "[token][formats][binary][example]")
{
    auto buffer = detail::createSyntheticBinaryVocab();

    SECTION("Load directly from memory buffer") {
        BinaryVocabReader reader;
        bool ok = reader.loadFromMemory(buffer);

        REQUIRE(ok);
        CHECK(reader.vocabSize() == 4);
        CHECK(reader.modelType() == HfModelType::BPE);
        CHECK(reader.data().byteFallback == true);
        CHECK(reader.bosId() == 1);
        CHECK(reader.eosId() == 2);
        CHECK(reader.unkId() == 0);
        CHECK(reader.padId() == 3);

        CHECK(reader.chatTemplate() == "{{ bos_token }}{{ messages[0].content }}");

        // Token queries
        CHECK(reader.findTokenId("<s>") == 1);
        CHECK(reader.findTokenId("</s>") == 2);
        CHECK(reader.findTokenString(0) == "<unk>");
        CHECK(reader.findTokenString(3) == "<pad>");

        // Merges verification
        REQUIRE(reader.data().merges.size() == 1);
        CHECK(reader.data().merges[0].first == "<");
        CHECK(reader.data().merges[0].second == "s");
    }

    SECTION("Load from transient file on disk") {
        detail::TransientTestFile tempFile("test_sample.jobv", buffer);

        BinaryVocabReader reader;
        bool ok = reader.loadFromFile(tempFile.path());

        REQUIRE(ok);
        CHECK(reader.vocabSize() == 4);
        CHECK(reader.findTokenId("<s>") == 1);
    }
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("BinaryVocabReader guards invariants against corrupt or invalid payloads", "[token][formats][binary][edge_cases]")
{
    BinaryVocabReader reader;

    SECTION("Invalid magic identifier fails validation") {
        auto badMagicBuffer = detail::createSyntheticBinaryVocab(0xDEADBEEF);
        CHECK_FALSE(reader.loadFromMemory(badMagicBuffer));
        CHECK(reader.vocabSize() == 0);
    }

    SECTION("Unsupported binary version fails validation") {
        auto badVersionBuffer = detail::createSyntheticBinaryVocab(BinaryVocabReader::MAGIC, 999);
        CHECK_FALSE(reader.loadFromMemory(badVersionBuffer));
        CHECK(reader.vocabSize() == 0);
    }

    SECTION("Truncated buffer smaller than header fails validation") {
        std::vector<uint8_t> tinyBuffer = {0x4A, 0x4F, 0x42, 0x56}; // 4 bytes only
        CHECK_FALSE(reader.loadFromMemory(tinyBuffer));
    }

    SECTION("Buffer truncated mid-payload fails validation") {
        auto validBuffer = detail::createSyntheticBinaryVocab();
        validBuffer.resize(validBuffer.size() - 10); // Chop off last token/merge payload

        CHECK_FALSE(reader.loadFromMemory(validBuffer));
        CHECK(reader.vocabSize() == 0);
    }

    SECTION("Non-existent file path returns false") {
        CHECK_FALSE(reader.loadFromFile("/non/existent/path/vocab.jobv"));
    }

    SECTION("Out-of-range token lookups return std::nullopt") {
        auto buffer = detail::createSyntheticBinaryVocab();
        REQUIRE(reader.loadFromMemory(buffer));

        CHECK_FALSE(reader.findTokenId("nonexistent_token").has_value());
        CHECK_FALSE(reader.findTokenString(-1).has_value());
        CHECK_FALSE(reader.findTokenString(9999).has_value());
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark BinaryVocabReader binary deserialization throughput", "[token][formats][binary][benchmark]")
{
    // Generate synthetic 10,000-token binary buffer
    std::vector<uint8_t> largeBuffer;

    auto appendRaw = [&largeBuffer](const auto& val) {
        const auto* ptr = reinterpret_cast<const uint8_t*>(&val);
        largeBuffer.insert(largeBuffer.end(), ptr, ptr + sizeof(val));
    };

    uint32_t magic = BinaryVocabReader::MAGIC;
    uint32_t version = BinaryVocabReader::CURRENT_VERSION;
    uint8_t  modelType = static_cast<uint8_t>(HfModelType::BPE);
    uint8_t  flags = 0x00;
    uint16_t reserved = 0;
    uint32_t vocabSize = 10000;
    uint32_t mergesSize = 2000;
    int32_t  bosId = 0, eosId = 1, unkId = 2, padId = 3, maskId = -1;
    uint32_t chatTmplLen = 0;

    appendRaw(magic); appendRaw(version); appendRaw(modelType); appendRaw(flags);
    appendRaw(reserved); appendRaw(vocabSize); appendRaw(mergesSize);
    appendRaw(bosId); appendRaw(eosId); appendRaw(unkId); appendRaw(padId); appendRaw(maskId);
    appendRaw(chatTmplLen);

    for (uint32_t i = 0; i < vocabSize; ++i) {
        std::string token = "token_" + std::to_string(i);
        uint16_t len = static_cast<uint16_t>(token.size());
        appendRaw(len);
        largeBuffer.insert(largeBuffer.end(), token.begin(), token.end());
        float score = static_cast<float>(i);
        appendRaw(score);
        uint8_t type = static_cast<uint8_t>(BinaryTokenType::Normal);
        appendRaw(type);
    }

    for (uint32_t i = 0; i < mergesSize; ++i) {
        std::string left = "t";
        std::string right = std::to_string(i);
        uint16_t lLen = static_cast<uint16_t>(left.size());
        appendRaw(lLen);
        largeBuffer.insert(largeBuffer.end(), left.begin(), left.end());
        uint16_t rLen = static_cast<uint16_t>(right.size());
        appendRaw(rLen);
        largeBuffer.insert(largeBuffer.end(), right.begin(), right.end());
    }

    BinaryVocabReader reader;

    BENCHMARK("Deserialize 10,000-token binary buffer") {
        return reader.loadFromMemory(largeBuffer);
    };

    BENCHMARK("Lookup 1000 tokens in loaded BinaryVocabReader") {
        int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            auto id = reader.findTokenId("token_" + std::to_string(i));
            if (id) sum += *id;
        }
        return sum;
    };
}
#endif

} // namespace job::token::test