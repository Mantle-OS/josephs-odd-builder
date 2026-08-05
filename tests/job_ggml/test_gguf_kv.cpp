#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_enums.h>

#include <job_gguf_kv.h>

using namespace job::ggml;

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE(
    "GGUF key value factories create owned scalar values",
    "[gguf][kv][usage][factory]"
    )
{
    auto shared =
        JobGgufKv::createShared(
            "job.shared",
            std::uint32_t{42}
            );

    auto unique =
        JobGgufKv::createUniq(
            "job.unique",
            std::string{"glasses"}
            );

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);

    REQUIRE(shared->isValid());
    REQUIRE(unique->isValid());

    REQUIRE(shared->key() == "job.shared");
    REQUIRE(unique->key() == "job.unique");

    REQUIRE(
        shared->value<std::uint32_t>() ==
        std::uint32_t{42}
        );

    REQUIRE(
        unique->value<std::string>() ==
        "glasses"
        );
}

TEST_CASE(
    "GGUF key value stores every supported unsigned scalar type",
    "[gguf][kv][usage][scalar][unsigned]"
    )
{
    SECTION("uint8")
    {
        JobGgufKv value{
            "job.u8",
            std::uint8_t{8}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.key() == "job.u8");

        REQUIRE(value.isScalar());
        REQUIRE_FALSE(value.isArray());

        REQUIRE(
            value.serializedType() ==
            JobGgufType::UInt8
            );

        REQUIRE(
            value.type() ==
            JobGgufType::UInt8
            );

        REQUIRE(value.elementCount() == 1);

        REQUIRE(
            value.value<std::uint8_t>() ==
            std::uint8_t{8}
            );
    }

    SECTION("uint16")
    {
        JobGgufKv value{
            "job.u16",
            std::uint16_t{1600}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::UInt16
            );

        REQUIRE(
            value.value<std::uint16_t>() ==
            std::uint16_t{1600}
            );
    }

    SECTION("uint32")
    {
        JobGgufKv value{
            "job.u32",
            std::uint32_t{320000}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::UInt32
            );

        REQUIRE(
            value.value<std::uint32_t>() ==
            std::uint32_t{320000}
            );
    }

    SECTION("uint64")
    {
        JobGgufKv value{
            "job.u64",
            std::uint64_t{64000000}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::UInt64
            );

        REQUIRE(
            value.value<std::uint64_t>() ==
            std::uint64_t{64000000}
            );
    }
}

TEST_CASE(
    "GGUF key value stores every supported signed scalar type",
    "[gguf][kv][usage][scalar][signed]"
    )
{
    SECTION("int8")
    {
        JobGgufKv value{
            "job.i8",
            std::int8_t{-8}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::Int8
            );

        REQUIRE(
            value.value<std::int8_t>() ==
            std::int8_t{-8}
            );
    }

    SECTION("int16")
    {
        JobGgufKv value{
            "job.i16",
            std::int16_t{-1600}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::Int16
            );

        REQUIRE(
            value.value<std::int16_t>() ==
            std::int16_t{-1600}
            );
    }

    SECTION("int32")
    {
        JobGgufKv value{
            "job.i32",
            std::int32_t{-320000}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::Int32
            );

        REQUIRE(
            value.value<std::int32_t>() ==
            std::int32_t{-320000}
            );
    }

    SECTION("int64")
    {
        JobGgufKv value{
            "job.i64",
            std::int64_t{-64000000}
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::Int64
            );

        REQUIRE(
            value.value<std::int64_t>() ==
            std::int64_t{-64000000}
            );
    }
}

TEST_CASE(
    "GGUF key value stores floating point scalar types",
    "[gguf][kv][usage][scalar][floating]"
    )
{
    SECTION("float32")
    {
        JobGgufKv value{
            "job.f32",
            3.25f
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::Float32
            );

        REQUIRE(
            value.value<float>() ==
            3.25f
            );
    }

    SECTION("float64")
    {
        JobGgufKv value{
            "job.f64",
            6.5
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isScalar());

        REQUIRE(
            value.type() ==
            JobGgufType::Float64
            );

        REQUIRE(
            value.value<double>() ==
            6.5
            );
    }
}

