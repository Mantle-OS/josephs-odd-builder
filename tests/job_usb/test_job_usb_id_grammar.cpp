#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>

#include <job_usb_id_cursor.h>
#include <job_usb_id_grammar.h>

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdGrammar identifies blank and comment records", "[job_usb][id][grammar][usage]")
{
    constexpr std::string_view source =
        "\n"
        "# USB IDs\n";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Blank);
        REQUIRE(record.id == 0);
        REQUIRE(record.name.empty());
        REQUIRE(record.lineNumber == 1);
        REQUIRE(record.indentation == 0);
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Comment);
        REQUIRE(record.id == 0);
        REQUIRE(record.name.empty());
        REQUIRE(record.lineNumber == 2);
        REQUIRE(record.indentation == 0);
    }
}

TEST_CASE("JobUsbIdGrammar parses a vendor hierarchy", "[job_usb][id][grammar][usage][vendor]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver\n"
        "\t\t0001  Receiver Interface";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Vendor);
        REQUIRE(record.id == 0x046d);
        REQUIRE(record.name == "Logitech, Inc.");
        REQUIRE(record.lineNumber == 1);
        REQUIRE(record.indentation == 0);
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Product);
        REQUIRE(record.id == 0xc534);
        REQUIRE(record.name == "Unifying Receiver");
        REQUIRE(record.lineNumber == 2);
        REQUIRE(record.indentation == 1);
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Interface);
        REQUIRE(record.id == 0x0001);
        REQUIRE(record.name == "Receiver Interface");
        REQUIRE(record.lineNumber == 3);
        REQUIRE(record.indentation == 2);
    }
}

TEST_CASE("JobUsbIdGrammar parses a class hierarchy", "[job_usb][id][grammar][usage][class]")
{
    constexpr std::string_view source =
        "C 03  Human Interface Device\n"
        "\t01  Boot Interface Subclass\n"
        "\t\t01  Keyboard";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Class);
        REQUIRE(record.id == 0x03);
        REQUIRE(record.name == "Human Interface Device");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Subclass);
        REQUIRE(record.id == 0x01);
        REQUIRE(record.name == "Boot Interface Subclass");
        REQUIRE(record.indentation == 1);
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Protocol);
        REQUIRE(record.id == 0x01);
        REQUIRE(record.name == "Keyboard");
        REQUIRE(record.indentation == 2);
    }
}

TEST_CASE("JobUsbIdGrammar parses HID usage hierarchy", "[job_usb][id][grammar][usage][hut]")
{
    constexpr std::string_view source =
        "HUT 01  Generic Desktop Controls\n"
        "\t001  Pointer";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::HidUsageTable);
        REQUIRE(record.id == 0x01);
        REQUIRE(record.name == "Generic Desktop Controls");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::HidUsage);
        REQUIRE(record.id == 0x001);
        REQUIRE(record.name == "Pointer");
        REQUIRE(record.indentation == 1);
    }
}

TEST_CASE("JobUsbIdGrammar parses language hierarchy", "[job_usb][id][grammar][usage][language]")
{
    constexpr std::string_view source =
        "L 0409  English (United States)\n"
        "\t01  United States";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Language);
        REQUIRE(record.id == 0x0409);
        REQUIRE(record.name == "English (United States)");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());

        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Dialect);
        REQUIRE(record.id == 0x01);
        REQUIRE(record.name == "United States");
    }
}

