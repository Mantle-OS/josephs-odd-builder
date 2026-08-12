#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <gguf.h>

#include <job_ggml_enums.h>
#include <job_gguf_type_traits.h>

using namespace job::ggml;

namespace {

template<typename T>
concept AcceptedGgufValue = JobGgufValueType<T>;

template<typename T>
concept RejectedGgufValue = !JobGgufValueType<T>;

struct UnsupportedValue
{
    std::uint32_t first{0};
    std::uint32_t second{0};
};

enum class UnsupportedEnum : std::uint32_t { Value = 1 };

} // namespace

// ============================================================================
// Compile-time contracts
// ============================================================================

static_assert(AcceptedGgufValue<std::uint8_t>);
static_assert(AcceptedGgufValue<std::int8_t>);

static_assert(AcceptedGgufValue<std::uint16_t>);
static_assert(AcceptedGgufValue<std::int16_t>);

static_assert(AcceptedGgufValue<std::uint32_t>);
static_assert(AcceptedGgufValue<std::int32_t>);

static_assert(AcceptedGgufValue<std::uint64_t>);
static_assert(AcceptedGgufValue<std::int64_t>);

static_assert(AcceptedGgufValue<float>);
static_assert(AcceptedGgufValue<double>);

static_assert(AcceptedGgufValue<bool>);
static_assert(AcceptedGgufValue<std::string>);

static_assert(AcceptedGgufValue<const std::uint32_t>);
static_assert(AcceptedGgufValue<volatile std::uint32_t>);
static_assert(AcceptedGgufValue<const volatile std::uint32_t>);

static_assert(AcceptedGgufValue<std::uint32_t &>);
static_assert(AcceptedGgufValue<const std::uint32_t &>);
static_assert(AcceptedGgufValue<std::uint32_t &&>);

static_assert(AcceptedGgufValue<const std::string>);
static_assert(AcceptedGgufValue<const std::string &>);
static_assert(AcceptedGgufValue<std::string &&>);

static_assert(RejectedGgufValue<char>);
static_assert(RejectedGgufValue<wchar_t>);
static_assert(RejectedGgufValue<char16_t>);
static_assert(RejectedGgufValue<char32_t>);

static_assert(RejectedGgufValue<long double>);

static_assert(RejectedGgufValue<std::byte>);
static_assert(RejectedGgufValue<void *>);
static_assert(RejectedGgufValue<const void *>);

static_assert(RejectedGgufValue<UnsupportedValue>);
static_assert(RejectedGgufValue<UnsupportedEnum>);

static_assert(RejectedGgufValue<std::vector<std::uint32_t>>);
static_assert(RejectedGgufValue<std::vector<std::string>>);

static_assert(JobGgufTypeTraits::typeFor<std::uint8_t>() == JobGgufType::UInt8);
static_assert(
    JobGgufTypeTraits::typeFor<std::int8_t>() ==
    JobGgufType::Int8
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::uint16_t>() ==
    JobGgufType::UInt16
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::int16_t>() ==
    JobGgufType::Int16
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::uint32_t>() ==
    JobGgufType::UInt32
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::int32_t>() ==
    JobGgufType::Int32
    );

static_assert(
    JobGgufTypeTraits::typeFor<float>() ==
    JobGgufType::Float32
    );

static_assert(
    JobGgufTypeTraits::typeFor<bool>() ==
    JobGgufType::Bool
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::string>() ==
    JobGgufType::String
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::uint64_t>() ==
    JobGgufType::UInt64
    );

static_assert(
    JobGgufTypeTraits::typeFor<std::int64_t>() ==
    JobGgufType::Int64
    );

static_assert(
    JobGgufTypeTraits::typeFor<double>() ==
    JobGgufType::Float64
    );


// Block one: usage / examples
TEST_CASE("GGUF type traits map unsigned integer types", "[gguf][type_traits][usage][unsigned]")
{
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint8_t>()  == JobGgufType::UInt8);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint16_t>() == JobGgufType::UInt16);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint32_t>() == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint64_t>() == JobGgufType::UInt64);
}

