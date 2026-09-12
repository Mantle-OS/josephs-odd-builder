#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>
#include <utility>

#include <job_usb_id_cursor.h>

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdCursor materializes the first line on construction", "[job_usb][id][cursor][usage]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver\n";

    JobUsbIdCursor cursor{source};

    REQUIRE_FALSE(cursor.atEnd());

    const auto &line = cursor.line();

    REQUIRE(line.raw == "046d  Logitech, Inc.");
    REQUIRE(line.text == "046d  Logitech, Inc.");
    REQUIRE(line.number == 1);
    REQUIRE(line.indentation == 0);
}

TEST_CASE("JobUsbIdCursor advances through physical lines", "[job_usb][id][cursor][usage]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver\n"
        "\t\t0001  Interface";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.line().text == "046d  Logitech, Inc.");

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().raw == "\tc534  Unifying Receiver");
    REQUIRE(cursor.line().text == "c534  Unifying Receiver");
    REQUIRE(cursor.line().indentation == 1);

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 3);
    REQUIRE(cursor.line().raw == "\t\t0001  Interface");
    REQUIRE(cursor.line().text == "0001  Interface");
    REQUIRE(cursor.line().indentation == 2);

    REQUIRE_FALSE(cursor.advance());
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JobUsbIdCursor exposes structural indentation separately from text", "[job_usb][id][cursor][usage][indentation]")
{
    constexpr std::string_view source =
        "\t\tHID usage";

    JobUsbIdCursor cursor{source};

    const auto &line = cursor.line();

    REQUIRE(line.raw == "\t\tHID usage");
    REQUIRE(line.text == "HID usage");
    REQUIRE(line.number == 1);
    REQUIRE(line.indentation == 2);
}

TEST_CASE("JobUsbIdCursor factories create initialized cursors", "[job_usb][id][cursor][usage][factory]")
{
    constexpr std::string_view source = "046d  Logitech, Inc.";

    const auto shared = JobUsbIdCursor::createShared(source);
    const auto unique = JobUsbIdCursor::createUniq(source);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE_FALSE(shared->atEnd());
    REQUIRE_FALSE(unique->atEnd());

    REQUIRE(shared->line().number == 1);
    REQUIRE(unique->line().number == 1);

    REQUIRE(shared->line().text == "046d  Logitech, Inc.");
    REQUIRE(unique->line().text == "046d  Logitech, Inc.");
}

//////////////////////////////////////////////////////////
// Block 2: Physical line semantics / edge cases
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdCursor begins at end for an empty source", "[job_usb][id][cursor][edge][empty]")
{
    JobUsbIdCursor cursor{std::string_view{}};
    REQUIRE(cursor.atEnd());
    REQUIRE_FALSE(cursor.advance());
}

TEST_CASE("JobUsbIdCursor accepts a final line without a newline", "[job_usb][id][cursor][edge][newline]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver";

    JobUsbIdCursor cursor{source};
    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.advance());
    REQUIRE_FALSE(cursor.atEnd());
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().raw == "\tc534  Unifying Receiver");
    REQUIRE(cursor.line().text == "c534  Unifying Receiver");
    REQUIRE_FALSE(cursor.advance());
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JobUsbIdCursor does not create a phantom line after a terminal newline", "[job_usb][id][cursor][edge][newline]")
{
    constexpr std::string_view source = "046d  Logitech, Inc.\n";

    JobUsbIdCursor cursor{source};
    REQUIRE_FALSE(cursor.atEnd());
    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.line().text == "046d  Logitech, Inc.");
    REQUIRE_FALSE(cursor.advance());
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JobUsbIdCursor strips carriage return from CRLF lines", "[job_usb][id][cursor][edge][crlf]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\r\n"
        "\tc534  Unifying Receiver\r\n";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().raw == "046d  Logitech, Inc.");
    REQUIRE(cursor.line().text == "046d  Logitech, Inc.");
    REQUIRE(cursor.line().number == 1);

    REQUIRE(cursor.advance());

    REQUIRE(cursor.line().raw == "\tc534  Unifying Receiver");
    REQUIRE(cursor.line().text == "c534  Unifying Receiver");
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().indentation == 1);

    REQUIRE_FALSE(cursor.advance());
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JobUsbIdCursor preserves blank physical lines", "[job_usb][id][cursor][edge][blank]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\n"
        "\tc534  Unifying Receiver";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().raw.empty());
    REQUIRE(cursor.line().text.empty());
    REQUIRE(cursor.line().indentation == 0);

    REQUIRE(cursor.advance());

    REQUIRE(cursor.line().number == 3);
    REQUIRE(cursor.line().text == "c534  Unifying Receiver");
}

