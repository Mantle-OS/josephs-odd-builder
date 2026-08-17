#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include <filesystem>
#include <string>

#include "itoken.h"
#include "job_token_enums.h"
#include "jobtoken_export.h"

namespace job::token {

enum class BinaryTokenType : std::uint8_t {
    Normal = 0,
    Special,
    Control,
    Byte,
    Unused
};

class JOBTOKEN_EXPORT BinaryToken final : public IToken
{
public:
    using Ptr  = std::shared_ptr<BinaryToken>;
    using WPtr = std::weak_ptr<BinaryToken>;
    using UPtr = std::unique_ptr<BinaryToken>;

    using Merges = std::vector<std::pair<std::string, std::string>>;

    static constexpr std::uint32_t MAGIC = 0x4A4F4256; // 'J' 'O' 'B' 'V'
    static constexpr std::uint8_t CURRENT_VERSION = 2;

    BinaryToken()
    {
        setProvider(Provider::Binary);
    }

    ~BinaryToken() override = default;

    BinaryToken(const BinaryToken &) = delete;
    BinaryToken &operator=(const BinaryToken &) = delete;
    BinaryToken(BinaryToken &&) = delete;
    BinaryToken &operator=(BinaryToken &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<BinaryToken>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<BinaryToken>();
    }

    [[nodiscard]] bool load(const std::filesystem::path &path);
    [[nodiscard]] bool load(const void *data, std::size_t size);
    [[nodiscard]] bool load(std::span<const std::uint8_t> buffer);

    [[nodiscard]] std::uint8_t version() const noexcept
    {
        return m_version;
    }

    [[nodiscard]] const Merges &merges() const noexcept
    {
        return m_merges;
    }

protected:
    void extraClear() noexcept override
    {
        setProvider(Provider::Binary);

        m_version = 0;
        m_merges.clear();
    }

private:
    class BufferReader
    {
    public:
        explicit BufferReader(std::span<const std::uint8_t> buffer) noexcept :
            m_buffer{buffer}
        {

        }

        [[nodiscard]] bool canRead(std::size_t bytes) const noexcept
        {
            return m_cursor <= m_buffer.size() && bytes <= m_buffer.size() - m_cursor;
        }

        template<typename T>
        [[nodiscard]] bool read(T &out) noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>);

            if (!canRead(sizeof(T)))
                return false;

            std::memcpy(&out, m_buffer.data() + m_cursor, sizeof(T));
            m_cursor += sizeof(T);
            return true;
        }

        [[nodiscard]] bool readString(std::string &out, std::size_t length)
        {
            if (!canRead(length))
                return false;

            out.assign(reinterpret_cast<const char *>(m_buffer.data() + m_cursor), length);
            m_cursor += length;
            return true;
        }

    private:
        std::span<const std::uint8_t> m_buffer;
        std::size_t m_cursor{0};
    };

    [[nodiscard]] static constexpr StructuralType mapTokenType(BinaryTokenType type) noexcept
    {
        switch (type) {
        case BinaryTokenType::Normal:
            return StructuralType::Normal;
        case BinaryTokenType::Special:
            return StructuralType::UserDefined;
        case BinaryTokenType::Control:
            return StructuralType::Control;
        case BinaryTokenType::Byte:
            return StructuralType::Byte;
        case BinaryTokenType::Unused:
            return StructuralType::Unused;
        default:
            return StructuralType::Normal;
        }
    }

    std::uint8_t m_version{0};
    Merges m_merges;
};

} // namespace job::token