TEST_CASE(
    "GGUF key value stores boolean scalar metadata",
    "[gguf][kv][usage][scalar][bool]"
    )
{
    JobGgufKv enabled{
        "job.enabled",
        true
    };

    JobGgufKv disabled{
        "job.disabled",
        false
    };

    REQUIRE(enabled.isValid());
    REQUIRE(disabled.isValid());

    REQUIRE(enabled.isScalar());
    REQUIRE(disabled.isScalar());

    REQUIRE(enabled.isBoolean());
    REQUIRE(disabled.isBoolean());

    REQUIRE_FALSE(enabled.isString());
    REQUIRE_FALSE(disabled.isString());

    REQUIRE(
        enabled.serializedType() ==
        JobGgufType::Bool
        );

    REQUIRE(
        enabled.type() ==
        JobGgufType::Bool
        );

    REQUIRE(enabled.value<bool>());
    REQUIRE_FALSE(disabled.value<bool>());
}

TEST_CASE(
    "GGUF key value stores string scalar metadata",
    "[gguf][kv][usage][scalar][string]"
    )
{
    JobGgufKv value{
        "general.name",
        std::string{
            "Joseph's Odd Builder"
        }
    };

    REQUIRE(value.isValid());

    REQUIRE(
        value.key() ==
        "general.name"
        );

    REQUIRE(value.isScalar());
    REQUIRE_FALSE(value.isArray());

    REQUIRE(value.isString());
    REQUIRE_FALSE(value.isBoolean());

    REQUIRE(
        value.serializedType() ==
        JobGgufType::String
        );

    REQUIRE(
        value.type() ==
        JobGgufType::String
        );

    REQUIRE(value.elementCount() == 1);

    REQUIRE(
        value.value<std::string>() ==
        "Joseph's Odd Builder"
        );
}

TEST_CASE(
    "GGUF key value stores unsigned integer arrays",
    "[gguf][kv][usage][array][unsigned]"
    )
{
    SECTION("uint8")
    {
        const std::vector<std::uint8_t> expected{
            1,
            2,
            3,
            4
        };

        JobGgufKv value{
            "job.u8.array",
            expected
        };

        REQUIRE(value.isValid());

        REQUIRE(value.isArray());
        REQUIRE_FALSE(value.isScalar());

        REQUIRE(
            value.serializedType() ==
            JobGgufType::Array
            );

        REQUIRE(
            value.type() ==
            JobGgufType::UInt8
            );

        REQUIRE(
            value.elementCount() ==
            expected.size()
            );

        REQUIRE(
            value.values<std::uint8_t>() ==
            expected
            );
    }

    SECTION("uint16")
    {
        const std::vector<std::uint16_t> expected{
            16,
            32,
            64,
            128
        };

        JobGgufKv value{
            "job.u16.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::UInt16
            );

        REQUIRE(
            value.values<std::uint16_t>() ==
            expected
            );
    }

    SECTION("uint32")
    {
        const std::vector<std::uint32_t> expected{
            64,
            128,
            256,
            512
        };

        JobGgufKv value{
            "job.u32.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::UInt32
            );

        REQUIRE(
            value.values<std::uint32_t>() ==
            expected
            );
    }

    SECTION("uint64")
    {
        const std::vector<std::uint64_t> expected{
            1024,
            2048,
            4096,
            8192
        };

        JobGgufKv value{
            "job.u64.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::UInt64
            );

        REQUIRE(
            value.values<std::uint64_t>() ==
            expected
            );
    }
}

TEST_CASE(
    "GGUF key value stores signed integer arrays",
    "[gguf][kv][usage][array][signed]"
    )
{
    SECTION("int8")
    {
        const std::vector<std::int8_t> expected{
            -4,
            -2,
            0,
            2,
            4
        };

        JobGgufKv value{
            "job.i8.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::Int8
            );

        REQUIRE(
            value.values<std::int8_t>() ==
            expected
            );
    }

    SECTION("int16")
    {
        const std::vector<std::int16_t> expected{
            -128,
            -64,
            64,
            128
        };

        JobGgufKv value{
            "job.i16.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::Int16
            );

        REQUIRE(
            value.values<std::int16_t>() ==
            expected
            );
    }

    SECTION("int32")
    {
        const std::vector<std::int32_t> expected{
            -4096,
            -2048,
            2048,
            4096
        };

        JobGgufKv value{
            "job.i32.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::Int32
            );

        REQUIRE(
            value.values<std::int32_t>() ==
            expected
            );
    }

    SECTION("int64")
    {
        const std::vector<std::int64_t> expected{
            -64000000,
            -32000000,
            32000000,
            64000000
        };

        JobGgufKv value{
            "job.i64.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::Int64
            );

        REQUIRE(
            value.values<std::int64_t>() ==
            expected
            );
    }
}

