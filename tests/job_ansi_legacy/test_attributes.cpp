#include <catch2/catch_test_macros.hpp>


#include <job_ansi_attributes.h>

using namespace job::ansi;
using namespace job::ansi::utils;

TEST_CASE("Attributes Bit-Packing", "[attributes][bits]")
{
    Attributes attr;

    SECTION("Defaults are clean") {
        CHECK_FALSE(attr.bold);
        CHECK_FALSE(attr.italic);
        CHECK(attr.getUnderline() == UnderlineStyle::None);
        CHECK(attr.getForeground() == nullptr);
        CHECK(attr.getBackground() == nullptr);
    }

    SECTION("Flags don't overlap") {
        // Set every flag one by one and verify no cross-talk
        attr.bold = true;
        CHECK(attr.bold);
        CHECK_FALSE(attr.italic);
        CHECK_FALSE(attr.inverse);

        attr.italic = true;
        CHECK(attr.bold);
        CHECK(attr.italic);

        attr.inverse = true;
        CHECK(attr.inverse);

        // Reset check
        attr.reset();
        CHECK_FALSE(attr.bold);
        CHECK_FALSE(attr.inverse);
    }

    SECTION("Underline Style Packing (3 bits)") {
        attr.setUnderline(UnderlineStyle::Curly);
        CHECK(attr.getUnderline() == UnderlineStyle::Curly);
        CHECK_FALSE(attr.bold);
        CHECK_FALSE(attr.blink);
    }

    SECTION("Blink Modes (2 bits)") {
        attr.setBlink(1); // Slow
        CHECK(attr.blink == 1);

        attr.setBlink(2); // Rapid
        CHECK(attr.blink == 2);

        attr.setBlink(0); // None
        CHECK(attr.blink == 0);
    }
}


TEST_CASE("Attributes Colors", "[attributes][color]")
{
    Attributes attr;
    RGBColor red(255, 0, 0);
    RGBColor blue(0, 0, 255);

    SECTION("Setting Foreground") {
        CHECK(attr.getForeground() == nullptr);

        attr.setForeground(red);
        REQUIRE(attr.getForeground() != nullptr);
        CHECK(*attr.getForeground() == red);

        // Ensure BG is still empty
        CHECK(attr.getBackground() == nullptr);
    }

    SECTION("Setting Background") {
        attr.setBackground(blue);
        REQUIRE(attr.getBackground() != nullptr);
        CHECK(*attr.getBackground() == blue);
    }

    SECTION("Granular Resets") {
        attr.setForeground(red);
        attr.setBackground(blue);

        attr.resetForeground();
        CHECK(attr.getForeground() == nullptr);
        CHECK(attr.getBackground() != nullptr); // BG untouched

        attr.resetBackground();
        CHECK(attr.getBackground() == nullptr);
    }
}


TEST_CASE("Attributes Interning", "[attributes][intern]")
{
    SECTION("Identical attributes share pointers") {
        Attributes a;
        a.bold = true;
        a.setForeground(RGBColor(10, 20, 30));

        Attributes b;
        b.bold = true;
        b.setForeground(RGBColor(10, 20, 30));

        auto sharedA = Attributes::intern(a);
        auto sharedB = Attributes::intern(b);

        // POINTER equality check (flyweight pattern success)
        CHECK(sharedA == sharedB);
        CHECK(sharedA.get() == sharedB.get());
    }

    SECTION("Different attributes have different pointers") {
        Attributes a;
        a.bold = true;

        Attributes b;
        b.bold = false;

        auto sharedA = Attributes::intern(a);
        auto sharedB = Attributes::intern(b);

        CHECK(sharedA != sharedB);
        CHECK(sharedA.get() != sharedB.get());
    }
}

TEST_CASE("Attributes Equality", "[attributes][eq]")
{
    Attributes a;
    Attributes b;

    SECTION("Empty equality") {
        CHECK(a == b);
    }

    SECTION("Color equality") {
        a.setForeground(RGBColor(255, 0, 0));
        b.setForeground(RGBColor(255, 0, 0));
        CHECK(a == b);

        b.setForeground(RGBColor(255, 0, 1));
        CHECK(a != b);
    }

    SECTION("Flag vs Color distinction") {
        a.bold = true;
        b.bold = false;
        CHECK(a != b);
    }
}


TEST_CASE("Attributes Size Constraint", "[attributes][size]")
{
    CHECK(sizeof(Attributes) <= 16);
}
