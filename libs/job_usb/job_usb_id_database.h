#pragma once

#include <cstdint>
#include <filesystem>
#include <flat_map>
#include <string>
#include <string_view>
#include <cstddef>

#include <job_arena_pool.h>

#include "jobusb_export.h"

namespace job::usb {

enum class UsbIdLoadState : uint8_t
{
    Uninitialized = 0,      // Database has not been loaded.
    Io,                     // Opening/reading the source database.
    Parsing,                // Cursor/grammar processing.
    UnsupportedSection,     // Recognized unsupported section; non-terminal.
    Finished,               // Finalized, immutable, query-ready.
    Error                   // Terminal load failure.
};

[[nodiscard]] constexpr std::string_view toString(UsbIdLoadState state) noexcept
{
    switch (state) {
    case UsbIdLoadState::Uninitialized:
        return "Uninitialized";
    case UsbIdLoadState::Io:
        return "Io";
    case UsbIdLoadState::Parsing:
        return "Parsing";
    case UsbIdLoadState::UnsupportedSection:
        return "Unsupported Section";
    case UsbIdLoadState::Finished:
        return "Finished";
    case UsbIdLoadState::Error:
        return "Error";
    }

    return "Error";
}

enum class UsbDbTables: uint8_t
{
    Vendor = 0,
    Product,
    Class,
    Subclass,
    Protocol,
    All
};


class JOBUSB_EXPORT JobUsbIdDatabase
{
public:
    inline static constexpr std::size_t kMaxDatabaseSize = 8 * 1024 * 1024;
    using VendorMap     = std::flat_map<uint16_t, std::string_view>;
    using ProductMap    = std::flat_map<uint32_t, std::string_view>;
    using ClassMap      = std::flat_map<uint8_t,  std::string_view>;
    using SubclassMap   = std::flat_map<uint16_t, std::string_view>;
    using ProtocolMap   = std::flat_map<uint32_t, std::string_view>;
    using ConstPath     = const std::filesystem::path &;

    ~JobUsbIdDatabase() = default;

    JobUsbIdDatabase(const JobUsbIdDatabase &) = delete;
    JobUsbIdDatabase &operator=(const JobUsbIdDatabase &) = delete;
    JobUsbIdDatabase(JobUsbIdDatabase &&) = delete;
    JobUsbIdDatabase &operator=(JobUsbIdDatabase &&) = delete;

    [[nodiscard]] static JobUsbIdDatabase &instance() noexcept;

    [[nodiscard]] bool load(ConstPath filePath = "/usr/share/hwdata/usb.ids", bool useMmap = true);

    [[nodiscard]] std::string_view vendorName(uint16_t vendorId) const noexcept;
    [[nodiscard]] std::string_view productName(uint16_t vendorId, uint16_t productId) const noexcept;

    [[nodiscard]] std::string_view className(uint8_t classId) const noexcept;
    [[nodiscard]] std::string_view subclassName(uint8_t classId, uint8_t subclassId) const noexcept;
    [[nodiscard]] std::string_view protocolName(uint8_t classId, uint8_t subclassId, uint8_t protocolId) const noexcept;

    [[nodiscard]] UsbIdLoadState state() const noexcept;
    [[nodiscard]] std::string_view lastError() const noexcept;
    [[nodiscard]] bool ready() const noexcept;


    void clear() noexcept
    {
        m_vendors.clear();
        m_products.clear();
        m_classes.clear();
        m_subclasses.clear();
        m_protocols.clear();

        m_arena.reset();
        m_lastError.clear();
        m_state = UsbIdLoadState::Uninitialized;
    }

private:
    JobUsbIdDatabase() = default;
    [[nodiscard]] bool loadFile(ConstPath filePath);
    [[nodiscard]] bool loadMmap(ConstPath filePath);
    [[nodiscard]] bool parse(std::string_view source);
    [[nodiscard]] std::string_view storeString(std::string_view value);

    [[nodiscard]] static constexpr uint32_t productKey(uint16_t vendorId, uint16_t productId) noexcept
    {
        return (static_cast<uint32_t>(vendorId) << 16) | productId;
    }

    [[nodiscard]] static constexpr uint16_t subclassKey(uint8_t classId, uint8_t subclassId) noexcept
    {
        return static_cast<uint16_t>((static_cast<uint16_t>(classId) << 8) | subclassId);
    }

    [[nodiscard]] static constexpr uint32_t protocolKey(uint8_t classId, uint8_t subclassId, uint8_t protocolId) noexcept
    {
        return (static_cast<uint32_t>(classId) << 16) |
               (static_cast<uint32_t>(subclassId) << 8) |
               protocolId;
    }

    io::JobArenaPool::Ptr   m_arena;
    VendorMap               m_vendors;
    ProductMap              m_products;
    ClassMap                m_classes;
    SubclassMap             m_subclasses;
    ProtocolMap             m_protocols;
    std::string             m_lastError;
    UsbIdLoadState          m_state{UsbIdLoadState::Uninitialized};
};

} // namespace job::usb