TEST_CASE(
    "GGUF key value stores floating point arrays",
    "[gguf][kv][usage][array][floating]"
    )
{
    SECTION("float32")
    {
        const std::vector<float> expected{
            -1.5f,
            0.0f,
            1.5f,
            3.0f
        };

        JobGgufKv value{
            "job.f32.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::Float32
            );

        REQUIRE(
            value.values<float>() ==
            expected
            );
    }

    SECTION("float64")
    {
        const std::vector<double> expected{
            -3.25,
            0.0,
            3.25,
            6.5
        };

        JobGgufKv value{
            "job.f64.array",
            expected
        };

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(
            value.type() ==
            JobGgufType::Float64
            );

        REQUIRE(
            value.values<double>() ==
            expected
            );
    }
}

TEST_CASE(
    "GGUF key value stores boolean arrays",
    "[gguf][kv][usage][array][bool]"
    )
{
    const std::vector<bool> expected{
        true,
        false,
        true,
        true,
        false
    };

    JobGgufKv value{
        "job.bool.array",
        expected
    };

    REQUIRE(value.isValid());

    REQUIRE(value.isArray());
    REQUIRE_FALSE(value.isScalar());

    REQUIRE(value.isBoolean());
    REQUIRE_FALSE(value.isString());

    REQUIRE(
        value.serializedType() ==
        JobGgufType::Array
        );

    REQUIRE(
        value.type() ==
        JobGgufType::Bool
        );

    REQUIRE(
        value.elementCount() ==
        expected.size()
        );

    REQUIRE(
        value.values<bool>() ==
        expected
        );
}

TEST_CASE("GGUF key value stores string arrays", "[gguf][kv][usage][array][string]")
{
    const std::vector<std::string> expected{
        "alpha",
        "beta",
        "gamma",
        "delta"
    };

    JobGgufKv value{ "job.string.array", expected };

    REQUIRE(value.isValid());
    REQUIRE(value.isArray());
    REQUIRE_FALSE(value.isScalar());
    REQUIRE(value.isString());
    REQUIRE_FALSE(value.isBoolean());
    REQUIRE(value.serializedType() == JobGgufType::Array);
    REQUIRE(value.type() == JobGgufType::String);
    REQUIRE(value.elementCount() == expected.size());
    REQUIRE(value.values<std::string>() == expected);
}

// Block two: edge cases / contracts
TEST_CASE("GGUF key value rejects an empty key", "[gguf][kv][edge][key]")
{
    REQUIRE_THROWS_AS((JobGgufKv{ "",
                                 std::uint32_t{42}}),
                      std::invalid_argument
                      );

    REQUIRE_THROWS_AS((JobGgufKv{
                                 std::string{},
                                 std::string{"value"}}),
                      std::invalid_argument
                      );

    REQUIRE_THROWS_AS((JobGgufKv{
                          "",
                          std::vector<std::uint32_t>{ 1, 2, 3 }
                      }), std::invalid_argument
                      );
}

TEST_CASE("GGUF key value supports empty typed arrays", "[gguf][kv][edge][array][empty]")
{
    SECTION("fixed-width array")
    {
        JobGgufKv value{"job.empty.u32", std::vector<std::uint32_t>{}};
        REQUIRE(value.isValid());
        REQUIRE(value.isArray());

        REQUIRE(value.serializedType() == JobGgufType::Array);

        REQUIRE(value.type() == JobGgufType::UInt32);
        REQUIRE(value.elementCount() == 0);
        REQUIRE(value.values<std::uint32_t>().empty());
    }

    SECTION("string array")
    {
        JobGgufKv value{"job.empty.string", std::vector<std::string>{}};

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());
        REQUIRE(value.isString());

        REQUIRE(value.type() == JobGgufType::String);
        REQUIRE(value.elementCount() == 0);
        REQUIRE(value.values<std::string>().empty());
    }

    SECTION("boolean array")
    {
        JobGgufKv value{"job.empty.bool", std::vector<bool>{}};

        REQUIRE(value.isValid());
        REQUIRE(value.isArray());
        REQUIRE(value.isBoolean());

        REQUIRE(value.type() == JobGgufType::Bool);
        REQUIRE(value.elementCount() == 0);
        REQUIRE(value.values<bool>().empty());
    }
}

