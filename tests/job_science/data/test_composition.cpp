#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <data/composition.h>

using namespace job::science::data;

TEST_CASE("Composition presets are physically valid", "[composition][presets]")
{
    Composition rocky    = SciencePresets::rocky();
    Composition metallic = SciencePresets::metallic();
    Composition icy      = SciencePresets::icy();
    Composition carbon   = SciencePresets::carbonaceous();
    Composition sulfidic = SciencePresets::sulfidic();

    REQUIRE(CompositionUtil::isValid(rocky));
    REQUIRE(CompositionUtil::isValid(metallic));
    REQUIRE(CompositionUtil::isValid(icy));
    REQUIRE(CompositionUtil::isValid(carbon));
    REQUIRE(CompositionUtil::isValid(sulfidic));


    auto check_optical_range = [](const Composition& c) {
        REQUIRE(c.emissivity >= 0.0f);
        REQUIRE(c.emissivity <= 1.0f);

        REQUIRE(c.absorptivity >= 0.0f);
        REQUIRE(c.absorptivity <= 1.0f);

        REQUIRE(CompositionUtil::reflectivity(c) >= 0.0f);
        REQUIRE(CompositionUtil::reflectivity(c) <= 1.0f);
    };

    check_optical_range(rocky);
    check_optical_range(metallic);
    check_optical_range(icy);
    check_optical_range(carbon);
    check_optical_range(sulfidic);
}

TEST_CASE("Composition density and heat hierarchy make sense", "[composition][physics]")
{
    Composition rocky    = SciencePresets::rocky();
    Composition icy      = SciencePresets::icy();
    Composition metallic = SciencePresets::metallic();
    Composition carbon   = SciencePresets::carbonaceous();

    REQUIRE(icy.density < carbon.density);
    REQUIRE(carbon.density < rocky.density);
    REQUIRE(rocky.density < metallic.density);

    REQUIRE(icy.density > 0.0f);
    REQUIRE(rocky.heatCapacity > 0.0f);
    REQUIRE(metallic.emissivity > 0.0f);

    Composition normalized_rocky = rocky;
    CompositionUtil::normalize(normalized_rocky);
    REQUIRE(CompositionUtil::totalFraction(normalized_rocky) == Catch::Approx(1.0f));
    REQUIRE(normalized_rocky.silicate == Catch::Approx(rocky.silicate));

    Composition invalid_comp{};
    invalid_comp.silicate = 2.0f;
    invalid_comp.metal = 1.0f;
    CompositionUtil::normalize(invalid_comp);

    REQUIRE(CompositionUtil::totalFraction(invalid_comp) == Catch::Approx(1.0f));
    REQUIRE(invalid_comp.silicate == Catch::Approx(2.0f/3.0f));
}
