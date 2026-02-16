#include <catch2/catch_test_macros.hpp>
#include "utils/charset_translator.h"
#include "utils/job_ansi_enums.h"

using namespace job::ansi::utils;
using namespace job::ansi::utils::enums;


TEST_CASE("CharsetTranslator Defaults", "[utils][charset][defaults]")
{
    CharsetTranslator translator;

    SECTION("Initial state is US_ASCII for all slots") {
        CHECK(translator.g0() == CharsetDesignator::US_ASCII);
        CHECK(translator.g1() == CharsetDesignator::US_ASCII);
        CHECK(translator.g2() == CharsetDesignator::US_ASCII);
        CHECK(translator.g3() == CharsetDesignator::US_ASCII);
    }

    SECTION("Initial active set is G0") {
        CHECK(translator.charSet() == CharsetTranslator::CharsetSet::G0);
        CHECK(translator.activeCharset() == CharsetDesignator::US_ASCII);
    }

    SECTION("Default Identity Mapping") {
        // ASCII maps to itself by default
        CHECK(translator.applyCharset('A') == 'A');
        CHECK(translator.applyCharset('~') == '~');
    }
}

TEST_CASE("CharsetTranslator Configuration", "[utils][charset][config]")
{
    CharsetTranslator translator;

    SECTION("Mapping Designators to Slots") {
        translator.selectCharset(CharsetTranslator::CharsetSet::G0, CharsetDesignator::DEC_SPECIAL_GRAPHICS);
        CHECK(translator.g0() == CharsetDesignator::DEC_SPECIAL_GRAPHICS);
        translator.selectCharset(CharsetTranslator::CharsetSet::G1, CharsetDesignator::DEC_TECHNICAL);
        CHECK(translator.g1() == CharsetDesignator::DEC_TECHNICAL);
    }

    SECTION("Switching Active Slots (Locking Shift)") {
        translator.selectCharset(CharsetTranslator::CharsetSet::G1, CharsetDesignator::DEC_SPECIAL_GRAPHICS);

        translator.setCharSet(CharsetTranslator::CharsetSet::G1);

        CHECK(translator.charSet() == CharsetTranslator::CharsetSet::G1);
        CHECK(translator.activeCharset() == CharsetDesignator::DEC_SPECIAL_GRAPHICS);

        translator.setCharSet(CharsetTranslator::CharsetSet::G0);
        CHECK(translator.activeCharset() == CharsetDesignator::US_ASCII);
    }
}


TEST_CASE("CharsetTranslator Application", "[utils][charset][apply]")
{
    CharsetTranslator translator;

    SECTION("DEC Special Graphics (Line Drawing)") {
        translator.selectCharset(CharsetTranslator::CharsetSet::G0, CharsetDesignator::DEC_SPECIAL_GRAPHICS);
        CHECK(translator.applyCharset('q') == U'─');
        CHECK(translator.applyCharset('x') == U'│');
        CHECK(translator.applyCharset('a') == U'▒');
        CHECK(translator.applyCharset('A') == 'A');
    }

    SECTION("DEC Technical") {
        translator.selectCharset(CharsetTranslator::CharsetSet::G0, CharsetDesignator::DEC_TECHNICAL);
        CHECK(translator.applyCharset('3') == U'╲');
    }

    SECTION("Direct Helper Mapping") {
        char32_t result = translator.mapCharForCharset('l', CharsetDesignator::DEC_SPECIAL_GRAPHICS);
        CHECK(result == U'┌');
    }
}