TEST_CASE("GGUF type traits map signed integer types", "[gguf][type_traits][usage][signed]")
{
    REQUIRE(JobGgufTypeTraits::typeFor<std::int8_t>() == JobGgufType::Int8);
    REQUIRE(JobGgufTypeTraits::typeFor<std::int16_t>() == JobGgufType::Int16);
    REQUIRE(JobGgufTypeTraits::typeFor<std::int32_t>() == JobGgufType::Int32);
    REQUIRE(JobGgufTypeTraits::typeFor<std::int64_t>() == JobGgufType::Int64);
}

TEST_CASE("GGUF type traits map floating point types", "[gguf][type_traits][usage][floating]")
{
    REQUIRE(JobGgufTypeTraits::typeFor<float>() == JobGgufType::Float32);
    REQUIRE(JobGgufTypeTraits::typeFor<double>() == JobGgufType::Float64);
}

TEST_CASE("GGUF type traits map boolean and string types", "[gguf][type_traits][usage][special]")
{
    REQUIRE(JobGgufTypeTraits::typeFor<bool>() == JobGgufType::Bool);
    REQUIRE(JobGgufTypeTraits::typeFor<std::string>() == JobGgufType::String);
}

TEST_CASE("GGUF type traits remove cv and reference qualifiers", "[gguf][type_traits][usage][qualifiers]")
{
    REQUIRE(JobGgufTypeTraits::typeFor<const std::uint32_t>()           == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<volatile std::uint32_t>()        == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<const volatile std::uint32_t>()  == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint32_t &>()               == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<const std::uint32_t &>()         == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint32_t &&>()              == JobGgufType::UInt32);
    REQUIRE(JobGgufTypeTraits::typeFor<const std::string &>()           == JobGgufType::String);
}

TEST_CASE("GGUF type traits map directly to native GGUF types", "[gguf][type_traits][usage][native]")
{
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::uint8_t>())     == GGUF_TYPE_UINT8);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::int8_t>())      == GGUF_TYPE_INT8);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::uint16_t>())    == GGUF_TYPE_UINT16);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::int16_t>())     == GGUF_TYPE_INT16);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::uint32_t>())    == GGUF_TYPE_UINT32);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::int32_t>())     == GGUF_TYPE_INT32);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<float>())            == GGUF_TYPE_FLOAT32);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<bool>())             == GGUF_TYPE_BOOL);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::string>())      == GGUF_TYPE_STRING);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::uint64_t>())    == GGUF_TYPE_UINT64);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<std::int64_t>())     == GGUF_TYPE_INT64);
    REQUIRE(toGgufType( JobGgufTypeTraits::typeFor<double>())           == GGUF_TYPE_FLOAT64);
}

TEST_CASE("GGUF type traits agree with fixed width type sizes", "[gguf][type_traits][usage][size]")
{
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::uint8_t>())   == sizeof(std::uint8_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::int8_t>())    == sizeof(std::int8_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::uint16_t>())  == sizeof(std::uint16_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::int16_t>())   == sizeof(std::int16_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::uint32_t>())  == sizeof(std::uint32_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::int32_t>())   == sizeof(std::int32_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<float>())          == sizeof(float) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<bool>())           == sizeof(std::int8_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::uint64_t>())  == sizeof(std::uint64_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::int64_t>())   == sizeof(std::int64_t) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<double>())         == sizeof(double) );
    REQUIRE(ggufTypeSize(JobGgufTypeTraits::typeFor<std::string>())    == 0 );
}

TEST_CASE("GGUF type traits agree with type names", "[gguf][type_traits][usage][name]")
{
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::uint8_t>())      == "u8");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::int8_t>())       == "i8");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::uint16_t>())     == "u16");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::int16_t>())      == "i16");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::uint32_t>())     == "u32");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::int32_t>())      == "i32");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<float>())             == "f32");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<bool>())              == "bool");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::string>())       == "str");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::uint64_t>())     == "u64");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<std::int64_t>())      == "i64");
    REQUIRE( ggufTypeName(JobGgufTypeTraits::typeFor<double>())            == "f64");
}

