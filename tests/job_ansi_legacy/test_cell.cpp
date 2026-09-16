#include <catch2/catch_test_macros.hpp>
#include "job_ansi_cell.h"

using namespace job::ansi;
using namespace job::ansi::utils;

TEST_CASE("Cell Basic Usage", "[cell][basic]")
{
    Cell cell;

    SECTION("Defaults") {
        CHECK(cell.getChar() == U' ');
        CHECK(cell.width() == 1);
        CHECK_FALSE(cell.isTrailing());
        CHECK(cell.dirty()); // New cells start dirty
        CHECK(cell.isEmpty());
    }

    SECTION("Setting Character") {
        cell.setChar('A', 1);
        CHECK(cell.getChar() == 'A');
        CHECK(cell.width() == 1);
        CHECK_FALSE(cell.isEmpty());
    }

    SECTION("Setting Wide Character") {
        // Emoji: 😀 (Width 2)
        // Note: Cell no longer calculates width; we tell it the width.
        cell.setChar(0x1F600, 2);
        CHECK(cell.getChar() == 0x1F600);
        CHECK(cell.width() == 2);
    }
}

TEST_CASE("Cell Bit-Field Integrity", "[cell][bits]")
{
    Cell cell;

    SECTION("Line Display Modes (Packed)") {
        cell.setLineDisplayMode(LineDisplayMode::DoubleWidth);
        CHECK(cell.getLineDisplayMode() == LineDisplayMode::DoubleWidth);

        // Ensure neighbor bits aren't clobbered
        cell.setLineWidth(2);
        CHECK(cell.getLineWidth() == 2);
        CHECK(cell.getLineDisplayMode() == LineDisplayMode::DoubleWidth);
    }

    SECTION("Protection Flags") {
        cell.setProtection(true, false); // Erase=true, Write=false
        CHECK(cell.isProtectedForErase());
        CHECK_FALSE(cell.isProtectedForWrite());

        cell.setProtection(false, true);
        CHECK_FALSE(cell.isProtectedForErase());
        CHECK(cell.isProtectedForWrite());
    }
}


TEST_CASE("Cell Combining Marks", "[cell][marks]")
{
    Cell cell('e'); // 'e'

    SECTION("Lazy Allocation") {
        // Should not have allocated unique_ptr yet
        CHECK_FALSE(cell.hasMarks());
        CHECK(cell.marks().empty());
    }

    SECTION("Appending Marks") {
        cell.appendCombiningChar(0x0301); // acute accent (´)

        REQUIRE(cell.hasMarks());
        CHECK(cell.marks().size() == 1);
        CHECK(cell.marks()[0] == 0x0301);

        // Full sequence check
        std::u32string full = cell.fullCharSequence();
        CHECK(full.size() == 2);
        CHECK(full[0] == 'e');
        CHECK(full[1] == 0x0301);
    }

    SECTION("Clearing Marks releases memory logic") {
        cell.appendCombiningChar(0x0301);
        REQUIRE(cell.hasMarks());

        cell.clearMarks();
        CHECK_FALSE(cell.hasMarks());
        CHECK(cell.marks().empty());
    }

    SECTION("Reset clears marks") {
        cell.appendCombiningChar(0x0301);
        cell.reset();
        CHECK_FALSE(cell.hasMarks());
        CHECK(cell.getChar() == U' ');
    }
}



TEST_CASE("Cell Attribute Flyweight", "[cell][attr]")
{
    Cell c1;
    Cell c2;

    SECTION("Default attributes share the same pointer") {
        // Both should point to the static default
        const Attributes *a1 = c1.attributes();
        const Attributes *a2 = c2.attributes();

        CHECK(a1 == a2);
    }

    SECTION("Setting attributes uses Interning") {
        Attributes attr;
        attr.bold = true;

        c1.setAttributes(attr);
        c2.setAttributes(attr);

        // Interning should return the SAME shared_ptr for identical values
        CHECK(c1.attributes() == c2.attributes());
    }

    SECTION("Copying Cell shares attributes") {
        Attributes attr;
        attr.bold = true;
        c1.setAttributes(attr);

        Cell c3 = c1; // Copy constructor
        CHECK(c3.attributes() == c1.attributes()); // Should point to same data
    }
}

// ============================================================================
// BLOCK FOUR: SIZE CHECK
// ============================================================================

TEST_CASE("Cell Size Optimization", "[cell][size]")
{
    // We targeted ~32 bytes.
    // 16 (shared_ptr) + 8 (unique_ptr) + 4 (char32) + 4 (bitfields) = 32 bytes
    // (Assuming 64-bit system)

    // Note: shared_ptr is 2 pointers (16 bytes). unique_ptr is 1 pointer (8 bytes).

    CHECK(sizeof(Cell) <= 32);
}