TEST_CASE("JobUsbIdGrammar parses flat usb.ids sections", "[job_usb][id][grammar][usage][sections]")
{
    constexpr std::string_view source =
        "AT 0101  USB Streaming\n"
        "HID 21  HID Descriptor\n"
        "R 80  Input\n"
        "BIAS 0  Not Applicable\n"
        "PHY 00  None\n"
        "HCC 00  Not Supported\n"
        "VT 0101  USB Streaming";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::AudioTerminal);
        REQUIRE(record.id == 0x0101);
        REQUIRE(record.name == "USB Streaming");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::HidDescriptor);
        REQUIRE(record.id == 0x21);
        REQUIRE(record.name == "HID Descriptor");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::HidItem);
        REQUIRE(record.id == 0x80);
        REQUIRE(record.name == "Input");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Bias);
        REQUIRE(record.id == 0x0);
        REQUIRE(record.name == "Not Applicable");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::Physical);
        REQUIRE(record.id == 0x00);
        REQUIRE(record.name == "None");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::HidCountryCode);
        REQUIRE(record.id == 0x00);
        REQUIRE(record.name == "Not Supported");
    }

    REQUIRE(cursor.advance());

    {
        const auto record = grammar.parse(cursor.line());
        REQUIRE(record.type == JobUsbIdGrammar::RecordType::VideoTerminal);
        REQUIRE(record.id == 0x0101);
        REQUIRE(record.name == "USB Streaming");
    }
}

TEST_CASE("JobUsbIdGrammar factories create initialized grammars", "[job_usb][id][grammar][usage][factory]")
{
    const auto shared = JobUsbIdGrammar::createShared();
    const auto unique = JobUsbIdGrammar::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    constexpr std::string_view source = "046d  Logitech, Inc.";
    JobUsbIdCursor cursor{source};

    REQUIRE(shared->parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Vendor);
    REQUIRE(unique->parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Vendor);
}

//////////////////////////////////////////////////////////
// Block 2: State / context
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdGrammar preserves vendor context across blank and comment lines", "[job_usb][id][grammar][state][vendor]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\n"
        "# receiver\n"
        "\tc534  Unifying Receiver";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Vendor);

    REQUIRE(cursor.advance());
    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Blank);

    REQUIRE(cursor.advance());
    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Comment);

    REQUIRE(cursor.advance());

    const auto product = grammar.parse(cursor.line());

    REQUIRE(product.type == JobUsbIdGrammar::RecordType::Product);
    REQUIRE(product.id == 0xc534);
    REQUIRE(product.name == "Unifying Receiver");
}

TEST_CASE("JobUsbIdGrammar requires a product before an interface", "[job_usb][id][grammar][edge][context]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\t\t0001  Receiver Interface";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Vendor);

    REQUIRE(cursor.advance());

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Invalid);
    REQUIRE(record.lineNumber == 2);
    REQUIRE(record.indentation == 2);
}

TEST_CASE("JobUsbIdGrammar requires a subclass before a protocol", "[job_usb][id][grammar][edge][context]")
{
    constexpr std::string_view source =
        "C 03  Human Interface Device\n"
        "\t\t01  Keyboard";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Class);

    REQUIRE(cursor.advance());

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Invalid);
    REQUIRE(record.lineNumber == 2);
    REQUIRE(record.indentation == 2);
}

TEST_CASE("JobUsbIdGrammar rejects indented records without an active section", "[job_usb][id][grammar][edge][context]")
{
    constexpr std::string_view source = "\t1234  Orphan";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Invalid);
    REQUIRE(record.id == 0);
    REQUIRE(record.name.empty());
}

TEST_CASE("JobUsbIdGrammar reset removes active grammatical context", "[job_usb][id][grammar][state][reset]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Vendor);

    grammar.reset();

    REQUIRE(cursor.advance());

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Invalid);
}

//////////////////////////////////////////////////////////
// Block 3: Unsupported / invalid input
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdGrammar reports unknown top level sections as unsupported", "[job_usb][id][grammar][unsupported]")
{
    constexpr std::string_view source =
        "FUTURE 01  Something New";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Unsupported);
    REQUIRE(record.id == 0);
    REQUIRE(record.name == "01  Something New");
    REQUIRE(record.lineNumber == 1);
    REQUIRE(record.indentation == 0);
}

