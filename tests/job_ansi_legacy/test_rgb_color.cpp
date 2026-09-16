#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <utils/rgb_color.h>

using namespace job::ansi::utils;

TEST_CASE("RGBColor Basic Usage", "[utils][color][usage]")
{
    SECTION("Default Constructor") {
        RGBColor c;
        // Spec: r=0, g=255, b=0, a=255
        CHECK(c.red() == 0);
        CHECK(c.green() == 255);
        CHECK(c.blue() == 0);
        CHECK(c.alpha() == 255);
    }

    SECTION("Parametrized Constructor") {
        RGBColor c(10, 20, 30, 40);
        CHECK(c.red() == 10);
        CHECK(c.green() == 20);
        CHECK(c.blue() == 30);
        CHECK(c.alpha() == 40);
    }

    SECTION("Equality Operators") {
        RGBColor c1(50, 50, 50);
        RGBColor c2(50, 50, 50);
        RGBColor c3(50, 50, 51);

        CHECK(c1 == c2);
        CHECK(c1 != c3);
    }
}

TEST_CASE("RGBColor Integer Conversions", "[utils][color][bits]")
{
    // Test Values: R=0x11, G=0x22, B=0x33, A=0x44
    RGBColor c(0x11, 0x22, 0x33, 0x44);

    SECTION("toRGBA (0xRRGGBBAA)") {
        // Expect: 11 22 33 44
        uint32_t val = c.toRGBA();
        CHECK(val == 0x11223344);
    }

    SECTION("toARGB (0xAARRGGBB)") {
        // Expect: 44 11 22 33
        uint32_t val = c.toARGB();
        CHECK(val == 0x44112233);
    }

    SECTION("fromRGBA Roundtrip") {
        uint32_t input = 0xDEADBEEF;
        // R=DE, G=AD, B=BE, A=EF
        RGBColor parsed = RGBColor::fromRGBA(input);

        CHECK(parsed.red()   == 0xDE);
        CHECK(parsed.green() == 0xAD);
        CHECK(parsed.blue()  == 0xBE);
        CHECK(parsed.alpha() == 0xEF);

        CHECK(parsed.toRGBA() == input);
    }

    SECTION("fromARGB Roundtrip") {
        uint32_t input = 0xFFCC0099;
        // A=FF, R=CC, G=00, B=99
        RGBColor parsed = RGBColor::fromARGB(input);

        CHECK(parsed.alpha() == 0xFF);
        CHECK(parsed.red()   == 0xCC);
        CHECK(parsed.green() == 0x00);
        CHECK(parsed.blue()  == 0x99);

        CHECK(parsed.toARGB() == input);
    }
}

TEST_CASE("RGBColor String Formatting", "[utils][color][string]")
{
    RGBColor c(255, 0, 128, 15); // FF 00 80 0F

    SECTION("toHexString (No Alpha)") {
        CHECK(c.toHexString(false) == "#FF0080");
    }

    SECTION("toHexString (With Alpha)") {
        CHECK(c.toHexString(true) == "#FF00800F");
    }
}



TEST_CASE("RGBColor String Parsing Edge Cases", "[utils][color][parsing]")
{
    SECTION("Valid Parsing") {
        // Standard 6-digit
        auto c1 = RGBColor::fromHexString("#FF0000");
        CHECK(c1 == RGBColor(255, 0, 0, 255));

        // 8-digit with Alpha
        auto c2 = RGBColor::fromHexString("#00FF0080");
        CHECK(c2 == RGBColor(0, 255, 0, 128));
    }

    SECTION("parseColorString Helper (Swallows exceptions)") {
        auto opt = parseColorString("#123456");
        REQUIRE(opt.has_value());
        CHECK(opt->red() == 0x12);

        auto fail = parseColorString("badstuff");
        CHECK_FALSE(fail.has_value());
    }

}


