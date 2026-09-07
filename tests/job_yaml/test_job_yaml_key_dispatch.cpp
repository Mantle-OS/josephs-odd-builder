#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_key_dispatch.h>

#include "test_job_yaml_utils.h"
#include "test_job_yaml_fixtures.h"

#include <concepts>
#include <meta>
#include <string_view>
#include <type_traits>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Basic dispatch
// ========================================

TEST_CASE("YamlKeyDispatch dispatches count member", "[job_yaml][key_dispatch]")
{
    bool called = false;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("count", [&]<auto member> {
        called = true;

        constexpr std::string_view name = std::meta::identifier_of(member);

        STATIC_REQUIRE(std::meta::is_nonstatic_data_member(member));
        REQUIRE(name == "count");
    });

    REQUIRE(matched);
    REQUIRE(called);
}

TEST_CASE("YamlKeyDispatch dispatches enabled member", "[job_yaml][key_dispatch]")
{
    bool called = false;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto member> {
        called = true;

        constexpr std::string_view name = std::meta::identifier_of(member);

        STATIC_REQUIRE(std::meta::is_nonstatic_data_member(member));
        REQUIRE(name == "enabled");
    });

    REQUIRE(matched);
    REQUIRE(called);
}

TEST_CASE("YamlKeyDispatch dispatches scale member", "[job_yaml][key_dispatch]")
{
    bool called = false;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("scale", [&]<auto member> {
        called = true;

        constexpr std::string_view name = std::meta::identifier_of(member);

        STATIC_REQUIRE(std::meta::is_nonstatic_data_member(member));
        REQUIRE(name == "scale");
    });

    REQUIRE(matched);
    REQUIRE(called);
}

// ========================================
// Failed dispatch
// ========================================

TEST_CASE("YamlKeyDispatch returns false for unknown key", "[job_yaml][key_dispatch]")
{
    bool called = false;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("missing", [&]<auto> {
        called = true;
    });

    REQUIRE_FALSE(matched);
    REQUIRE_FALSE(called);
}

TEST_CASE("YamlKeyDispatch returns false for empty key", "[job_yaml][key_dispatch]")
{
    bool called = false;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("", [&]<auto> {
        called = true;
    });

    REQUIRE_FALSE(matched);
    REQUIRE_FALSE(called);
}

TEST_CASE("YamlKeyDispatch requires exact member name match", "[job_yaml][key_dispatch]")
{
    const std::string_view cases[] = {
        "c",
        "cou",
        "counts",
        "Count",
        "COUNT",
        " enabled",
        "enabled ",
        "scaleX",
        "_scale",
    };

    for (const std::string_view key : cases) {
        bool called = false;

        const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>(key, [&]<auto> {
            called = true;
        });

        REQUIRE_FALSE(matched);
        REQUIRE_FALSE(called);
    }
}

// ========================================
// Callback behavior
// ========================================

TEST_CASE("YamlKeyDispatch invokes callback exactly once", "[job_yaml][key_dispatch]")
{
    unsigned calls = 0;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto> {
        ++calls;
    });

    REQUIRE(matched);
    REQUIRE(calls == 1);
}

TEST_CASE("YamlKeyDispatch invokes callback only for matched member", "[job_yaml][key_dispatch]")
{
    unsigned calls = 0;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("scale", [&]<auto member> {
        ++calls;

        constexpr std::string_view name = std::meta::identifier_of(member);

        REQUIRE(name == "scale");
    });

    REQUIRE(matched);
    REQUIRE(calls == 1);
}

// ========================================
// Reflection metadata
// ========================================

TEST_CASE("YamlKeyDispatch callback receives nonstatic data member reflection",
          "[job_yaml][key_dispatch]")
{
    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("count", [&]<auto member> {
        constexpr std::string_view name = std::meta::identifier_of(member);

        STATIC_REQUIRE(std::meta::is_nonstatic_data_member(member));
        STATIC_REQUIRE(std::meta::has_identifier(member));
        STATIC_REQUIRE(std::meta::parent_of(member) == ^^KeyDispatchObject);

        REQUIRE(name == "count");
    }));
}

