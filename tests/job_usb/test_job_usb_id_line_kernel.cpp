#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <job_usb_id_line_kernel.h>

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdLineKernel parses hexadecimal values", "[job_usb][id][line_kernel][usage][hex]")
{
    std::uint8_t value8 = 0;
    std::uint16_t value16 = 0;
    std::uint32_t value32 = 0;

    REQUIRE(JobUsbIdLineKernel::parseHex("ff", value8));
    REQUIRE(value8 == 0xff);

    REQUIRE(JobUsbIdLineKernel::parseHex("046d", value16));
    REQUIRE(value16 == 0x046d);

    REQUIRE(JobUsbIdLineKernel::parseHex("1234abcd", value32));
    REQUIRE(value32 == 0x1234abcd);
}

TEST_CASE("JobUsbIdLineKernel counts structural indentation", "[job_usb][id][line_kernel][usage][indentation]")
{
    REQUIRE(JobUsbIdLineKernel::indentation("046d Logitech") == 0);
    REQUIRE(JobUsbIdLineKernel::indentation("\tc534 Unifying Receiver") == 1);
    REQUIRE(JobUsbIdLineKernel::indentation("\t\t0001 Interface") == 2);
    REQUIRE(JobUsbIdLineKernel::indentation("\t\t\tvalue") == 3);
}

TEST_CASE("JobUsbIdLineKernel removes structural indentation", "[job_usb][id][line_kernel][usage][indentation]")
{
    REQUIRE(JobUsbIdLineKernel::removeIndentation("046d Logitech") == "046d Logitech");
    REQUIRE(JobUsbIdLineKernel::removeIndentation("\tc534 Unifying Receiver") == "c534 Unifying Receiver");
    REQUIRE(JobUsbIdLineKernel::removeIndentation("\t\t0001 Interface") == "0001 Interface");
}

TEST_CASE("JobUsbIdLineKernel trims whitespace", "[job_usb][id][line_kernel][usage][trim]")
{
    REQUIRE(JobUsbIdLineKernel::trimLeft(" \t hello") == "hello");
    REQUIRE(JobUsbIdLineKernel::trimRight("hello \t\r\n") == "hello");
    REQUIRE(JobUsbIdLineKernel::trim(" \t hello \r\n") == "hello");

    REQUIRE(JobUsbIdLineKernel::trim("hello") == "hello");
    REQUIRE(JobUsbIdLineKernel::trim("") == "");
}

TEST_CASE("JobUsbIdLineKernel identifies blank lines", "[job_usb][id][line_kernel][usage][blank]")
{
    REQUIRE(JobUsbIdLineKernel::blank(""));
    REQUIRE(JobUsbIdLineKernel::blank(" "));
    REQUIRE(JobUsbIdLineKernel::blank("\t"));
    REQUIRE(JobUsbIdLineKernel::blank(" \t\r\n"));

    REQUIRE_FALSE(JobUsbIdLineKernel::blank("x"));
    REQUIRE_FALSE(JobUsbIdLineKernel::blank("  x  "));
}

TEST_CASE("JobUsbIdLineKernel identifies comment lines", "[job_usb][id][line_kernel][usage][comment]")
{
    REQUIRE(JobUsbIdLineKernel::comment("# comment"));
    REQUIRE(JobUsbIdLineKernel::comment("   # comment"));
    REQUIRE(JobUsbIdLineKernel::comment("\t# comment"));

    REQUIRE_FALSE(JobUsbIdLineKernel::comment(""));
    REQUIRE_FALSE(JobUsbIdLineKernel::comment("046d Logitech"));
    REQUIRE_FALSE(JobUsbIdLineKernel::comment("value # comment"));
}

