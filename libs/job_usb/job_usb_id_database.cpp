#include "job_usb_id_database.h"

#include <cstring>
#include <string>
#include <utility>

#include <job_file.h>
#include <job_mmap.h>

#include "job_usb_id_cursor.h"
#include "job_usb_id_grammar.h"

namespace job::usb {

JobUsbIdDatabase &JobUsbIdDatabase::instance() noexcept
{
    static JobUsbIdDatabase database;
    return database;
}

bool JobUsbIdDatabase::load(ConstPath &filePath, bool useMmap)
{
    if (m_state == UsbIdLoadState::Finished)
        return true;

    if (m_state != UsbIdLoadState::Uninitialized)
        return false;

    m_state = UsbIdLoadState::Io;
    m_lastError.clear();

    const bool success = useMmap ?
                             loadMmap(filePath) :
                             loadFile(filePath);

    if (!success && m_state != UsbIdLoadState::Error) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "Failed to load usb.ids";
    }

    return success;
}

bool JobUsbIdDatabase::loadFile(ConstPath &filePath)
{
    io::JobFile file{filePath};

    if (!file.openDevice()) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "Failed to open usb.ids";
        return false;
    }

    const std::size_t fileSize = file.size();

    if (fileSize == 0) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids is empty";
        return false;
    }

    if (fileSize > kMaxDatabaseSize) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids exceeds maximum supported size";
        return false;
    }

    std::string source;

    if (file.readAll(source) < 0) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "Failed to read usb.ids";
        return false;
    }

    if (source.size() != fileSize) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids changed while being read";
        return false;
    }

    return parse(source);
}

bool JobUsbIdDatabase::loadMmap(ConstPath &filePath)
{
    io::JobMmap mmap{filePath};

    if (!mmap.isValid()) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "Failed to map usb.ids";
        return false;
    }

    const std::size_t sourceSize = mmap.mapLength();

    if (sourceSize == 0) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids is empty";
        return false;
    }

    if (sourceSize > kMaxDatabaseSize) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids exceeds maximum supported size";
        return false;
    }

    const std::string_view source {
        static_cast<const char *>(mmap.addr()),
        sourceSize
    };

    return parse(source);
}

