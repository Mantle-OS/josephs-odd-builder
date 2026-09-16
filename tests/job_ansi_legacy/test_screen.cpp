#include <catch2/catch_test_macros.hpp>
#include "job_ansi_screen.h"

using namespace job::ansi;

// Helper to fill a row with a char
void fillRow(Screen &screen, int row, char32_t ch) {
    screen.setCursor(row, 0);
    for (int i = 0; i < screen.columnCount(); ++i)
        screen.putChar(ch);
}

TEST_CASE("Screen Initialization", "[screen][init]")
{
    Screen screen(24, 80);

    SECTION("Dimensions are correct") {
        CHECK(screen.rowCount() == 24);
        CHECK(screen.columnCount() == 80);
    }

    SECTION("Cursor starts at 0,0") {
        REQUIRE(screen.cursor() != nullptr);
        CHECK(screen.cursor()->row() == 0);
        CHECK(screen.cursor()->column() == 0);
    }

    SECTION("Scrollback is configured") {
        // Assuming default constant from header
        CHECK(screen.scrollback().maxLines() == MAX_SCROLLBACK_LINES);
        CHECK(screen.scrollback().empty());
    }
}

TEST_CASE("Screen Character Output", "[screen][output]")
{
    Screen screen(10, 10);

    SECTION("Writing characters advances cursor") {
        screen.setCursor(0, 0);
        screen.putChar('A');

        CHECK(screen.cursor()->row() == 0);
        CHECK(screen.cursor()->column() == 1); // Advanced by 1

        // Check grid content
        Cell* cell = screen.cellAt(0, 0);
        REQUIRE(cell != nullptr);
        CHECK(cell->getChar() == 'A');
    }

    SECTION("Writing at end of line performs Late Wrap (if enabled)") {
        screen.set_wraparound(true);
        screen.setCursor(0, 9); // Last column (width is 10)

        screen.putChar('B');

        CHECK(screen.cursor()->row() == 0);
        CHECK(screen.cursor()->column() == 9);
        CHECK(screen.cellAt(0, 9)->getChar() == 'B');

        screen.putChar('C');

        CHECK(screen.cellAt(1, 0)->getChar() == 'C');

        CHECK(screen.cursor()->row() == 1);
        CHECK(screen.cursor()->column() == 1);
    }

    SECTION("Writing at end of line clamps (if wrap disabled)") {
        screen.set_wraparound(false);
        screen.setCursor(0, 9);
        screen.putChar('C');

        // Should stay at last column
        CHECK(screen.cursor()->row() == 0);
        CHECK(screen.cursor()->column() == 9); // Clamped

        // Content should be 'C' (overwriting whatever was there)
        CHECK(screen.cellAt(0, 9)->getChar() == 'C');
    }
}

TEST_CASE("Screen Scrolling & History", "[screen][scroll]")
{
    // Small screen for easier testing
    Screen screen(5, 10);
    screen.set_wraparound(true);

    SECTION("Implicit Scroll (newline at bottom)") {
        // Fill the screen
        for(int i=0; i<5; ++i)
            fillRow(screen, i, '0' + i);

        // Cursor is now at (4, 10) -> effectively (5,0) implicit or clamped
        screen.setCursor(4, 9);

        // Force a scroll via newline (or putChar wrapping)
        // Let's use explicit scrollUp logic usually triggered by \n at bottom
        screen.scrollUp();

        // Top row ('0') should be in scrollback
        REQUIRE(screen.scrollback().size() == 1);
        const auto& line = screen.scrollback().back();
        CHECK(line[0].getChar() == '0');

        // 2. Rows shifted up
        // Row 0 on screen should now contain what was Row 1 ('1')
        CHECK(screen.cellAt(0, 0)->getChar() == '1');

        // 3. Bottom row should be blank/new
        CHECK(screen.cellAt(4, 0)->getChar() == ' ');
    }

    SECTION("Explicit Scroll Region (DECSTBM)") {
        // Set region to middle 3 lines (rows 1, 2, 3). 0-based.
        // top=1, bottom=4 (exclusive? usually inclusive in CSI, converted to exclusive index internally)
        // Assuming implementation handles 1-based args: setScrollRegion(1, 3)
        // Let's assume the API expects 0-based indices for top/bottom bounds.
        // Usually Screen::setScrollRegion handles the conversion. Let's assume it sets internal state.

        screen.setScrollRegion(1, 4); // Rows 1, 2, 3 affected. Row 0 and 4 static.

        fillRow(screen, 0, 'T'); // Top (Static)
        fillRow(screen, 1, 'A');
        fillRow(screen, 2, 'B');
        fillRow(screen, 3, 'C');
        fillRow(screen, 4, 'B'); // Bottom (Static)

        screen.setCursor(3, 0); // At bottom of scroll region
        screen.scrollUp(screen.scrollTop(), screen.scrollBottom());

        // Top static row unchanged
        CHECK(screen.cellAt(0, 0)->getChar() == 'T');

        // Region shifted
        CHECK(screen.cellAt(1, 0)->getChar() == 'B'); // A moved out/deleted? (Region scroll doesn't usually save to history)
        CHECK(screen.cellAt(2, 0)->getChar() == 'C');
        CHECK(screen.cellAt(3, 0)->getChar() == ' '); // New line

        // Bottom static row unchanged
        CHECK(screen.cellAt(4, 0)->getChar() == 'B');
    }
}