TEST_CASE("GGUF scalar value rejects mismatched typed access", "[gguf][kv][edge][scalar][type]")
{
    JobGgufKv value{ "job.value", std::uint32_t{42} };

    REQUIRE_THROWS_AS(value.value<std::int32_t>(), std::invalid_argument);
    REQUIRE_THROWS_AS(value.value<std::uint64_t>(), std::invalid_argument);
    REQUIRE_THROWS_AS(value.value<float>(), std::invalid_argument);
    REQUIRE_THROWS_AS(value.value<std::string>(), std::invalid_argument);
}

TEST_CASE("GGUF array value rejects mismatched typed access", "[gguf][kv][edge][array][type]")
{
    JobGgufKv value{ "job.values", std::vector<std::uint32_t>{ 1, 2, 3, 4 }};
    REQUIRE_THROWS_AS(value.values<std::int32_t>(), std::invalid_argument);
    REQUIRE_THROWS_AS(value.values<std::uint64_t>(), std::invalid_argument);
    REQUIRE_THROWS_AS(value.values<float>(), std::invalid_argument);
    REQUIRE_THROWS_AS(value.values<std::string>(), std::invalid_argument);
}

TEST_CASE("GGUF scalar value rejects array access", "[gguf][kv][edge][shape]")
{
    JobGgufKv value{ "job.scalar", std::uint32_t{42} };
    REQUIRE(value.isScalar());
    REQUIRE_FALSE(value.isArray());
    REQUIRE_THROWS_AS(value.values<std::uint32_t>(), std::invalid_argument);
}

TEST_CASE("GGUF array value rejects scalar access", "[gguf][kv][edge][shape]")
{
    JobGgufKv value{"job.array", std::vector<std::uint32_t>{ 1, 2, 3 }};

    REQUIRE(value.isArray());
    REQUIRE_FALSE(value.isScalar());

    REQUIRE_THROWS_AS(value.value<std::uint32_t>(), std::invalid_argument);
}

TEST_CASE("GGUF string shape is independent from scalar or array storage", "[gguf][kv][edge][string][shape]")
{
    JobGgufKv scalar{"job.string.scalar", std::string{"alpha"} };
    JobGgufKv array{"job.string.array", std::vector<std::string>{ "alpha", "beta" }};

    REQUIRE(scalar.isString());
    REQUIRE(array.isString());

    REQUIRE(scalar.isScalar());
    REQUIRE_FALSE(scalar.isArray());

    REQUIRE(array.isArray());
    REQUIRE_FALSE(array.isScalar());

    REQUIRE(scalar.serializedType() == JobGgufType::String);
    REQUIRE(array.serializedType() == JobGgufType::Array);
    REQUIRE(scalar.type() == JobGgufType::String);
    REQUIRE(array.type() == JobGgufType::String);
}

TEST_CASE("GGUF boolean shape is independent from scalar or array storage", "[gguf][kv][edge][bool][shape]")
{
    JobGgufKv scalar{ "job.bool.scalar", true };
    JobGgufKv array{ "job.bool.array", std::vector<bool>{ true, false } };

    REQUIRE(scalar.isBoolean());
    REQUIRE(array.isBoolean());

    REQUIRE(scalar.isScalar());
    REQUIRE_FALSE(scalar.isArray());

    REQUIRE(array.isArray());
    REQUIRE_FALSE(array.isScalar());

    REQUIRE(scalar.serializedType() == JobGgufType::Bool);
    REQUIRE(array.serializedType() == JobGgufType::Array);

    REQUIRE(scalar.type() == JobGgufType::Bool);
    REQUIRE(array.type() == JobGgufType::Bool);
}

TEST_CASE("GGUF key value retains independent owned data", "[gguf][kv][edge][ownership]")
{
    std::string scalarSource{ "original scalar" };
    std::vector<std::uint32_t> arraySource{ 1, 2, 3, 4 };
    std::vector<std::string> stringArraySource{ "alpha", "beta", "gamma" };
    JobGgufKv scalar{ "job.scalar", scalarSource };
    JobGgufKv array{ "job.array", arraySource };
    JobGgufKv stringArray{ "job.string.array", stringArraySource };

    scalarSource = "changed";

    arraySource[0] = 100;
    arraySource.clear();

    stringArraySource[0] = "changed";
    stringArraySource.clear();

    REQUIRE(scalar.value<std::string>() == "original scalar");

    REQUIRE(array.values<std::uint32_t>() == std::vector<std::uint32_t>{ 1, 2, 3, 4 });
    REQUIRE(stringArray.values<std::string>() == std::vector<std::string>{ "alpha", "beta", "gamma" });
}

