#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <utility>

#include <job_yaml_decoded_scalar.h>

namespace job::yaml::tests {

TEST_CASE("YamlDecodedScalar defaults to an empty borrowed value",
          "[job_yaml][decoded_scalar]")
{
    const YamlDecodedScalar decoded;

    REQUIRE(decoded.value().empty());
    REQUIRE_FALSE(decoded.transformed());
    REQUIRE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar stores a borrowed scalar view",
          "[job_yaml][decoded_scalar][borrowed]")
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
          "[job_yaml][decoded_scalar][transformed]")
{
    YamlDecodedScalar decoded;
    decoded.setTransformed("alpha");

    REQUIRE(decoded.value() == "alpha");
    REQUIRE(decoded.transformed());
    REQUIRE_FALSE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar clear restores the empty borrowed state",
          "[job_yaml][decoded_scalar]")
{
    YamlDecodedScalar decoded;
    decoded.setTransformed("alpha");

    decoded.clear();

    REQUIRE(decoded.value().empty());
    REQUIRE_FALSE(decoded.transformed());
    REQUIRE(decoded.borrowed());
}

TEST_CASE("YamlDecodedScalar replaces borrowed storage with transformed storage",
          "[job_yaml][decoded_scalar]")
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
          "[job_yaml][decoded_scalar]")
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
          "[job_yaml][decoded_scalar][copy]")
{
    YamlDecodedScalar original;
    original.setTransformed("alpha");

    const YamlDecodedScalar copy = original;

    REQUIRE(original.value() == "alpha");
    REQUIRE(copy.value() == "alpha");
    REQUIRE(original.transformed());
    REQUIRE(copy.transformed());
    REQUIRE_FALSE(copy.borrowed());
}

TEST_CASE("YamlDecodedScalar copied borrowed values preserve the source view",
          "[job_yaml][decoded_scalar][borrowed][copy]")
{
    constexpr std::string_view source = "alpha";

    YamlDecodedScalar original;
    original.setBorrowed(source);

    const YamlDecodedScalar copy = original;

    REQUIRE(original.value() == source);
    REQUIRE(copy.value() == source);
    REQUIRE(original.value().data() == source.data());
    REQUIRE(copy.value().data() == source.data());
    REQUIRE_FALSE(copy.transformed());
    REQUIRE(copy.borrowed());
}

TEST_CASE("YamlDecodedScalar moved transformed values remain valid",
          "[job_yaml][decoded_scalar][move]")
{
    YamlDecodedScalar original;
    original.setTransformed("alpha");

    YamlDecodedScalar moved = std::move(original);

    REQUIRE(moved.value() == "alpha");
    REQUIRE(moved.transformed());
    REQUIRE_FALSE(moved.borrowed());
}

TEST_CASE("YamlDecodedScalar moved borrowed values preserve the source view",
          "[job_yaml][decoded_scalar][borrowed][move]")
{
    constexpr std::string_view source = "alpha";

    YamlDecodedScalar original;
    original.setBorrowed(source);

    YamlDecodedScalar moved = std::move(original);

    REQUIRE(moved.value() == source);
    REQUIRE(moved.value().data() == source.data());
    REQUIRE_FALSE(moved.transformed());
    REQUIRE(moved.borrowed());
}

TEST_CASE("YamlDecodedScalar createShared returns an empty decoded scalar",
          "[job_yaml][decoded_scalar][factory]")
{
    const auto decoded = YamlDecodedScalar::createShared();

    REQUIRE(decoded != nullptr);
    REQUIRE(decoded->value().empty());
    REQUIRE_FALSE(decoded->transformed());
    REQUIRE(decoded->borrowed());
}

TEST_CASE("YamlDecodedScalar createUniq returns an empty decoded scalar",
          "[job_yaml][decoded_scalar][factory]")
{
    const auto decoded = YamlDecodedScalar::createUniq();

    REQUIRE(decoded != nullptr);
    REQUIRE(decoded->value().empty());
    REQUIRE_FALSE(decoded->transformed());
    REQUIRE(decoded->borrowed());
}

} // namespace job::yaml::tests