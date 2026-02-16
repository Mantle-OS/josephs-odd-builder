#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include "job_ansi_parser.h"
#include "job_ansi_screen.h"

using namespace job::ansi;
using namespace job::ansi::utils;

TEST_CASE("ANSIParser Basic Input", "[parser][basic]")
{
    Screen screen(5, 10);
    ANSIParser parser(&screen);

    SECTION("Plain ASCII text appears on screen") {
        parser.parseInput("Hello");

        CHECK(screen.cursor()->column() == 5);
        CHECK(screen.cellAt(0, 0)->getChar() == 'H');
        CHECK(screen.cellAt(0, 4)->getChar() == 'o');
    }

    SECTION("UTF-8 Decoding") {
        // "José" (é is 0xC3 0xA9 in UTF-8)
        parser.parseInput("Jos\xC3\xA9");

        CHECK(screen.cursor()->column() == 4);
        CHECK(screen.cellAt(0, 3)->getChar() == 0x00E9); // é codepoint
    }
}

TEST_CASE("ANSIParser CSI Sequences", "[parser][csi]")
{
    Screen screen(5, 10);
    ANSIParser parser(&screen);

    SECTION("Cursor Movement (CSI A, B, C, D)") {
        screen.setCursor(2, 2);

        parser.parseInput("\x1B[A"); // Up 1 -> (1, 2)
        CHECK(screen.cursor()->row() == 1);

        parser.parseInput("\x1B[B"); // Down 1 -> (2, 2)
        CHECK(screen.cursor()->row() == 2);

        parser.parseInput("\x1B[C"); // Right 1 -> (2, 3)
        CHECK(screen.cursor()->column() == 3);

        parser.parseInput("\x1B[D"); // Left 1 -> (2, 2)
        CHECK(screen.cursor()->column() == 2);
    }

    SECTION("Cursor Position (CSI H)") {
        parser.parseInput("\x1B[3;5H"); // Row 3, Col 5 (1-based)
        CHECK(screen.cursor()->row() == 2);
        CHECK(screen.cursor()->column() == 4);
    }

    SECTION("Colors (SGR)") {
        // Reset to ensure clean state
        parser.parseInput("\x1B[0m");

        // SGR 31: Foreground Red
        parser.parseInput("\x1B[31m");
        parser.parseInput("X");

        auto *c = screen.cellAt(0, 0);
        auto *attr = c->attributes();

        REQUIRE(attr->getForeground() != nullptr);

        // Your palette uses 255 for standard Red
        CHECK(attr->getForeground()->red() == 255);
        CHECK(attr->getForeground()->green() == 0);
        CHECK(attr->getForeground()->blue() == 0);
    }
}

TEST_CASE("ANSIParser State Machine Fragmentation", "[parser][fragment]")
{
    Screen screen(5, 10);
    ANSIParser parser(&screen);

    SECTION("Split Escape Sequence") {
        screen.setCursor(0, 0);

        parser.parseInput("\x1B");  // Packet 1: Escape
        CHECK(screen.cursor()->column() == 0);

        parser.parseInput("[");     // Packet 2: CSI
        CHECK(screen.cursor()->column() == 0);

        parser.parseInput("C");     // Packet 3: Command
        CHECK(screen.cursor()->column() == 1);
    }

    SECTION("Split UTF-8") {
        // Euro sign (€) is 3 bytes: 0xE2 0x82 0xAC
        parser.parseInput("\xE2");
        parser.parseInput("\x82");
        CHECK(screen.cursor()->column() == 0);

        parser.parseInput("\xAC");
        CHECK(screen.cursor()->column() == 1);
        CHECK(screen.cellAt(0, 0)->getChar() == 0x20AC);
    }
}

TEST_CASE("ANSIParser OSC Sequences", "[parser][osc]")
{
    Screen screen(5, 10);
    ANSIParser parser(&screen);

    // 1. Setup a "spy" to capture the callback
    std::string capturedCallbackTitle;
    bool callbackFired = false;

    screen.on_windowTitleChanged = [&](const std::string &newTitle) {
        capturedCallbackTitle = newTitle;
        callbackFired = true;
    };

    SECTION("OSC 0 Sets Window Title (Icon + Title)") {
        // \x07 is the standard BEL terminator
        parser.parseInput("\x1B]0;My Window Title\x07");

        // Verify internal state (The Getter)
        CHECK(screen.get_windowTitle() == "My Window Title");

        // Verify the API Callback fired (The Notification)
        CHECK(callbackFired);
        CHECK(capturedCallbackTitle == "My Window Title");
    }

    SECTION("OSC 2 Sets Window Title (Title Only)") {
        // Reset state
        callbackFired = false;
        capturedCallbackTitle.clear();

        // \x1B\ (ESC Backslash) is the ST (String Terminator)
        parser.parseInput("\x1B]2;Another Title\x1B\\");

        CHECK(screen.get_windowTitle() == "Another Title");

        // Ensure callback fired again for the new value
        CHECK(callbackFired);
        CHECK(capturedCallbackTitle == "Another Title");
    }
}



#ifdef JOB_TEST_BENCHMARKS
// Helper to generate a "compiler output" workload
// 10MB of text with occasional colors
std::string generateCompilerLog(int lines) {
    std::string out;
    out.reserve(lines * 80);
    for (int i = 0; i < lines; ++i) {
        out += "\033[32m[INFO]\033[0m Compiling source file ";
        out += "\033[1msrc/main.cpp\033[0m: ";
        out += "This is a standard log line that fills the screen width... ";
        out += std::to_string(i);
        out += "\n";
    }
    return out;
}

// Helper to generate "Rainbow Noise"
// Stresses the Attribute Interning cache heavily
std::string generateRainbowNoise(int count) {
    std::string out;
    out.reserve(count * 20);
    for (int i = 0; i < count; ++i) {
        // Random TrueColor SGR: ESC[38;2;r;g;bm
        int r = (i * 3) % 255;
        int g = (i * 7) % 255;
        int b = (i * 11) % 255;
        out += "\033[38;2;" + std::to_string(r) + ";" +
               std::to_string(g) + ";" +
               std::to_string(b) + "m#";
    }
    return out;
}

TEST_CASE("Benchmarks: Core Engine", "[benchmark]")
{
    // Setup
    Screen screen(24, 80);
    // Disable render callbacks to measure raw engine speed, not I/O
    screen.renderCallback = nullptr;

    ANSIParser parser(&screen);

    // Data Prep
    std::string fastLog = generateCompilerLog(1000); // ~100KB
    std::string heavyColor = generateRainbowNoise(5000); // 5000 unique colors

    BENCHMARK("Parser: Standard Log Stream (Throughput)") {
        // Reset screen to prevent infinite scrollback growth during bench loops
        screen.reset();
        parser.parseInput(fastLog);
        return screen.cursor()->row();
    };

    BENCHMARK("Parser: Heavy Attribute Churn (Interning Stress)") {
        // This hits the Attribute::intern lock every single character
        parser.parseInput(heavyColor);
        return screen.currentAttributes();
    };

    BENCHMARK("Screen: Vertical Scroll (Delete Lines)") {
        // Force 1000 scrolls
        for(int i = 0; i < 1000; ++i) {
            screen.scrollUp();
        }
        return screen.scrollback().size();
    };

    BENCHMARK("Screen: Full Grid Fill (PutChar)") {
        screen.setCursor(0,0);
        for(int i = 0; i < 24 * 80; ++i) {
            screen.putChar('X');
        }
        return screen.cursor()->row();
    };
}

#endif