TEST_CASE("YamlKeyDispatch reflected members expose correct reflected types",
          "[job_yaml][key_dispatch]")
{
    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("count", [&]<auto member> {
        constexpr std::string_view name = std::meta::identifier_of(member);

        if constexpr (name == "count") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^int);
        } else if constexpr (name == "enabled") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^bool);
        } else if constexpr (name == "scale") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^float);
        } else {
            static_assert(false, "Unhandled KeyDispatchObject member");
        }

        REQUIRE(name == "count");
    }));

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto member> {
        constexpr std::string_view name = std::meta::identifier_of(member);

        if constexpr (name == "count") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^int);
        } else if constexpr (name == "enabled") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^bool);
        } else if constexpr (name == "scale") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^float);
        } else {
            static_assert(false, "Unhandled KeyDispatchObject member");
        }

        REQUIRE(name == "enabled");
    }));

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("scale", [&]<auto member> {
        constexpr std::string_view name = std::meta::identifier_of(member);

        if constexpr (name == "count") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^int);
        } else if constexpr (name == "enabled") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^bool);
        } else if constexpr (name == "scale") {
            STATIC_REQUIRE(std::meta::type_of(member) == ^^float);
        } else {
            static_assert(false, "Unhandled KeyDispatchObject member");
        }

        REQUIRE(name == "scale");
    }));
}

// ========================================
// Direct object member access
// ========================================

TEST_CASE("YamlKeyDispatch writes directly to reflected integer member",
          "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{};

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("count", [&]<auto member> {
        object.[:member:] = 42;
    });

    REQUIRE(matched);
    REQUIRE(object.count == 42);
    REQUIRE_FALSE(object.enabled);
    REQUIRE(object.scale == 0.0F);
}

TEST_CASE("YamlKeyDispatch writes directly to reflected boolean member",
          "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{};

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto member> {
        object.[:member:] = true;
    });

    REQUIRE(matched);
    REQUIRE(object.count == 0);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 0.0F);
}

TEST_CASE("YamlKeyDispatch writes directly to reflected floating point member",
          "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{};

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>("scale", [&]<auto member> {
        object.[:member:] = 2.5F;
    });

    REQUIRE(matched);
    REQUIRE(object.count == 0);
    REQUIRE_FALSE(object.enabled);
    REQUIRE(object.scale == 2.5F);
}

// ========================================
// Destination C++ type preservation
// ========================================

TEST_CASE("YamlKeyDispatch preserves destination types", "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{};

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("count", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;
        constexpr std::string_view name = std::meta::identifier_of(member);

        if constexpr (name == "count") {
            STATIC_REQUIRE(std::same_as<MemberType, int>);
        } else if constexpr (name == "enabled") {
            STATIC_REQUIRE(std::same_as<MemberType, bool>);
        } else if constexpr (name == "scale") {
            STATIC_REQUIRE(std::same_as<MemberType, float>);
        } else {
            static_assert(false, "Unhandled KeyDispatchObject member");
        }

        REQUIRE(name == "count");

        if constexpr (std::same_as<MemberType, int>) {
            object.[:member:] = 123;
        }
    }));

    REQUIRE(object.count == 123);

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;
        constexpr std::string_view name = std::meta::identifier_of(member);

        if constexpr (name == "count") {
            STATIC_REQUIRE(std::same_as<MemberType, int>);
        } else if constexpr (name == "enabled") {
            STATIC_REQUIRE(std::same_as<MemberType, bool>);
        } else if constexpr (name == "scale") {
            STATIC_REQUIRE(std::same_as<MemberType, float>);
        } else {
            static_assert(false, "Unhandled KeyDispatchObject member");
        }

        REQUIRE(name == "enabled");

        if constexpr (std::same_as<MemberType, bool>) {
            object.[:member:] = true;
        }
    }));

    REQUIRE(object.enabled);

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("scale", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;
        constexpr std::string_view name = std::meta::identifier_of(member);

        if constexpr (name == "count") {
            STATIC_REQUIRE(std::same_as<MemberType, int>);
        } else if constexpr (name == "enabled") {
            STATIC_REQUIRE(std::same_as<MemberType, bool>);
        } else if constexpr (name == "scale") {
            STATIC_REQUIRE(std::same_as<MemberType, float>);
        } else {
            static_assert(false, "Unhandled KeyDispatchObject member");
        }

        REQUIRE(name == "scale");

        if constexpr (std::same_as<MemberType, float>) {
            object.[:member:] = 4.25F;
        }
    }));

    REQUIRE(object.scale == 4.25F);
}

