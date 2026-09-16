#include <memory>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

TEST_CASE("JBaseObj satisfies BaseObject requirements")
{
    STATIC_REQUIRE(BaseObjectType<MacroBaseObject>);
    STATIC_REQUIRE(std::derived_from<MacroBaseObject, BaseObject>);

    STATIC_REQUIRE(std::default_initializable<MacroBaseObject>);
    STATIC_REQUIRE(std::destructible<MacroBaseObject>);

    STATIC_REQUIRE(std::copy_constructible<MacroBaseObject>);
    STATIC_REQUIRE(std::is_copy_assignable_v<MacroBaseObject>);
    STATIC_REQUIRE(std::move_constructible<MacroBaseObject>);
    STATIC_REQUIRE(std::is_move_assignable_v<MacroBaseObject>);
}

TEST_CASE("JLightObj satisfies LightObject requirements")
{
    STATIC_REQUIRE(LightObjectType<MacroLightObject>);
    STATIC_REQUIRE(std::derived_from<MacroLightObject, LightObject>);

    STATIC_REQUIRE(std::default_initializable<MacroLightObject>);
    STATIC_REQUIRE(std::destructible<MacroLightObject>);

    STATIC_REQUIRE_FALSE(std::copy_constructible<MacroLightObject>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<MacroLightObject>);
    STATIC_REQUIRE_FALSE(std::move_constructible<MacroLightObject>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<MacroLightObject>);
}

TEST_CASE("JObj satisfies Object requirements")
{
    STATIC_REQUIRE(ObjectType<MacroObject>);
    STATIC_REQUIRE(BaseObjectType<MacroObject>);
    STATIC_REQUIRE(std::derived_from<MacroObject, Object>);

    STATIC_REQUIRE(std::default_initializable<MacroObject>);
    STATIC_REQUIRE(std::destructible<MacroObject>);

    STATIC_REQUIRE_FALSE(std::copy_constructible<MacroObject>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<MacroObject>);
    STATIC_REQUIRE_FALSE(std::move_constructible<MacroObject>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<MacroObject>);
}

TEST_CASE("generated object classes provide canonical pointer aliases")
{
    STATIC_REQUIRE(std::same_as<MacroBaseObject::Ptr, std::shared_ptr<MacroBaseObject>>);
    STATIC_REQUIRE(std::same_as<MacroBaseObject::WPtr, std::weak_ptr<MacroBaseObject>>);
    STATIC_REQUIRE(std::same_as<MacroBaseObject::UPtr, std::unique_ptr<MacroBaseObject>>);

    STATIC_REQUIRE(std::same_as<MacroLightObject::Ptr, std::shared_ptr<MacroLightObject>>);
    STATIC_REQUIRE(std::same_as<MacroLightObject::WPtr, std::weak_ptr<MacroLightObject>>);
    STATIC_REQUIRE(std::same_as<MacroLightObject::UPtr, std::unique_ptr<MacroLightObject>>);

    STATIC_REQUIRE(std::same_as<MacroObject::Ptr, std::shared_ptr<MacroObject>>);
    STATIC_REQUIRE(std::same_as<MacroObject::WPtr, std::weak_ptr<MacroObject>>);
    STATIC_REQUIRE(std::same_as<MacroObject::UPtr, std::unique_ptr<MacroObject>>);
}

TEST_CASE("generated object classes provide shared factories")
{
    const auto base = MacroBaseObject::createShared();
    const auto light = MacroLightObject::createShared();
    const auto object = MacroObject::createShared();

    REQUIRE(base);
    REQUIRE(light);
    REQUIRE(object);

    STATIC_REQUIRE(std::same_as<decltype(MacroBaseObject::createShared()), MacroBaseObject::Ptr>);
    STATIC_REQUIRE(std::same_as<decltype(MacroLightObject::createShared()), MacroLightObject::Ptr>);
    STATIC_REQUIRE(std::same_as<decltype(MacroObject::createShared()), MacroObject::Ptr>);
}

TEST_CASE("generated object classes provide unique factories")
{
    const auto base = MacroBaseObject::createUniq();
    const auto light = MacroLightObject::createUniq();
    const auto object = MacroObject::createUniq();

    REQUIRE(base);
    REQUIRE(light);
    REQUIRE(object);

    STATIC_REQUIRE(std::same_as<decltype(MacroBaseObject::createUniq ( )), MacroBaseObject::UPtr>);
    STATIC_REQUIRE(std::same_as<decltype(MacroLightObject::createUniq ( )), MacroLightObject::UPtr>);
    STATIC_REQUIRE(std::same_as<decltype(MacroObject::createUniq ( )), MacroObject::UPtr>);
}

TEST_CASE("generated class members retain member macro behavior")
{
    MacroBaseObject base;

    REQUIRE(base.get_name() == "base");
    REQUIRE(base.get_value() == 42);

    base.set_name("updated" );
    base.set_value(100);

    REQUIRE(base.get_name() == "updated");
    REQUIRE(base.get_value() == 100);

    base.reset_name();
    base.reset_value();

    REQUIRE(base.get_name() == "base");
    REQUIRE(base.get_value() == 42);
}

TEST_CASE("JLightObj generated signal member emits on change")
{
    MacroLightObject object;

    u32 value = 0;
    std::size_t count = 0;

    const auto connection = object.valueChanged.connect([&](const u32 &emittedValue) {
        value = emittedValue;
        ++count;
    });

    REQUIRE(connection);

    object.set_value(42);
    REQUIRE(count == 0);

    object.set_value(100);

    REQUIRE(object.get_value() == 100);
    REQUIRE(value == 100);
    REQUIRE(count == 1);
}

TEST_CASE("JObj generated signal member emits on change")
{
    MacroObject object;

    u32 value = 0;
    std::size_t count = 0;

    const auto connection = object.valueChanged.connect([&](const u32 &emittedValue) {
        value = emittedValue;
        ++count;
    });

    REQUIRE(connection);

    object.set_value(42);
    REQUIRE(count == 0);

    object.set_value(100);

    REQUIRE(object.get_value() == 100);
    REQUIRE(value == 100);
    REQUIRE(count == 1);
}