#include <memory>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

// =============================================================================
// Shared Pointer Macros
// =============================================================================

TEST_CASE("JPTR_RW exposes the expected pointer type")
{
    SharedPointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_rw())>, MacroObject::Ptr>);
    REQUIRE_FALSE(fixture.get_rw());
}

TEST_CASE("JPTR_RW stores shared ownership")
{
    SharedPointerFixture fixture;
    auto object = MacroObject::createShared();

    REQUIRE(object);
    REQUIRE(object.use_count() == 1);

    fixture.set_rw(object);

    REQUIRE(fixture.get_rw());
    REQUIRE(fixture.get_rw() == object);
    REQUIRE(fixture.get_rw().get() == object.get());
    REQUIRE(object.use_count() == 2);
}

TEST_CASE("JPTR_RW replaces shared ownership")
{
    SharedPointerFixture fixture;

    auto first = MacroObject::createShared();
    auto second = MacroObject::createShared();

    fixture.set_rw(first);

    REQUIRE(fixture.get_rw() == first);
    REQUIRE(first.use_count() == 2);

    fixture.set_rw(second);

    REQUIRE(fixture.get_rw() == second);
    REQUIRE(first.use_count() == 1);
    REQUIRE(second.use_count() == 2);
}

TEST_CASE("JPTR_RW init creates shared object")
{
    SharedPointerFixture fixture;
    REQUIRE_FALSE(fixture.get_rw());
    fixture.init_rw(); // Huh ?
    REQUIRE(fixture.get_rw());
}

TEST_CASE("JPTR_RO exposes shared pointer getter")
{
    SharedPointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_ro())>, MacroObject::Ptr>);
    REQUIRE_FALSE(fixture.get_ro());
}

TEST_CASE("JPTR_CONST exposes shared pointer getter")
{
    SharedPointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_constant())>,MacroObject::Ptr>);
    REQUIRE_FALSE(fixture.get_constant());
}

// =============================================================================
// Weak Pointer Macros
// =============================================================================

TEST_CASE("JWPTR_RW exposes the expected pointer type")
{
    WeakPointerFixture fixture;

    STATIC_REQUIRE(std::same_as<
                   std::remove_cvref_t<decltype(fixture.get_rw())>,
                   MacroObject::WPtr>);

    REQUIRE(fixture.get_rw().expired());
}

TEST_CASE("JWPTR_RW observes shared object without taking ownership")
{
    WeakPointerFixture fixture;
    auto object = MacroObject::createShared();

    REQUIRE(object.use_count() == 1);

    fixture.set_rw(object);

    REQUIRE_FALSE(fixture.get_rw().expired());
    REQUIRE(object.use_count() == 1);

    const auto locked = fixture.get_rw().lock();

    REQUIRE(locked);
    REQUIRE(locked == object);
    REQUIRE(object.use_count() == 2);
}

TEST_CASE("JWPTR_RW expires when owning object is destroyed")
{
    WeakPointerFixture fixture;

    {
        auto object = MacroObject::createShared();

        fixture.set_rw(object);

        REQUIRE_FALSE(fixture.get_rw().expired());
        REQUIRE(object.use_count() == 1);
    }

    REQUIRE(fixture.get_rw().expired());
    REQUIRE_FALSE(fixture.get_rw().lock());
}

TEST_CASE("JWPTR_RW can replace observed object")
{
    WeakPointerFixture fixture;

    auto first = MacroObject::createShared();
    auto second = MacroObject::createShared();

    fixture.set_rw(first);
    REQUIRE(fixture.get_rw().lock() == first);
    fixture.set_rw(second);

    REQUIRE(fixture.get_rw().lock() == second);
    REQUIRE(first.use_count() == 1);
    REQUIRE(second.use_count() == 1);
}

TEST_CASE("JWPTR_RO exposes weak pointer getter")
{
    WeakPointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_ro())>,MacroObject::WPtr>);
    REQUIRE(fixture.get_ro().expired());
}

TEST_CASE("JWPTR_CONST exposes weak pointer getter")
{
    WeakPointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_constant())>,MacroObject::WPtr>);
    REQUIRE(fixture.get_constant().expired());
}

// =============================================================================
// Unique Pointer Macros
// =============================================================================

TEST_CASE("JUPTR_RW exposes the expected pointer type")
{
    UniquePointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_rw())>, MacroObject::UPtr>);
    REQUIRE_FALSE(fixture.get_rw());
}

TEST_CASE("JUPTR_RW transfers unique ownership")
{
    UniquePointerFixture fixture;

    auto object = MacroObject::createUniq();
    auto *address = object.get();

    REQUIRE(object);

    fixture.set_rw(std::move(object));

    REQUIRE_FALSE(object);
    REQUIRE(fixture.get_rw());
    REQUIRE(fixture.get_rw().get() == address);
}

TEST_CASE("JUPTR_RW replaces unique ownership")
{
    UniquePointerFixture fixture;

    auto first = MacroObject::createUniq();
    REQUIRE(first);
    fixture.set_rw(std::move(first));

    auto second = MacroObject::createUniq();
    auto *secondAddress = second.get();

    REQUIRE_FALSE(first);
    REQUIRE(fixture.get_rw());

    fixture.set_rw(std::move(second));

    REQUIRE_FALSE(second);
    REQUIRE(fixture.get_rw());
    REQUIRE(fixture.get_rw().get() == secondAddress);
}

TEST_CASE("JUPTR_RW init creates unique object")
{
    UniquePointerFixture fixture;
    REQUIRE_FALSE(fixture.get_rw());
    fixture.init_rw();
    REQUIRE(fixture.get_rw());
}

TEST_CASE("JUPTR_RO exposes unique pointer getter")
{
    UniquePointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_ro())>, MacroObject::UPtr>);
    REQUIRE_FALSE(fixture.get_ro());
}

TEST_CASE("JUPTR_CONST exposes unique pointer getter")
{
    UniquePointerFixture fixture;
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(fixture.get_constant())>, MacroObject::UPtr>);
    REQUIRE_FALSE(fixture.get_constant());
}