// ========================================
// Runtime key -> compile-time type routing
// ========================================

TEST_CASE("YamlKeyDispatch routes runtime keys into compile time destination types",
          "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{};

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("count", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

        if constexpr (std::same_as<MemberType, int>)
            object.[:member:] = 100;
        else if constexpr (std::same_as<MemberType, bool>)
            object.[:member:] = true;
        else if constexpr (std::same_as<MemberType, float>)
            object.[:member:] = 3.5F;
    }));

    REQUIRE(object.count == 100);
    REQUIRE_FALSE(object.enabled);
    REQUIRE(object.scale == 0.0F);

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

        if constexpr (std::same_as<MemberType, int>)
            object.[:member:] = 100;
        else if constexpr (std::same_as<MemberType, bool>)
            object.[:member:] = true;
        else if constexpr (std::same_as<MemberType, float>)
            object.[:member:] = 3.5F;
    }));

    REQUIRE(object.count == 100);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 0.0F);

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("scale", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

        if constexpr (std::same_as<MemberType, int>)
            object.[:member:] = 100;
        else if constexpr (std::same_as<MemberType, bool>)
            object.[:member:] = true;
        else if constexpr (std::same_as<MemberType, float>)
            object.[:member:] = 3.5F;
    }));

    REQUIRE(object.count == 100);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 3.5F);
}

// ========================================
// Existing object state
// ========================================

TEST_CASE("YamlKeyDispatch changes only selected object member",
          "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{
        .count = 7,
        .enabled = false,
        .scale = 1.25F,
    };

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled", [&]<auto member> {
        using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

        if constexpr (std::same_as<MemberType, bool>)
            object.[:member:] = true;
    }));

    REQUIRE(object.count == 7);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.25F);
}

TEST_CASE("YamlKeyDispatch failed lookup does not alter object",
          "[job_yaml][key_dispatch]")
{
    KeyDispatchObject object{
        .count = 7,
        .enabled = true,
        .scale = 1.25F,
    };

    bool called = false;

    REQUIRE_FALSE(YamlKeyDispatch::dispatch<KeyDispatchObject>("missing", [&]<auto> {
        called = true;
    }));

    REQUIRE_FALSE(called);
    REQUIRE(object.count == 7);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.25F);
}

// ========================================
// string_view behavior
// ========================================

TEST_CASE("YamlKeyDispatch accepts non null terminated string_view key",
          "[job_yaml][key_dispatch]")
{
    constexpr char source[] = {
        'x', 'x',
        'c', 'o', 'u', 'n', 't',
        'x', 'x'
    };

    const std::string_view key{source + 2, 5};

    bool called = false;

    const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>(key, [&]<auto member> {
        called = true;

        constexpr std::string_view name = std::meta::identifier_of(member);

        REQUIRE(name == "count");
    });

    REQUIRE(matched);
    REQUIRE(called);
}

TEST_CASE("YamlKeyDispatch respects string_view length", "[job_yaml][key_dispatch]")
{
    constexpr char source[] = "count-extra";
    const std::string_view key{source, 5};

    bool called = false;

    REQUIRE(YamlKeyDispatch::dispatch<KeyDispatchObject>(key, [&]<auto member> {
        called = true;

        constexpr std::string_view name = std::meta::identifier_of(member);

        REQUIRE(name == "count");
    }));

    REQUIRE(called);
}