TEST_CASE("JobUsbIdLineKernel splits fields into key and value", "[job_usb][id][line_kernel][usage][field]")
{
    {
        JobUsbIdLineKernel::Field field{};
        REQUIRE(JobUsbIdLineKernel::splitField("046d Logitech, Inc." , field));
        REQUIRE(field.key == "046d");
        REQUIRE(field.value == "Logitech, Inc.");
    }

    {
        JobUsbIdLineKernel::Field field{};
        REQUIRE(JobUsbIdLineKernel::splitField("C 00  (Defined at Interface level)", field));
        REQUIRE(field.key == "C");
        REQUIRE(field.value == "00  (Defined at Interface level)");
    }

    {
        JobUsbIdLineKernel::Field field{};
        REQUIRE(JobUsbIdLineKernel::splitField("HUT 01  Generic Desktop Controls", field));
        REQUIRE(field.key == "HUT");
        REQUIRE(field.value == "01  Generic Desktop Controls");
    }
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failures
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdLineKernel hexadecimal parsing requires complete input", "[job_usb][id][line_kernel][edge][hex]")
{
    std::uint16_t value = 0xbeef;

    REQUIRE_FALSE(JobUsbIdLineKernel::parseHex("", value));
    REQUIRE(value == 0xbeef);

    REQUIRE_FALSE(JobUsbIdLineKernel::parseHex("xyz", value));
    REQUIRE(value == 0xbeef);

    REQUIRE_FALSE(JobUsbIdLineKernel::parseHex("046dxyz", value));
    REQUIRE(value == 0xbeef);

    REQUIRE_FALSE(JobUsbIdLineKernel::parseHex("046d ", value));
    REQUIRE(value == 0xbeef);
}

TEST_CASE("JobUsbIdLineKernel hexadecimal parsing rejects overflow", "[job_usb][id][line_kernel][edge][hex]")
{
    std::uint8_t value = 0x5a;

    REQUIRE_FALSE(JobUsbIdLineKernel::parseHex("100", value));
    REQUIRE(value == 0x5a);
}

TEST_CASE("JobUsbIdLineKernel hexadecimal parsing preserves destination on failure", "[job_usb][id][line_kernel][edge][hex]")
{
    std::uint32_t value = 0x12345678;

    REQUIRE_FALSE(JobUsbIdLineKernel::parseHex("not_hex", value));
    REQUIRE(value == 0x12345678);
}

TEST_CASE("JobUsbIdLineKernel indentation handles arbitrarily deep input without narrowing", "[job_usb][id][line_kernel][edge][indentation]")
{
    constexpr std::string_view line =
        "\t\t\t\t\t\t\t\t"
        "\t\t\t\t\t\t\t\t"
        "value";

    REQUIRE(JobUsbIdLineKernel::indentation(line) == 16);
    REQUIRE(JobUsbIdLineKernel::removeIndentation(line) == "value");
}

TEST_CASE("JobUsbIdLineKernel indentation counts tabs only", "[job_usb][id][line_kernel][edge][indentation]")
{
    REQUIRE(JobUsbIdLineKernel::indentation("    046d Logitech") == 0);
    REQUIRE(JobUsbIdLineKernel::indentation(" \t046d Logitech") == 0);
    REQUIRE(JobUsbIdLineKernel::indentation("\t    046d Logitech") == 1);
}

TEST_CASE("JobUsbIdLineKernel trimming handles all supported whitespace", "[job_usb][id][line_kernel][edge][trim]")
{
    REQUIRE(JobUsbIdLineKernel::trim(" \t\r\n") == "");
    REQUIRE(JobUsbIdLineKernel::trim("\rhello\r") == "hello");
    REQUIRE(JobUsbIdLineKernel::trim("\nhello\n") == "hello");
    REQUIRE(JobUsbIdLineKernel::trim("\thello\t") == "hello");
}

TEST_CASE("JobUsbIdLineKernel splitField handles missing values", "[job_usb][id][line_kernel][edge][field]")
{
    {
        JobUsbIdLineKernel::Field field{};
        REQUIRE_FALSE(JobUsbIdLineKernel::splitField("", field));
        REQUIRE(field.key.empty());
        REQUIRE(field.value.empty());
    }

    {
        JobUsbIdLineKernel::Field field{};
        REQUIRE(JobUsbIdLineKernel::splitField("046d", field));
        REQUIRE(field.key == "046d");
        REQUIRE(field.value.empty());
    }

    {
        JobUsbIdLineKernel::Field field{};
        REQUIRE(JobUsbIdLineKernel::splitField("046d    ", field));
        REQUIRE(field.key == "046d");
        REQUIRE(field.value.empty());
    }
}

TEST_CASE("JobUsbIdLineKernel splitField normalizes surrounding whitespace", "[job_usb][id][line_kernel][edge][field]")
{
    JobUsbIdLineKernel::Field field{};
    REQUIRE(JobUsbIdLineKernel::splitField("   046d    Logitech, Inc.   ", field));
    REQUIRE(field.key == "046d");
    REQUIRE(field.value == "Logitech, Inc.");
}

//////////////////////////////////////////////////////////
// Block 3: usb.ids shaped input
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdLineKernel handles a vendor record", "[job_usb][id][line_kernel][usb_ids]")
{
    constexpr std::string_view line = "046d  Logitech, Inc.";
    REQUIRE(JobUsbIdLineKernel::indentation(line) == 0);
    JobUsbIdLineKernel::Field field{};
    REQUIRE(JobUsbIdLineKernel::splitField(JobUsbIdLineKernel::removeIndentation(line), field));
    REQUIRE(field.key   == "046d");
    REQUIRE(field.value == "Logitech, Inc.");

    std::uint16_t vendorId = 0;
    REQUIRE(JobUsbIdLineKernel::parseHex(field.key, vendorId));
    REQUIRE(vendorId == 0x046d);
}

TEST_CASE("JobUsbIdLineKernel handles an indented product record", "[job_usb][id][line_kernel][usb_ids]")
{
    constexpr std::string_view line = "\tc534  Unifying Receiver";

    REQUIRE(JobUsbIdLineKernel::indentation(line) == 1);
    JobUsbIdLineKernel::Field field{};
    REQUIRE(JobUsbIdLineKernel::splitField(JobUsbIdLineKernel::removeIndentation(line), field));

    REQUIRE(field.key == "c534");
    REQUIRE(field.value == "Unifying Receiver");

    std::uint16_t productId = 0;

    REQUIRE(JobUsbIdLineKernel::parseHex(field.key, productId));
    REQUIRE(productId == 0xc534);
}

} // namespace job::usb::tests