// Block two: edge cases / contracts
TEST_CASE("GGUF value concept accepts exactly the supported primitive types", "[gguf][type_traits][edge][concept]")
{
    STATIC_REQUIRE(JobGgufValueType<std::uint8_t>);
    STATIC_REQUIRE(JobGgufValueType<std::int8_t>);
    STATIC_REQUIRE(JobGgufValueType<std::uint16_t>);
    STATIC_REQUIRE(JobGgufValueType<std::int16_t>);
    STATIC_REQUIRE(JobGgufValueType<std::uint32_t>);
    STATIC_REQUIRE(JobGgufValueType<std::int32_t>);
    STATIC_REQUIRE(JobGgufValueType<std::uint64_t>);
    STATIC_REQUIRE(JobGgufValueType<std::int64_t>);
    STATIC_REQUIRE(JobGgufValueType<float>);
    STATIC_REQUIRE(JobGgufValueType<double>);
    STATIC_REQUIRE(JobGgufValueType<bool>);
    STATIC_REQUIRE(JobGgufValueType<std::string>);
}

TEST_CASE("GGUF value concept rejects unsupported primitive and object types", "[gguf][type_traits][edge][concept][invalid]")
{
    STATIC_REQUIRE_FALSE(JobGgufValueType<char>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<wchar_t>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<char16_t>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<char32_t>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<long double>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<std::byte>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<void *>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<UnsupportedValue>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<UnsupportedEnum>);
}

TEST_CASE("GGUF value concept rejects containers as scalar element types", "[gguf][type_traits][edge][concept][container]")
{
    STATIC_REQUIRE_FALSE(JobGgufValueType<std::vector<std::uint32_t>>);
    STATIC_REQUIRE_FALSE(JobGgufValueType<std::vector<std::string>>);

    /*
     * The vector is accepted by JobGgufKv's array constructor because its
     * element type satisfies JobGgufValueType. The vector itself is not a
     * primitive GGUF value type.
     */
    STATIC_REQUIRE(JobGgufValueType<std::uint32_t>);
    STATIC_REQUIRE(JobGgufValueType<std::string>);
}

TEST_CASE("GGUF type mapping remains stable across qualifiers", "[gguf][type_traits][edge][qualifiers]")
{
    constexpr JobGgufType plainType = JobGgufTypeTraits::typeFor<std::uint32_t>();
    REQUIRE(JobGgufTypeTraits::typeFor<const std::uint32_t>()           == plainType);
    REQUIRE(JobGgufTypeTraits::typeFor<volatile std::uint32_t>()        == plainType);
    REQUIRE(JobGgufTypeTraits::typeFor<const volatile std::uint32_t>()  == plainType);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint32_t &>()               == plainType);
    REQUIRE(JobGgufTypeTraits::typeFor<const std::uint32_t &>()         == plainType);
    REQUIRE(JobGgufTypeTraits::typeFor<std::uint32_t &&>()              == plainType);
}

TEST_CASE("Every mapped GGUF value type is valid", "[gguf][type_traits][edge][validity]")
{
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::uint8_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::int8_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::uint16_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::int16_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::uint32_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::int32_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<float>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<bool>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::string>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::uint64_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<std::int64_t>()));
    REQUIRE(isValidGgufType(JobGgufTypeTraits::typeFor<double>()));
}


// Block three: benchmarks / stress
#ifdef JOB_TEST_BENCHMARKS

// This is hard to test because well the design off off load to the compiler is well ....
// If we tested this as just the "normal" way to do this the compiler would remove(compile out)
// JobGgufTypeTraits::typeFor<std::uint32_t>() -> mov eax, 4 ret -> Actually... I don't even need that. Entire benchmark deleted.
// because the compiler now knows: array size, array contents, enum value, index at compile time.
// There's literally nothing left to execute. -> Benchmark failed (could not measure benchmark, maybe it was optimized away)
// Thoughts:  "Performance: Infinite" :P
//
// So this is our best attempt at still getting benchmarks with out the compiler doing what it is meant to do.
TEST_CASE("GGUF type size lookup performance", "[gguf][type_traits][benchmark][size]")
{
    volatile std::int32_t nativeValue = static_cast<std::int32_t>(GGUF_TYPE_UINT64);
    BENCHMARK("lookup one runtime GGUF type size"){
        const auto type = static_cast<JobGgufType>(nativeValue);
        return ggufTypeSize(type);
    };
}

TEST_CASE("GGUF type name lookup performance", "[gguf][type_traits][benchmark][name]")
{
    volatile std::int32_t nativeValue = static_cast<std::int32_t>(GGUF_TYPE_STRING);
    BENCHMARK("lookup one runtime GGUF type name"){
        const auto type = static_cast<JobGgufType>(nativeValue);
        return ggufTypeName(type);
    };
}
#endif