#include "job_usb_id_grammar.h"

#include "job_usb_id_line_kernel.h"

namespace job::usb {

JobUsbIdGrammar::Ptr JobUsbIdGrammar::createShared()
{
    return std::make_shared<JobUsbIdGrammar>();
}

JobUsbIdGrammar::UPtr JobUsbIdGrammar::createUniq()
{
    return std::make_unique<JobUsbIdGrammar>();
}

JobUsbIdGrammar::Record JobUsbIdGrammar::parse(const JobUsbIdCursor::Line &line) noexcept
{
    if (JobUsbIdLineKernel::blank(line.raw)) {
        return {
            .type = RecordType::Blank,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    }

    if (JobUsbIdLineKernel::comment(line.raw)) {
        return {
            .type = RecordType::Comment,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    }

    if (line.indentation > 2) {
        return {
            .type = RecordType::Invalid,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    }

    if (line.indentation == 0)
        return parseTopLevel(line);

    return parseIndented(line);
}

void JobUsbIdGrammar::reset() noexcept
{
    m_section = Section::None;
    m_hasIndentedParent = false;
}

JobUsbIdGrammar::Record JobUsbIdGrammar::parseTopLevel(const JobUsbIdCursor::Line &line) noexcept
{
    JobUsbIdLineKernel::Field field;
    if (!JobUsbIdLineKernel::splitField(line.text, field)) {
        return {
            .type = RecordType::Invalid,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    }

    const auto makeRecord = [&](RecordType type, uint32_t id, std::string_view name) {
        return Record {
            .type = type,
            .id = id,
            .name = name,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    };

    const auto invalid = [&] {
        return makeRecord(RecordType::Invalid, 0, {});
    };

    const auto parsePrefixed = [&](std::size_t width, RecordType type, Section section) -> Record {
        JobUsbIdLineKernel::Field value;

        if (!JobUsbIdLineKernel::splitField(field.value, value))
            return invalid();

        if (value.key.size() != width || value.value.empty())
            return invalid();

        uint32_t id = 0;

        if (!JobUsbIdLineKernel::parseHex(value.key, id))
            return invalid();

        m_section = section;
        m_hasIndentedParent = false;

        return makeRecord(type, id, value.value);
    };

    uint32_t vendorId = 0;

    if (field.key.size() == 4 && !field.value.empty() && JobUsbIdLineKernel::parseHex(field.key, vendorId)) {
        m_section = Section::Vendor;
        m_hasIndentedParent = false;

        return makeRecord(RecordType::Vendor, vendorId, field.value);
    }

    if (field.key == ClassPrefix)
        return parsePrefixed(2, RecordType::Class, Section::Class);

    if (field.key == AudioTerminalPrefix)
        return parsePrefixed(4, RecordType::AudioTerminal, Section::AudioTerminal);

    if (field.key == HidDescriptorPrefix)
        return parsePrefixed(2, RecordType::HidDescriptor, Section::HidDescriptor);

    if (field.key == HidItemPrefix)
        return parsePrefixed(2, RecordType::HidItem, Section::HidItem);

    if (field.key == BiasPrefix)
        return parsePrefixed(1, RecordType::Bias, Section::Bias);

    if (field.key == PhysicalPrefix)
        return parsePrefixed(2, RecordType::Physical, Section::Physical);

    if (field.key == HidUsageTablePrefix)
        return parsePrefixed(2, RecordType::HidUsageTable, Section::HidUsageTable);

    if (field.key == LanguagePrefix)
        return parsePrefixed(4, RecordType::Language, Section::Language);

    if (field.key == HidCountryCodePrefix)
        return parsePrefixed(2, RecordType::HidCountryCode, Section::HidCountryCode);

    if (field.key == VideoTerminalPrefix)
        return parsePrefixed(4, RecordType::VideoTerminal, Section::VideoTerminal);

    if (!field.value.empty()) {
        m_section = Section::Unsupported;
        m_hasIndentedParent = false;

        return makeRecord(RecordType::Unsupported, 0, field.value);
    }

    return invalid();
}

JobUsbIdGrammar::Record JobUsbIdGrammar::parseIndented(const JobUsbIdCursor::Line &line) noexcept
{
    JobUsbIdLineKernel::Field field;

    if (!JobUsbIdLineKernel::splitField(line.text, field)) {
        return {
            .type = RecordType::Invalid,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    }

    const auto makeRecord = [&](RecordType type, uint32_t id, std::string_view name) {
        return Record{
            .type = type,
            .id = id,
            .name = name,
            .lineNumber = line.number,
            .indentation = line.indentation
        };
    };

    const auto invalid = [&] {
        return makeRecord(RecordType::Invalid, 0, {});
    };

    const auto parseId = [&](std::size_t width, RecordType type) -> Record {
        if (field.key.size() != width || field.value.empty())
            return invalid();

        uint32_t id = 0;

        if (!JobUsbIdLineKernel::parseHex(field.key, id))
            return invalid();

        return makeRecord(type, id, field.value);
    };

    switch (m_section) {
    case Section::Vendor:
        if (line.indentation == 1) {
            auto record = parseId(4, RecordType::Product);

            if (record.type == RecordType::Product)
                m_hasIndentedParent = true;

            return record;
        }

        if (line.indentation == 2 && m_hasIndentedParent)
            return parseId(4, RecordType::Interface);

        return invalid();

    case Section::Class:
        if (line.indentation == 1) {
            auto record = parseId(2, RecordType::Subclass);

            if (record.type == RecordType::Subclass)
                m_hasIndentedParent = true;

            return record;
        }

        if (line.indentation == 2 && m_hasIndentedParent)
            return parseId(2, RecordType::Protocol);

        return invalid();

    case Section::HidUsageTable:
        if (line.indentation != 1)
            return invalid();

        return parseId(3, RecordType::HidUsage);

    case Section::Language:
        if (line.indentation != 1)
            return invalid();

        return parseId(2, RecordType::Dialect);

    case Section::Unsupported:
        return makeRecord(RecordType::Unsupported, 0, field.value);

    case Section::None:
    case Section::AudioTerminal:
    case Section::HidDescriptor:
    case Section::HidItem:
    case Section::Bias:
    case Section::Physical:
    case Section::HidCountryCode:
    case Section::VideoTerminal:
        return invalid();
    }

    return invalid();
}

} // namespace job::usb