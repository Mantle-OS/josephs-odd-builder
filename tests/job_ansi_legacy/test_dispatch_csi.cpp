#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <span>

#include "job_ansi_screen.h"
#include "esc/csi/csi.h"

// We need these to construct the params vector easily
using namespace job::ansi;
using namespace job::ansi::csi;
using namespace job::ansi::utils::enums;

TEST_CASE("DispatchCSI Routing logic", "[csi][dispatch]")
{
    Screen screen(10, 10);
    DispatchCSI dispatcher(&screen);

    SECTION("Cursor Movement Delegation (CUU)") {
        screen.setCursor(5, 5);

        // Simulate CSI 2 A (Up 2)
        std::vector<int> params = { 2 };
        dispatcher.dispatch(CSI_CODE::CUU, params);

        CHECK(screen.cursor()->row() == 3);
        CHECK(screen.cursor()->column() == 5);
    }

    SECTION("SGR Delegation (Colors)") {
        // Simulate CSI 31 m (Red Foreground)
        std::vector<int> params = { 31 };
        dispatcher.dispatch(CSI_CODE::SGR, params);

        // Verify Attribute state on screen
        auto* attr = screen.currentAttributes();
        REQUIRE(attr->getForeground() != nullptr);
        CHECK(attr->getForeground()->red() == 255);
    }
}

TEST_CASE("DispatchCSI Private Mode Logic", "[csi][modes]")
{
    Screen screen(10, 10);
    DispatchCSI dispatcher(&screen);

    SECTION("Public Mode (SM 4) -> Insert Mode") {
        screen.set_insertMode(false);

        dispatcher.setPrivateMode(false); // Default
        std::vector<int> params = { 4 };  // IRM (Insert Replacement Mode)

        dispatcher.dispatch(CSI_CODE::SM, params);

        CHECK(screen.get_insertMode() == true);
    }

    SECTION("Private Mode (?SM 25) -> Cursor Visible") {
        screen.set_cursorVisible(false);

        // The Parser usually sets this flag when it sees '?'
        dispatcher.setPrivateMode(true);
        std::vector<int> params = { 25 }; // DECTCEM (Text Cursor Enable Mode)

        dispatcher.dispatch(CSI_CODE::SM, params);

        CHECK(screen.get_cursorVisible() == true);

        // Verify flag auto-reset
        CHECK(dispatcher.privateMode() == false);
    }

    SECTION("Private Mode Reset (RM ? 25)") {
        screen.set_cursorVisible(true);

        dispatcher.setPrivateMode(true);
        std::vector<int> params = { 25 };

        dispatcher.dispatch(CSI_CODE::RM, params);

        CHECK(screen.get_cursorVisible() == false);
    }
}

TEST_CASE("DispatchCSI State Tracking", "[csi][state]")
{
    Screen screen(10, 10);
    DispatchCSI dispatcher(&screen);

    SECTION("Last Printed Character Tracking") {
        // This is used for "Repeat Last Character" (REP / CSI b)

        // 1. Clear state
        dispatcher.clearLastPrintedChar();

        // 2. Print 'A'
        dispatcher.handlePrintable('A');

        // 3. Verify Screen got it
        CHECK(screen.cellAt(0, 0)->getChar() == 'A');

        // 4. Verify Repeat works (CSI 2 b)
        // Note: REP relies on internal state inside DispatchUnclassified usually,
        // or DispatchCSI needs to expose the char.
        // Based on your header, DispatchCSI holds m_lastPrintedChar,
        // but dispatch logic passes it down.

        // Let's manually verify the helper function logic
        dispatcher.setLastPrintedChar('Z');
        // If we implemented REP, we would test it here.
        // For now, ensure the API doesn't crash.
        dispatcher.clearLastPrintedChar();
    }
}

TEST_CASE("DispatchCSI Tilde Sequences", "[csi][tilde]")
{
    Screen screen(10, 10);
    DispatchCSI dispatcher(&screen);

    SECTION("Bracketed Paste Mode Toggle") {
        // Logic check: Your implementation toggles paste enabled state?
        // Actually the code provided says:
        // case BRACKETED_PASTE_START: if(enabled) set(true)
        // case BRACKETED_PASTE_END:   if(enabled) set(false)

        // Let's set the precondition
        screen.set_bracketedPasteEnabled(true);

        std::vector<int> startParams = { 200 };
        // This effectively keeps it true in your implementation
        dispatcher.dispatchTilde(startParams);
        CHECK(screen.get_bracketedPasteEnabled() == true);

        std::vector<int> endParams = { 201 };
        // This should flip it to false?
        // Wait, looking at your code: set_bracketedPasteEnabled(false)
        dispatcher.dispatchTilde(endParams);
        CHECK(screen.get_bracketedPasteEnabled() == false);
    }
}
