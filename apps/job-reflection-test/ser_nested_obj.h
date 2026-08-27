#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "obj.h"
#include "obj_concept.h"

enum class SomeEnum : std::uint8_t {
    Foo,
    Bar,
    Car
};

class SerNestedObj : public Object
{
public:
    using Ptr = std::shared_ptr<SerNestedObj>;

    SerNestedObj() = default;
    ~SerNestedObj() = default;

    [[nodiscard]] const std::string &name() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }

    [[nodiscard]] std::int32_t id() const noexcept { return m_id; }
    void setId(std::int32_t id) noexcept { m_id = id; }

    [[nodiscard]] float weight() const noexcept { return m_weight; }
    void setWeight(float weight) noexcept { m_weight = weight; }

    [[nodiscard]] bool enabled() const noexcept { return m_enabled; }
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }

    [[nodiscard]] std::int8_t int8() const noexcept { return m_int8; }
    void setInt8(std::int8_t value) noexcept { m_int8 = value; }

    [[nodiscard]] std::int16_t int16() const noexcept { return m_int16; }
    void setInt16(std::int16_t value) noexcept { m_int16 = value; }

    [[nodiscard]] std::int64_t id64() const noexcept { return m_id64; }
    void setId64(std::int64_t value) noexcept { m_id64 = value; }

    [[nodiscard]] std::uint8_t uint8() const noexcept { return m_uint8; }
    void setUint8(std::uint8_t value) noexcept { m_uint8 = value; }

    [[nodiscard]] std::uint16_t uint16() const noexcept { return m_uint16; }
    void setUint16(std::uint16_t value) noexcept { m_uint16 = value; }

    [[nodiscard]] std::uint32_t uid() const noexcept { return m_uid_field; }
    void setUid(std::uint32_t value) noexcept { m_uid_field = value; }

    [[nodiscard]] std::uint64_t uid64() const noexcept { return m_uid64; }
    void setUid64(std::uint64_t value) noexcept { m_uid64 = value; }

    [[nodiscard]] double dweight() const noexcept { return m_dweight; }
    void setDweight(double value) noexcept { m_dweight = value; }

    [[nodiscard]] int inter() const noexcept { return m_inter; }
    void setInter(int value) noexcept { m_inter = value; }

    [[nodiscard]] SomeEnum someEnum() const noexcept { return m_someEnum; }
    void setSomeEnum(SomeEnum value) noexcept { m_someEnum = value; }

    std::string m_name;

    std::int8_t  m_int8{0};
    std::int16_t m_int16{0};
    std::int32_t m_id{0};
    std::int64_t m_id64{0};

    std::uint8_t  m_uint8{0};
    std::uint16_t m_uint16{0};
    std::uint32_t m_uid_field{0};
    std::uint64_t m_uid64{0};

    short              m_short{0};
    unsigned short     m_ushort{0};
    int                m_inter{-1};
    unsigned int       m_uint{0};
    long               m_long{0};
    unsigned long      m_ulong{0};
    long long          m_longLong{0};
    unsigned long long m_ulongLong{0};

    char          m_char{'A'};
    signed char   m_signedChar{-1};
    unsigned char m_unsignedChar{1};
    wchar_t       m_wchar{L'Z'};
    char8_t       m_char8{u8'Q'};
    char16_t      m_char16{u'R'};
    char32_t      m_char32{U'S'};

    float       m_weight{0.0f};
    double      m_dweight{0.0};
    long double m_longDouble{0.0L};

    bool m_enabled{false};

    std::byte m_byte{std::byte{0x2A}};

    SomeEnum m_someEnum{SomeEnum::Bar};
};

static_assert(ObjectType<SerNestedObj>);