TEST_CASE("Screen Buffers (Alt Grid)", "[screen][alt]")
{
    Screen screen(5, 10);

    // Write to Primary
    screen.putChar('P');
    CHECK(screen.cellAt(0, 0)->getChar() == 'P');

    SECTION("Enter Alternate Grid") {
        screen.enterAlternateGrid();
        CHECK(screen.isAlternateGridActive());

        // Alt grid should be empty/clean usually, or copy?
        // Xterm default is no clear, but many implementations clear.
        // Let's verify we are writing to a different grid.
        screen.setCursor(0, 0);
        screen.putChar('A');
        CHECK(screen.cellAt(0, 0)->getChar() == 'A');

        // Exit
        screen.exitAlternateGrid();
        CHECK_FALSE(screen.isAlternateGridActive());

        // Should have restored Primary state
        CHECK(screen.cellAt(0, 0)->getChar() == 'P');
    }
}

TEST_CASE("Screen Attributes Integration", "[screen][attr]")
{
    Screen screen(5, 5);

    Attributes attr;
    attr.bold = true;
    attr.setForeground(RGBColor(255, 0, 0));

    // Set current style
    screen.setCurrentAttributes(Attributes::intern(attr));

    // Write char
    screen.putChar('X');

    // Inspect cell
    Cell* c = screen.cellAt(0, 0);
    REQUIRE(c != nullptr);
    CHECK(c->getChar() == 'X');

    const Attributes* cAttr = c->attributes();
    CHECK(cAttr->bold == true);

    // Check color via new API
    REQUIRE(cAttr->getForeground() != nullptr);
    CHECK(*cAttr->getForeground() == RGBColor(255, 0, 0));
}


TEST_CASE("Screen Insert/Delete Operations", "[screen][ops]")
{
    // 5 rows, 10 cols
    Screen screen(5, 10);
    fillRow(screen, 0, 'A'); // AAAAA...
    fillRow(screen, 1, 'B'); // BBBBB...
    fillRow(screen, 2, 'C'); // CCCCC...

    SECTION("Delete Line (DL) shifts rows up") {
        screen.setCursor(1, 0); // On 'B' row
        screen.deleteLines(1);

        // Row 0 unchanged
        CHECK(screen.cellAt(0, 0)->getChar() == 'A');
        // Row 1 should now be 'C' (shifted up)
        CHECK(screen.cellAt(1, 0)->getChar() == 'C');
        // Row 2 should be blank (new line pulled in)
        CHECK(screen.cellAt(2, 0)->getChar() == ' ');
    }

    SECTION("Insert Line (IL) shifts rows down") {
        screen.setCursor(1, 0); // On 'B' row
        screen.insertLines(1, 1);

        // Row 0 unchanged
        CHECK(screen.cellAt(0, 0)->getChar() == 'A');
        // Row 1 should be blank (new inserted line)
        CHECK(screen.cellAt(1, 0)->getChar() == ' ');
        // Row 2 should be 'B' (shifted down)
        CHECK(screen.cellAt(2, 0)->getChar() == 'B');
        // 'C' row pushed off the bottom/region
    }

    SECTION("Delete Char (DCH) shifts characters left") {
        screen.setCursor(0, 0);
        screen.putChar('X');
        screen.putChar('Y');
        screen.putChar('Z');
        // Line is "XYZAAAAAAA"

        screen.setCursor(0, 1); // On 'Y'
        screen.deleteChars(1);

        // Should be "XZAAAAAAA "
        CHECK(screen.cellAt(0, 0)->getChar() == 'X');
        CHECK(screen.cellAt(0, 1)->getChar() == 'Z'); // Y gone, Z shifted left
        CHECK(screen.cellAt(0, 9)->getChar() == ' '); // Space pulled in at end
    }

    SECTION("Insert Char (ICH) - requires Mode check or Parser logic?") {
        // Screen::putChar handles Insert Mode internally if set_insertMode(true) is called.
        // But usually ICH (CSI @) is an explicit command too.
        // Let's test the 'insert mode' flag during typing.

        screen.clearLine();
        screen.setCursor(0, 0);
        screen.putChar('A');
        screen.putChar('B');
        screen.putChar('C');
        // "ABC       "

        screen.setCursor(0, 1); // On 'B'
        screen.set_insertMode(true);
        screen.putChar('X');

        // Should be "AXBC      "
        CHECK(screen.cellAt(0, 0)->getChar() == 'A');
        CHECK(screen.cellAt(0, 1)->getChar() == 'X');
        CHECK(screen.cellAt(0, 2)->getChar() == 'B'); // Shifted right
    }
}



