#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include "job_usb_id_cursor.h"
#include "jobusb_export.h"

namespace job::usb {

class JOBUSB_EXPORT JobUsbIdGrammar
{
public:
    using Ptr  = std::shared_ptr<JobUsbIdGrammar>;
    using WPtr = std::weak_ptr<JobUsbIdGrammar>;
    using UPtr = std::unique_ptr<JobUsbIdGrammar>;


    enum class RecordType : uint8_t
    {
        Blank = 0,          // Empty or whitespace-only line.
        Comment,            // Comment line beginning with '#'.

        Vendor,             // Top-level USB vendor.
        Product,            // Device belonging to the current vendor.
        Interface,          // Interface belonging to the current product.

        Class,              // Top-level USB device/interface class.
        Subclass,           // Subclass belonging to the current class.
        Protocol,           // Protocol belonging to the current subclass.

        AudioTerminal,      // Audio class terminal type.
        HidDescriptor,      // HID descriptor type.
        HidItem,            // HID report descriptor item type.
        Bias,               // Physical descriptor bias type.
        Physical,           // Physical descriptor item type.

        HidUsageTable,      // HID usage page.
        HidUsage,           // Usage belonging to the current HID usage page.

        Language,           // USB language identifier.
        Dialect,            // Dialect belonging to the current language.

        HidCountryCode,     // HID country code.
        VideoTerminal,      // Video class terminal type.

        Unsupported,        // Valid usb.ids syntax not retained by this grammar.
        Invalid             // Malformed or contextually invalid record.
    };

    [[nodiscard]] static constexpr std::string_view toString(RecordType type) noexcept
    {
        switch (type) {
        case RecordType::Blank:
            return "Blank";
        case RecordType::Comment:
            return "Comment";
        case RecordType::Vendor:
            return "Vendor";
        case RecordType::Product:
            return "Product";
        case RecordType::Interface:
            return "Interface";
        case RecordType::Class:
            return "Class";
        case RecordType::Subclass:
            return "Subclass";
        case RecordType::Protocol:
            return "Protocol";
        case RecordType::AudioTerminal:
            return "Audio Terminal";
        case RecordType::HidDescriptor:
            return "Hid Descriptor";
        case RecordType::HidItem:
            return "Hid Item";
        case RecordType::Bias:
            return "Bias";
        case RecordType::Physical:
            return "Physical";
        case RecordType::HidUsageTable:
            return "Hid Usage Table";
        case RecordType::HidUsage:
            return "Hid Usage";
        case RecordType::Language:
            return "Language";
        case RecordType::Dialect:
            return "Dialect";
        case RecordType::HidCountryCode:
            return "Hid Country Code";
        case RecordType::VideoTerminal:
            return "Video Terminal";
        case RecordType::Unsupported:
            return "Unsupported";
        case RecordType::Invalid:
            return "Invalid";
        }

        return "Invalid";
    }
    struct Record
    {
        RecordType          type = RecordType::Invalid;
        uint32_t            id = 0;
        std::string_view    name{};
        std::size_t         lineNumber = 0;
        std::size_t         indentation = 0;
    };

    JobUsbIdGrammar() = default;
    ~JobUsbIdGrammar() = default;

    JobUsbIdGrammar(const JobUsbIdGrammar &) = default;
    JobUsbIdGrammar &operator=(const JobUsbIdGrammar &) = default;
    JobUsbIdGrammar(JobUsbIdGrammar &&) noexcept = default;
    JobUsbIdGrammar &operator=(JobUsbIdGrammar &&) noexcept = default;

    [[nodiscard]] static Ptr  createShared();
    [[nodiscard]] static UPtr createUniq();

    [[nodiscard]] Record parse(const JobUsbIdCursor::Line &line) noexcept;

    void reset() noexcept;

private:
    inline static constexpr std::string_view ClassPrefix            = "C";
    inline static constexpr std::string_view AudioTerminalPrefix    = "AT";
    inline static constexpr std::string_view HidDescriptorPrefix    = "HID";
    inline static constexpr std::string_view HidItemPrefix          = "R";
    inline static constexpr std::string_view BiasPrefix             = "BIAS";
    inline static constexpr std::string_view PhysicalPrefix         = "PHY";
    inline static constexpr std::string_view HidUsageTablePrefix    = "HUT";
    inline static constexpr std::string_view LanguagePrefix         = "L";
    inline static constexpr std::string_view HidCountryCodePrefix   = "HCC";
    inline static constexpr std::string_view VideoTerminalPrefix    = "VT";

    // Tracks the grammatical world that subsequent indented records belong to.
    enum class Section : uint8_t
    {
        None = 0,           // No active top-level section.
        Vendor,             // Vendor -> Product -> Interface hierarchy.
        Class,              // Class -> Subclass -> Protocol hierarchy.
        AudioTerminal,      // Flat Audio Terminal table.
        HidDescriptor,      // Flat HID Descriptor table.
        HidItem,            // Flat HID Item table.
        Bias,               // Flat Physical Bias table.
        Physical,           // Flat Physical Item table.
        HidUsageTable,      // HUT -> HID Usage hierarchy.
        Language,           // Language -> Dialect hierarchy.
        HidCountryCode,     // Flat HID Country Code table.
        VideoTerminal,      // Flat Video Terminal table.
        Unsupported         // Active but unsupported top-level section.
    };

    [[nodiscard]] Record parseTopLevel(const JobUsbIdCursor::Line &line) noexcept;
    [[nodiscard]] Record parseIndented(const JobUsbIdCursor::Line &line) noexcept;

    Section     m_section           = Section::None;
    bool        m_hasIndentedParent = false;

};

} // namespace job::usb