TEST_CASE("JobUsbIdCursor preserves comment physical lines", "[job_usb][id][cursor][edge][comment]")
{
    constexpr std::string_view source =
        "# usb.ids comment\n"
        "046d  Logitech, Inc.";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.line().raw == "# usb.ids comment");
    REQUIRE(cursor.line().text == "# usb.ids comment");
    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().text == "046d  Logitech, Inc.");
}

TEST_CASE("JobUsbIdCursor counts consecutive blank lines as physical lines", "[job_usb][id][cursor][edge][blank]")
{
    constexpr std::string_view source =
        "\n"
        "\n"
        "046d  Logitech, Inc.";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.line().raw.empty());

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().raw.empty());

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 3);
    REQUIRE(cursor.line().text == "046d  Logitech, Inc.");

    REQUIRE_FALSE(cursor.advance());
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JobUsbIdCursor preserves spaces because only tabs are structural indentation", "[job_usb][id][cursor][edge][indentation]")
{
    constexpr std::string_view source = " \t046d  Logitech, Inc.";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().indentation == 0);
    REQUIRE(cursor.line().raw == " \t046d  Logitech, Inc.");
    REQUIRE(cursor.line().text == " \t046d  Logitech, Inc.");
}

//////////////////////////////////////////////////////////
// Block 3: Copy / move behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdCursor copies preserve cursor position independently", "[job_usb][id][cursor][copy]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver\n"
        "\t\t0001  Interface";

    JobUsbIdCursor original{source};

    REQUIRE(original.advance());
    REQUIRE(original.line().number == 2);

    JobUsbIdCursor copy{original};

    REQUIRE(copy.line().number == 2);
    REQUIRE(copy.line().text == "c534  Unifying Receiver");

    REQUIRE(copy.advance());

    REQUIRE(copy.line().number == 3);
    REQUIRE(copy.line().text == "0001  Interface");

    REQUIRE(original.line().number == 2);
    REQUIRE(original.line().text == "c534  Unifying Receiver");
}

TEST_CASE("JobUsbIdCursor move construction preserves cursor state", "[job_usb][id][cursor][move]")
{
    constexpr std::string_view source =
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver";

    JobUsbIdCursor original{source};

    REQUIRE(original.advance());
    REQUIRE(original.line().number == 2);

    JobUsbIdCursor moved{std::move(original)};

    REQUIRE_FALSE(moved.atEnd());
    REQUIRE(moved.line().number == 2);
    REQUIRE(moved.line().raw == "\tc534  Unifying Receiver");
    REQUIRE(moved.line().text == "c534  Unifying Receiver");
    REQUIRE(moved.line().indentation == 1);

    REQUIRE_FALSE(moved.advance());
    REQUIRE(moved.atEnd());
}

//////////////////////////////////////////////////////////
// Block 4: usb.ids shaped input
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdCursor walks a usb.ids vendor hierarchy without interpreting it", "[job_usb][id][cursor][usb_ids]")
{
    constexpr std::string_view source =
        "# Vendors\n"
        "\n"
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver\n"
        "\t\t0001  Receiver Interface\n"
        "1532  Razer USA, Ltd";

    JobUsbIdCursor cursor{source};

    REQUIRE(cursor.line().number == 1);
    REQUIRE(cursor.line().text == "# Vendors");
    REQUIRE(cursor.line().indentation == 0);

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 2);
    REQUIRE(cursor.line().text.empty());
    REQUIRE(cursor.line().indentation == 0);

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 3);
    REQUIRE(cursor.line().text == "046d  Logitech, Inc.");
    REQUIRE(cursor.line().indentation == 0);

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 4);
    REQUIRE(cursor.line().text == "c534  Unifying Receiver");
    REQUIRE(cursor.line().indentation == 1);

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 5);
    REQUIRE(cursor.line().text == "0001  Receiver Interface");
    REQUIRE(cursor.line().indentation == 2);

    REQUIRE(cursor.advance());
    REQUIRE(cursor.line().number == 6);
    REQUIRE(cursor.line().text == "1532  Razer USA, Ltd");
    REQUIRE(cursor.line().indentation == 0);

    REQUIRE_FALSE(cursor.advance());
    REQUIRE(cursor.atEnd());
}

} // namespace job::usb::tests