TEST_CASE("GGUF key value handles numeric boundary values", "[gguf][kv][edge][numeric]")
{
    SECTION("unsigned boundaries")
    {
        JobGgufKv minimum{"job.u64.minimum", std::uint64_t{0} };
        JobGgufKv maximum{ "job.u64.maximum", UINT64_MAX };
        REQUIRE( minimum.value<std::uint64_t>() == std::uint64_t{0} );
        REQUIRE( maximum.value<std::uint64_t>() == UINT64_MAX );
    }

    SECTION("signed boundaries")
    {
        JobGgufKv minimum{ "job.i64.minimum", INT64_MIN };
        JobGgufKv maximum{ "job.i64.maximum", INT64_MAX };

        REQUIRE( minimum.value<std::int64_t>() == INT64_MIN );
        REQUIRE( maximum.value<std::int64_t>() == INT64_MAX );
    }

    SECTION("floating signs")
    {
        JobGgufKv negativeZero{ "job.f32.negative_zero", -0.0f };
        JobGgufKv positiveZero{ "job.f32.positive_zero", 0.0f };

        REQUIRE( negativeZero.value<float>() == -0.0f );
        REQUIRE( positiveZero.value<float>() == 0.0f );
    }
}


// Block three: benchmarks / stress
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE(
    "GGUF scalar key value construction performance",
    "[gguf][kv][benchmark][construction][scalar]"
    )
{
    BENCHMARK(
        "construct one uint64 GGUF value"
        )
    {
        return JobGgufKv{
            "job.benchmark.scalar",
            std::uint64_t{42}
        };
    };
}

TEST_CASE("GGUF string key value construction performance", "[gguf][kv][benchmark][construction][string]")
{
    const std::string source{"Joseph's Odd Builder GGUF benchmark value"};

    BENCHMARK("construct one string GGUF value"){
        return JobGgufKv{ "job.benchmark.string", source };
    };
}

TEST_CASE("GGUF numeric array construction performance", "[gguf][kv][benchmark][construction][array]")
{
    std::vector<std::uint32_t> values(1024);

    for (std::size_t index = 0; index < values.size(); ++index)
        values[index] = static_cast<std::uint32_t>(index);


    BENCHMARK("construct one 1024 element GGUF array"){
        return JobGgufKv{"job.benchmark.array", values};
    };
}

TEST_CASE("GGUF string array construction performance", "[gguf][kv][benchmark][construction][string_array]")
{
    std::vector<std::string> values;
    values.reserve(256);

    for (std::size_t index = 0; index < 256; ++index)
        values.push_back("job.value." + std::to_string(index));


    BENCHMARK("construct one 256 element GGUF string array"){
        return JobGgufKv{ "job.benchmark.string_array", values };
    };
}

TEST_CASE("GGUF scalar typed access performance", "[gguf][kv][benchmark][access][scalar]")
{
    JobGgufKv value{"job.benchmark.scalar", std::uint64_t{42}};

    BENCHMARK("read one GGUF scalar value"){
        return value.value<std::uint64_t>();
    };
}

TEST_CASE("GGUF numeric array reconstruction performance", "[gguf][kv][benchmark][access][array]")
{
    std::vector<std::uint32_t> values(1024);

    for (std::size_t index = 0; index < values.size(); ++index)
        values[index] = static_cast<std::uint32_t>(index);

    JobGgufKv value{ "job.benchmark.array", values};

    BENCHMARK("reconstruct one 1024 element GGUF array"){
        return value.values<std::uint32_t>();
    };
}

TEST_CASE("GGUF string array reconstruction performance", "[gguf][kv][benchmark][access][string_array]")
{
    std::vector<std::string> values;
    values.reserve(256);

    for (std::size_t index = 0; index < 256; ++index)
        values.push_back("job.value." + std::to_string(index));

    JobGgufKv value{ "job.benchmark.string_array", values };
    BENCHMARK("reconstruct one 256 element GGUF string array"){
        return value.values<std::string>();
    };
}

#endif