bool JobUsbIdDatabase::parse(std::string_view source)
{
    if (source.empty()) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids is empty";
        return false;
    }

    if (source.size() > kMaxDatabaseSize) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "usb.ids exceeds maximum supported size";
        return false;
    }

    m_arena = io::JobArenaPool::createShared(source.size());

    if (!m_arena || m_arena->size() != source.size()) {
        m_state = UsbIdLoadState::Error;
        m_lastError = "Failed to create usb.ids string arena";
        return false;
    }

    m_vendors.clear();
    m_products.clear();
    m_classes.clear();
    m_subclasses.clear();
    m_protocols.clear();

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    std::uint16_t currentVendor = 0;
    std::uint8_t currentClass = 0;
    std::uint8_t currentSubclass = 0;

    bool haveVendor = false;
    bool haveClass = false;
    bool haveSubclass = false;

    m_state = UsbIdLoadState::Parsing;

    while (!cursor.atEnd()) {
        const JobUsbIdGrammar::Record record = grammar.parse(cursor.line());

        switch (record.type) {
        case JobUsbIdGrammar::RecordType::Blank:
        case JobUsbIdGrammar::RecordType::Comment:
            break;

        case JobUsbIdGrammar::RecordType::Vendor: {
            const std::string_view name = storeString(record.name);

            if (!record.name.empty() && name.empty()) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids string arena exhausted";
                return false;
            }

            currentVendor = static_cast<std::uint16_t>(record.id);
            haveVendor = true;

            m_vendors.insert_or_assign(currentVendor, name);
            break;
        }

        case JobUsbIdGrammar::RecordType::Product: {
            if (!haveVendor) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids product encountered without vendor";
                return false;
            }

            const std::string_view name = storeString(record.name);

            if (!record.name.empty() && name.empty()) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids string arena exhausted";
                return false;
            }

            const auto id = static_cast<std::uint16_t>(record.id);
            m_products.insert_or_assign(productKey(currentVendor, id), name);
            break;
        }

        case JobUsbIdGrammar::RecordType::Interface:
            break;

        case JobUsbIdGrammar::RecordType::Class: {
            const std::string_view name = storeString(record.name);

            if (!record.name.empty() && name.empty()) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids string arena exhausted";
                return false;
            }

            currentClass = static_cast<std::uint8_t>(record.id);
            currentSubclass = 0;

            haveClass = true;
            haveSubclass = false;

            m_classes.insert_or_assign(currentClass, name);
            break;
        }

        case JobUsbIdGrammar::RecordType::Subclass: {
            if (!haveClass) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids subclass encountered without class";
                return false;
            }

            const std::string_view name = storeString(record.name);
            if (!record.name.empty() && name.empty()) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids string arena exhausted";
                return false;
            }

            currentSubclass = static_cast<std::uint8_t>(record.id);
            haveSubclass = true;

            m_subclasses.insert_or_assign(subclassKey(currentClass, currentSubclass), name);
            break;
        }

        case JobUsbIdGrammar::RecordType::Protocol: {
            if (!haveClass || !haveSubclass) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids protocol encountered without subclass";
                return false;
            }

            const std::string_view name = storeString(record.name);

            if (!record.name.empty() && name.empty()) {
                m_state = UsbIdLoadState::Error;
                m_lastError = "usb.ids string arena exhausted";
                return false;
            }

            const auto id = static_cast<std::uint8_t>(record.id);
            m_protocols.insert_or_assign(protocolKey(currentClass, currentSubclass, id), name);

            break;
        }

        case JobUsbIdGrammar::RecordType::Unsupported:
            m_state = UsbIdLoadState::UnsupportedSection;
            break;

        case JobUsbIdGrammar::RecordType::AudioTerminal:
        case JobUsbIdGrammar::RecordType::HidDescriptor:
        case JobUsbIdGrammar::RecordType::HidItem:
        case JobUsbIdGrammar::RecordType::Bias:
        case JobUsbIdGrammar::RecordType::Physical:
        case JobUsbIdGrammar::RecordType::HidUsageTable:
        case JobUsbIdGrammar::RecordType::HidUsage:
        case JobUsbIdGrammar::RecordType::Language:
        case JobUsbIdGrammar::RecordType::Dialect:
        case JobUsbIdGrammar::RecordType::HidCountryCode:
        case JobUsbIdGrammar::RecordType::VideoTerminal:
            break;

        case JobUsbIdGrammar::RecordType::Invalid:
            m_state = UsbIdLoadState::Error;
            m_lastError =
                "Invalid usb.ids record at line " +
                std::to_string(record.lineNumber);
            return false;
        }

        if (m_state == UsbIdLoadState::UnsupportedSection)
            m_state = UsbIdLoadState::Parsing;

        if (!cursor.advance())
            break;
    }

    m_state = UsbIdLoadState::Finished;
    return true;
}

std::string_view JobUsbIdDatabase::storeString(std::string_view value)
{
    if (value.empty())
        return {};

    auto *destination = static_cast<char *>(m_arena->alloc(value.size(), alignof(char)));

    if (destination == nullptr)
        return {};

    std::memcpy(destination, value.data(), value.size());

    return std::string_view{
        destination,
        value.size()
    };
}

std::string_view JobUsbIdDatabase::vendorName(std::uint16_t vendorId) const noexcept
{
    const auto it = m_vendors.find(vendorId);
    if (it == m_vendors.end())
        return {};

    return it->second;
}

std::string_view JobUsbIdDatabase::productName(std::uint16_t vendorId, std::uint16_t productId) const noexcept
{
    const auto it = m_products.find(productKey(vendorId, productId));

    if (it == m_products.end())
        return {};

    return it->second;
}

std::string_view JobUsbIdDatabase::className(std::uint8_t classId) const noexcept
{
    const auto it = m_classes.find(classId);

    if (it == m_classes.end())
        return {};

    return it->second;
}

std::string_view JobUsbIdDatabase::subclassName(
    std::uint8_t classId,
    std::uint8_t subclassId) const noexcept
{
    const auto it = m_subclasses.find(subclassKey(classId, subclassId));
    if (it == m_subclasses.end())
        return {};

    return it->second;
}

std::string_view JobUsbIdDatabase::protocolName(std::uint8_t classId, std::uint8_t subclassId, std::uint8_t protocolId) const noexcept
{
    const auto it = m_protocols.find(protocolKey(classId, subclassId, protocolId));
    if (it == m_protocols.end())
        return {};

    return it->second;
}

UsbIdLoadState JobUsbIdDatabase::state() const noexcept
{
    return m_state;
}

std::string_view JobUsbIdDatabase::lastError() const noexcept
{
    return m_lastError;
}

bool JobUsbIdDatabase::ready() const noexcept
{
    return m_state == UsbIdLoadState::Finished;
}

} // namespace job::usb