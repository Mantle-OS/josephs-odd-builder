#include "formats/binary_vocab_reader.h"
#include "formats/format_utils.h"

#include <cstring>
#include <fstream>
#include <job_logger.h>

namespace job::token {

namespace detail {

#pragma pack(push, 1)
struct BinaryHeader {
    uint32_t magic;
    uint32_t version;
    uint8_t  modelType;
    uint8_t  flags;
    uint16_t reserved;
    uint32_t vocabSize;
    uint32_t mergesSize;
    int32_t  bosId;
    int32_t  eosId;
    int32_t  unkId;
    int32_t  padId;
    int32_t  maskId;
    uint32_t chatTemplateLen;
};
#pragma pack(pop)

class BufferReader {
public:
    explicit BufferReader(std::span<const uint8_t> buffer) noexcept
        : m_buffer(buffer) {}

    [[nodiscard]] bool canRead(size_t bytes) const noexcept {
        return m_cursor + bytes <= m_buffer.size();
    }

    template <typename T>
    [[nodiscard]] bool read(T& out) noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        if (!canRead(sizeof(T))) return false;
        std::memcpy(&out, m_buffer.data() + m_cursor, sizeof(T));
        m_cursor += sizeof(T);
        return true;
    }

    [[nodiscard]] bool readString(std::string& out, size_t length) {
        if (!canRead(length)) return false;
        out.assign(reinterpret_cast<const char*>(m_buffer.data() + m_cursor), length);
        m_cursor += length;
        return true;
    }

    [[nodiscard]] size_t cursor() const noexcept { return m_cursor; }
    [[nodiscard]] size_t size() const noexcept { return m_buffer.size(); }

private:
    std::span<const uint8_t> m_buffer;
    size_t m_cursor{0};
};

} // namespace detail

bool BinaryVocabReader::loadFromFile(const std::filesystem::path& path)
{
    clear();

    if (!std::filesystem::exists(path)) {
        JOB_LOG_ERROR("Binary vocab file does not exist: {}", path.string());
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        JOB_LOG_ERROR("Failed to open binary vocab file: {}", path.string());
        return false;
    }

    const auto fileSize = file.tellg();
    if (fileSize <= 0 || static_cast<size_t>(fileSize) < sizeof(detail::BinaryHeader)) {
        JOB_LOG_ERROR("Binary vocab file is too small or invalid: {}", path.string());
        return false;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        JOB_LOG_ERROR("Failed to read binary vocab file into buffer: {}", path.string());
        return false;
    }

    return loadFromMemory(buffer);
}

bool BinaryVocabReader::loadFromMemory(const void* data, size_t size)
{
    if (!data || size < sizeof(detail::BinaryHeader)) {
        JOB_LOG_ERROR("Invalid pointer or insufficient size passed to loadFromMemory");
        return false;
    }
    return loadFromMemory(std::span<const uint8_t>(static_cast<const uint8_t*>(data), size));
}

bool BinaryVocabReader::loadFromMemory(std::span<const uint8_t> buffer)
{
    clear();

    detail::BufferReader reader(buffer);
    detail::BinaryHeader header{};

    if (!reader.read(header)) {
        JOB_LOG_ERROR("Failed to read binary vocab header");
        return false;
    }

    if (header.magic != MAGIC) {
        JOB_LOG_ERROR("Invalid binary vocab magic: 0x{:X} (expected 0x{:X})", header.magic, MAGIC);
        return false;
    }

    if (header.version != CURRENT_VERSION) {
        JOB_LOG_ERROR("Unsupported binary vocab version: {} (expected {})", header.version, CURRENT_VERSION);
        return false;
    }

    BinaryVocabData data{};
    data.version   = header.version;
    data.modelType = static_cast<HfModelType>(header.modelType);
    data.byteFallback   = (header.flags & 0x01) != 0;
    data.addPrefixSpace = (header.flags & 0x02) != 0;

    data.bosId  = header.bosId;
    data.eosId  = header.eosId;
    data.unkId  = header.unkId;
    data.padId  = header.padId;
    data.maskId = header.maskId;

    // 1. Read Chat Template (if present)
    if (header.chatTemplateLen > 0) {
        if (!reader.readString(data.chatTemplate, header.chatTemplateLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading chat template (length {})", header.chatTemplateLen);
            return false;
        }
    }

    // 2. Read Vocab Entries
    data.vocab.resize(header.vocabSize);
    data.tokenToId.reserve(header.vocabSize);

    for (uint32_t i = 0; i < header.vocabSize; ++i) {
        uint16_t tokenLen = 0;
        if (!reader.read(tokenLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token length at index {}", i);
            return false;
        }

        std::string tokenContent;
        if (!reader.readString(tokenContent, tokenLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token content at index {}", i);
            return false;
        }

        float score = 0.0f;
        if (!reader.read(score)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token score at index {}", i);
            return false;
        }

        uint8_t rawTokenType = 0;
        if (!reader.read(rawTokenType)) {
            JOB_LOG_ERROR("Unexpected EOF while reading token type at index {}", i);
            return false;
        }

        data.tokenToId[tokenContent] = static_cast<int32_t>(i);
        data.vocab[i] = BinaryTokenEntry{
            std::move(tokenContent),
            score,
            static_cast<BinaryTokenType>(rawTokenType)
        };
    }

    // 3. Read BPE Merges
    data.merges.reserve(header.mergesSize);
    for (uint32_t i = 0; i < header.mergesSize; ++i) {
        uint16_t leftLen = 0;
        if (!reader.read(leftLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge left length at index {}", i);
            return false;
        }

        std::string left;
        if (!reader.readString(left, leftLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge left string at index {}", i);
            return false;
        }

        uint16_t rightLen = 0;
        if (!reader.read(rightLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge right length at index {}", i);
            return false;
        }

        std::string right;
        if (!reader.readString(right, rightLen)) {
            JOB_LOG_ERROR("Unexpected EOF while reading merge right string at index {}", i);
            return false;
        }

        data.merges.emplace_back(std::move(left), std::move(right));
    }

    // Commit only on complete parse success
    m_data = std::move(data);

    JOB_LOG_DEBUG("Successfully loaded binary vocab (Vocab: {}, Merges: {}, Model: {})",
                  m_data.vocab.size(),
                  m_data.merges.size(),
                  hfModelTypeToString(m_data.modelType));

    return true;
}

std::optional<int32_t> BinaryVocabReader::findTokenId(std::string_view token) const noexcept
{
    auto it = m_data.tokenToId.find(std::string(token));
    if (it != m_data.tokenToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string_view> BinaryVocabReader::findTokenString(int32_t id) const noexcept
{
    if (id >= 0 && static_cast<size_t>(id) < m_data.vocab.size()) {
        return m_data.vocab[static_cast<size_t>(id)].content;
    }
    return std::nullopt;
}

void BinaryVocabReader::clear() noexcept
{
    m_data = BinaryVocabData{};
}

} // namespace job::token