TEST_CASE("YamlKeyDispatch rejects longer string_view sharing member prefix",
          "[job_yaml][key_dispatch]")
{
    constexpr char source[] = "count-extra";

    bool called = false;

    REQUIRE_FALSE(
        YamlKeyDispatch::dispatch<KeyDispatchObject>(std::string_view{source}, [&]<auto> {
            called = true;
        })
        );

    REQUIRE_FALSE(called);
}

// ========================================
// constexpr dispatch
// ========================================

constexpr bool constexprKeyDispatchCountTest()
{
    KeyDispatchObject object{};

    const bool matched =
        YamlKeyDispatch::dispatch<KeyDispatchObject>("count"sv, [&]<auto member> {
            using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

            if constexpr (std::same_as<MemberType, int>)
                object.[:member:] = 77;
        });

    return matched &&
           object.count == 77 &&
           !object.enabled &&
           object.scale == 0.0F;
}

constexpr bool constexprKeyDispatchEnabledTest()
{
    KeyDispatchObject object{};

    const bool matched =
        YamlKeyDispatch::dispatch<KeyDispatchObject>("enabled"sv, [&]<auto member> {
            using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

            if constexpr (std::same_as<MemberType, bool>)
                object.[:member:] = true;
        });

    return matched &&
           object.count == 0 &&
           object.enabled &&
           object.scale == 0.0F;
}

constexpr bool constexprKeyDispatchScaleTest()
{
    KeyDispatchObject object{};

    const bool matched =
        YamlKeyDispatch::dispatch<KeyDispatchObject>("scale"sv, [&]<auto member> {
            using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

            if constexpr (std::same_as<MemberType, float>)
                object.[:member:] = 2.5F;
        });

    return matched &&
           object.count == 0 &&
           !object.enabled &&
           object.scale == 2.5F;
}

constexpr bool constexprKeyDispatchMissingTest()
{
    KeyDispatchObject object{};

    const bool matched =
        YamlKeyDispatch::dispatch<KeyDispatchObject>("missing"sv, [&]<auto member> {
            using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

            if constexpr (std::same_as<MemberType, int>)
                object.[:member:] = 1;
            else if constexpr (std::same_as<MemberType, bool>)
                object.[:member:] = true;
            else if constexpr (std::same_as<MemberType, float>)
                object.[:member:] = 1.0F;
        });

    return !matched &&
           object.count == 0 &&
           !object.enabled &&
           object.scale == 0.0F;
}

static_assert(constexprKeyDispatchCountTest());
static_assert(constexprKeyDispatchEnabledTest());
static_assert(constexprKeyDispatchScaleTest());
static_assert(constexprKeyDispatchMissingTest());



#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlKeyDispatch benchmarks", "[job_yaml][key_dispatch][benchmark]")
{
    std::string_view key = "scale";
    benchmarkDoNotOptimize(key);
    BENCHMARK("YamlKeyDispatch reflected dispatch") {
        KeyDispatchObject object{};

        benchmarkDoNotOptimize(key);

        const bool matched = YamlKeyDispatch::dispatch<KeyDispatchObject>(key, [&]<auto member> {
            using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

            if constexpr (std::same_as<MemberType, int>) {
                object.[:member:] = 42;
            } else if constexpr (std::same_as<MemberType, bool>) {
                object.[:member:] = true;
            } else if constexpr (std::same_as<MemberType, float>) {
                object.[:member:] = 2.5F;
            }
        });

        benchmarkClobber(object);
        benchmarkDoNotOptimize(matched);

        return object.scale;
    };

    BENCHMARK("manual string_view dispatch") {
        KeyDispatchObject object{};
        benchmarkDoNotOptimize(key);

        if (key == "count") {
            object.count = 42;
        } else if (key == "enabled") {
            object.enabled = true;
        } else if (key == "scale") {
            object.scale = 2.5F;
        }

        benchmarkClobber(object);

        return object.scale;
    };
}
#endif

} // namespace job::yaml::tests