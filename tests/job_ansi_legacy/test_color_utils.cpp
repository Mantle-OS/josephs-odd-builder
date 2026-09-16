#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif
#include "utils/color_utils.h"

using namespace job::ansi::utils;

// ============================================================================
// BLOCK ONE: USAGE / EXAMPLES
// "How do I use this?"
// ============================================================================

TEST_CASE("Color Utils Basic Access", "[utils][color][usage]")
{
    SECTION("Standard ANSI colors are correct") {
        // Verify a few known constants
        CHECK(colors::Red()   == RGBColor(255, 0, 0));
        CHECK(colors::Green() == RGBColor(0, 255, 0));
        CHECK(colors::Blue()  == RGBColor(0, 0, 255));

        // Verify Brights
        CHECK(colors::BrightRed() == RGBColor(255, 85, 85));
    }

    SECTION("Parsing named colors") {
        auto c1 = parseNamedOrHexColor("red");
        REQUIRE(c1.has_value());
        CHECK(*c1 == colors::Red());

        auto c2 = parseNamedOrHexColor("bright-cyan");
        REQUIRE(c2.has_value());
        CHECK(*c2 == colors::BrightCyan());

        // Hex fallback (assuming RGBColor::parseColorString works)
        // If your RGBColor doesn't handle hex, this specific check might fail depending on impl
        // but the utils wrapper delegates to it.
        // auto c3 = parseNamedOrHexColor("#FF0000");
    }
}

TEST_CASE("Xterm 256 Palette Logic", "[utils][color][xterm]")
{
    SECTION("Standard 0-15 range matches ANSI tables") {
        CHECK(fromXterm256Palette(1) == colors::Red());       // Index 1
        CHECK(fromXterm256Palette(9) == colors::BrightRed()); // Index 9
    }

    SECTION("Color Cube Logic (16-231)") {
        // index 16 is usually Black (0,0,0) in the cube start
        CHECK(fromXterm256Palette(16) == RGBColor(0, 0, 0));

        // index 196 is classic Red in xterm
        // 196 - 16 = 180.
        // 180 / 36 = 5 (Max Red)
        // G, B = 0
        CHECK(fromXterm256Palette(196) == RGBColor(255, 0, 0));
    }

    SECTION("Grayscale Ramp (232-255)") {
        CHECK(fromXterm256Palette(232) == RGBColor(8, 8, 8));
        CHECK(fromXterm256Palette(255) == RGBColor(238, 238, 238));
    }

    SECTION("Nearest Color Lookup (Euclidean)") {
        // Red (255,0,0) exists at index 1 and 196. Algorithm picks first match.
        int result = findNearestXterm256(RGBColor(255, 0, 0));
        CHECK((result == 1 || result == 196));
    }

}

TEST_CASE("SGR Code Conversions", "[utils][color][sgr]")
{
    SECTION("SGR to Color (Foreground)") {
        // 31 = Red FG
        auto res = sgrCodeToColor(31);
        REQUIRE(res.has_value());
        CHECK(res->isForeground == true);
        CHECK(res->color == colors::Red());
    }

    SECTION("SGR to Color (Background)") {
        // 44 = Blue BG
        auto res = sgrCodeToColor(44);
        REQUIRE(res.has_value());
        CHECK(res->isForeground == false);
        CHECK(res->color == colors::Blue());
    }

    SECTION("SGR to Color (Bright FG)") {
        // 92 = Bright Green
        auto res = sgrCodeToColor(92);
        REQUIRE(res.has_value());
        CHECK(res->isForeground == true);
        CHECK(res->color == colors::BrightGreen());
    }

    SECTION("Generation strings") {
        // True Color string generation
        RGBColor c(10, 20, 30);
        // Expect: ESC[38;2;10;20;30m
        CHECK_THAT(sgrTrueColor(c, true), Catch::Matchers::ContainsSubstring("38;2;10;20;30"));

        // Xterm 256 string generation
        // Expect: ESC[48;5;196m
        CHECK_THAT(sgr256Color(196, false), Catch::Matchers::ContainsSubstring("48;5;196"));
    }
}

// ============================================================================
// BLOCK TWO: EDGE CASES
// "Stress the corners"
// ============================================================================

TEST_CASE("Color Utils Edge Cases", "[utils][color][edge]")
{
    SECTION("Invalid Named Colors") {
        auto res = parseNamedOrHexColor("not-a-color");
        // Unless "not-a-color" is a valid hex (it's not), this should fail
        CHECK_FALSE(res.has_value());
    }

    SECTION("Lighter/Darker Clamping") {
        RGBColor red(200, 0, 0);

        // Lighten 100% -> White
        CHECK(lighter(red, 256) == RGBColor(255, 255, 255));

        // Darken 100% -> Black
        CHECK(darker(red, 256) == RGBColor(0, 0, 0));

        // 0% change -> Original
        CHECK(lighter(red, 0) == red);
        CHECK(darker(red, 0) == red);
    }

    SECTION("Xterm Indices Clamping") {
        // Negative -> Index 0? Or clamped?
        // Implementation clamps [0, 255]
        // fromXterm256Palette calls std::clamp

        // -1 -> 0 (Black)
        CHECK(fromXterm256Palette(-1) == RGBColor(0,0,0));

        // 999 -> 255 (Brightest Gray)
        CHECK(fromXterm256Palette(999) == RGBColor(238,238,238));
    }

    SECTION("rgbToSGR Exact Matches Only") {
        // Standard Red -> 30+1 = 31
        CHECK(rgbToSGR(colors::Red(), true) == 31);

        // Slightly off red -> nullopt (it doesn't do nearest match)
        CHECK(rgbToSGR(RGBColor(254, 0, 0), true) == std::nullopt);
    }
}


#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Color Utils Benchmarks", "[utils][color][benchmark]")
{
    // Generating the palette table once is static, so we benchmark the lookup
    // logic which does math.
    volatile int inputIndex = 123;
    BENCHMARK("fromXterm256Palette (Cube calc)") {
        return fromXterm256Palette(inputIndex);
    };

    // This involves a loop over 256 items with distance math
    volatile uint8_t r=100, g=150, b=200;
    RGBColor randomColor(r, g, b);
    BENCHMARK("findNearestXterm256 (Search)") {
        return findNearestXterm256(randomColor);
    };

    // Benchmark string formatting
    BENCHMARK("sgrTrueColor string gen") {
        return sgrTrueColor(randomColor, true);
    };
}

#endif
