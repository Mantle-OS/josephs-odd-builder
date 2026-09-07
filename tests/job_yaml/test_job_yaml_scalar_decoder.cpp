#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <utility>

#include <job_yaml_scalar_decoder.h>

#include "test_job_yaml_scalar_fixtures_generated.h"

namespace job::yaml::tests {

static bool viewWithinSource(std::string_view view, std::string_view source) noexcept
{
    if (view.empty())
        return view.data() >= source.data() && view.data() <= source.data() + source.size();

    return view.data() >= source.data() &&
           view.data() + view.size() <= source.data() + source.size();
}

static void testFixture(const YamlScalarFixture &fixture)
{
    // WARN("fixture: " << fixture.name);
    // WARN("production: " << fixture.production);
    // WARN("source: " << fixture.source);

    YamlDecodedScalar decoded;
    const bool result = YamlScalarDecoder::decode(fixture.source,
                                                  fixture.style,
                                                  decoded);

    REQUIRE(result == fixture.valid);

    if (!fixture.valid)
        return;

    REQUIRE(decoded.value() == fixture.expected);
    REQUIRE(decoded.transformed() == fixture.transformed);
    REQUIRE(decoded.borrowed() == !fixture.transformed);

    if (!fixture.transformed)
        REQUIRE(viewWithinSource(decoded.value(), fixture.source));
}

TEST_CASE("YamlDecodedScalar defaults to an empty borrowed value", "[job_yaml][scalar_decoder][decoded_scalar]")
{
    const YamlDecodedScalar decoded;

    REQUIRE(decoded.value().empty());
    REQUIRE_FALSE(decoded.transformed());
    REQUIRE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar stores a borrowed scalar view",
          "[job_yaml][scalar_decoder][decoded_scalar][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlDecodedScalar decoded;
    decoded.setBorrowed(source);

    REQUIRE(decoded.value() == source);
    REQUIRE(decoded.value().data() == source.data());
    REQUIRE_FALSE(decoded.transformed());
    REQUIRE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar stores transformed scalar text",
          "[job_yaml][scalar_decoder][decoded_scalar][transformed]")
{
    YamlDecodedScalar decoded;
    decoded.setTransformed("alpha");

    REQUIRE(decoded.value() == "alpha");
    REQUIRE(decoded.transformed());
    REQUIRE_FALSE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar clear restores the empty borrowed state",
          "[job_yaml][scalar_decoder][decoded_scalar]")
{
    YamlDecodedScalar decoded;
    decoded.setTransformed("alpha");

    decoded.clear();

    REQUIRE(decoded.value().empty());
    REQUIRE_FALSE(decoded.transformed());
    REQUIRE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar replaces borrowed storage with transformed storage",
          "[job_yaml][scalar_decoder][decoded_scalar]")
{
    constexpr std::string_view source = "alpha";

    YamlDecodedScalar decoded;
    decoded.setBorrowed(source);
    decoded.setTransformed("beta");

    REQUIRE(decoded.value() == "beta");
    REQUIRE(decoded.transformed());
    REQUIRE_FALSE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar replaces transformed storage with borrowed storage",
          "[job_yaml][scalar_decoder][decoded_scalar]")
{
    constexpr std::string_view source = "beta";

    YamlDecodedScalar decoded;
    decoded.setTransformed("alpha");
    decoded.setBorrowed(source);

    REQUIRE(decoded.value() == source);
    REQUIRE(decoded.value().data() == source.data());
    REQUIRE_FALSE(decoded.transformed());
    REQUIRE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar copied transformed values remain valid",
          "[job_yaml][scalar_decoder][decoded_scalar][copy]")
{
    YamlDecodedScalar original;
    original.setTransformed("alpha");

    YamlDecodedScalar copy = original;

    REQUIRE(original.value() == "alpha");
    REQUIRE(copy.value() == "alpha");
    REQUIRE(original.transformed());
    REQUIRE(copy.transformed());
}

TEST_CASE("YamlDecodedScalar moved transformed values remain valid",
          "[job_yaml][scalar_decoder][decoded_scalar][move]")
{
    YamlDecodedScalar original;
    original.setTransformed("alpha");

    YamlDecodedScalar moved = std::move(original);

    REQUIRE(moved.value() == "alpha");
    REQUIRE(moved.transformed());
    REQUIRE_FALSE(moved.borrowed());
}

TEST_CASE("YamlScalarDecoder rejects a non-scalar style",
          "[job_yaml][scalar_decoder]")
{
    YamlDecodedScalar decoded;

    REQUIRE_FALSE(YamlScalarDecoder::decode("alpha",
                                            YamlScalarStyle::None,
                                            decoded));
}

TEST_CASE("YamlScalarDecoder decodes generated plain scalar fixtures",
          "[job_yaml][scalar_decoder][plain][generated]")
{
    for (const auto &fixture : PlainScalarFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlScalarDecoder decodes generated single-quoted scalar fixtures",
          "[job_yaml][scalar_decoder][single_quoted][generated]")
{
    for (const auto &fixture : SingleQuotedScalarFixtures)
        testFixture(fixture);
}

TEST_CASE("YamlScalarDecoder decodes generated double-quoted scalar fixtures",
          "[job_yaml][scalar_decoder][double_quoted][generated]")
{
    for (const auto &fixture : DoubleQuotedScalarFixtures)
        testFixture(fixture);
}

} // namespace job::yaml::tests