TEST_CASE("JobUsbIdGrammar keeps indented records in an unsupported section unsupported", "[job_usb][id][grammar][unsupported]")
{
    constexpr std::string_view source =
        "FUTURE 01  Something New\n"
        "\t02  Child Entry";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Unsupported);

    REQUIRE(cursor.advance());

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Unsupported);
    REQUIRE(record.id == 0);
    REQUIRE(record.name == "Child Entry");
    REQUIRE(record.indentation == 1);
}

TEST_CASE("JobUsbIdGrammar rejects indentation deeper than two levels", "[job_usb][id][grammar][edge][indentation]")
{
    constexpr std::string_view source =
        "\t\t\t0001  Too Deep";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    const auto record = grammar.parse(cursor.line());

    REQUIRE(record.type == JobUsbIdGrammar::RecordType::Invalid);
    REQUIRE(record.lineNumber == 1);
    REQUIRE(record.indentation == 3);
}

TEST_CASE("JobUsbIdGrammar rejects malformed vendor records", "[job_usb][id][grammar][edge][vendor]")
{
    {
        constexpr std::string_view source = "046d";
        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
    }

    {
        constexpr std::string_view source = "12345  Too Wide";
        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Unsupported);
    }
}

TEST_CASE("JobUsbIdGrammar rejects malformed known prefixed records", "[job_usb][id][grammar][edge][prefix]")
{
    {
        constexpr std::string_view source = "C 003  Wrong Width";
        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
    }

    {
        constexpr std::string_view source = "HUT zz  Bad Hex";
        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
    }

    {
        constexpr std::string_view source = "AT 0101";
        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
    }
}

TEST_CASE("JobUsbIdGrammar rejects indentation under flat sections", "[job_usb][id][grammar][edge][section]")
{
    constexpr std::string_view source =
        "HID 21  HID Descriptor\n"
        "\t01  Not Allowed";

    JobUsbIdCursor cursor{source};
    JobUsbIdGrammar grammar;

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::HidDescriptor);

    REQUIRE(cursor.advance());

    REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
}

TEST_CASE("JobUsbIdGrammar validates child identifier widths", "[job_usb][id][grammar][edge][width]")
{
    {
        constexpr std::string_view source =
            "046d  Logitech, Inc.\n"
            "\tc53  Too Short";

        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Vendor);
        REQUIRE(cursor.advance());
        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
    }

    {
        constexpr std::string_view source =
            "HUT 01  Generic Desktop Controls\n"
            "\t01  Too Short";

        JobUsbIdCursor cursor{source};
        JobUsbIdGrammar grammar;

        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::HidUsageTable);
        REQUIRE(cursor.advance());
        REQUIRE(grammar.parse(cursor.line()).type == JobUsbIdGrammar::RecordType::Invalid);
    }
}

//////////////////////////////////////////////////////////
// Block 4: RecordType presentation
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdGrammar converts record types to names", "[job_usb][id][grammar][to_string]")
{
    using RecordType = JobUsbIdGrammar::RecordType;

    REQUIRE(JobUsbIdGrammar::toString(RecordType::Blank)            == "Blank");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Comment)          == "Comment");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Vendor)           == "Vendor");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Product)          == "Product");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Interface)        == "Interface");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Class)            == "Class");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Subclass)         == "Subclass");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Protocol)         == "Protocol");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::AudioTerminal)    == "Audio Terminal");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::HidDescriptor)    == "Hid Descriptor");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::HidItem)          == "Hid Item");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Bias)             == "Bias");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Physical)         == "Physical");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::HidUsageTable)    == "Hid Usage Table");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::HidUsage)         == "Hid Usage");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Language)         == "Language");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Dialect)          == "Dialect");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::HidCountryCode)   == "Hid Country Code");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::VideoTerminal)    == "Video Terminal");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Unsupported)      == "Unsupported");
    REQUIRE(JobUsbIdGrammar::toString(RecordType::Invalid)          == "Invalid");
}

} // namespace job::usb::tests