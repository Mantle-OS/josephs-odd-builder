#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "obj/job_obj.h"

using namespace job::core;

namespace {

struct GrammarTestType {};

} // namespace

TEST_CASE("job_obj grammar basic types")
{
    STATIC_REQUIRE(std::same_as<i8, std::int8_t>);
    STATIC_REQUIRE(std::same_as<i16, std::int16_t>);
    STATIC_REQUIRE(std::same_as<i32, std::int32_t>);
    STATIC_REQUIRE(std::same_as<i64, std::int64_t>);

    STATIC_REQUIRE(std::same_as<u8, std::uint8_t>);
    STATIC_REQUIRE(std::same_as<u16, std::uint16_t>);
    STATIC_REQUIRE(std::same_as<u32, std::uint32_t>);
    STATIC_REQUIRE(std::same_as<u64, std::uint64_t>);

    STATIC_REQUIRE(std::same_as<usize, std::size_t>);

    STATIC_REQUIRE(std::same_as<byte, std::byte>);
    STATIC_REQUIRE(std::same_as<bytes, std::vector<std::byte>>);

    STATIC_REQUIRE(std::same_as<str, std::string>);
    STATIC_REQUIRE(std::same_as<str_view, std::string_view>);
}

TEST_CASE("job_obj grammar optional")
{
    STATIC_REQUIRE(std::same_as<opt<i32>, std::optional<std::int32_t>>);
    STATIC_REQUIRE(std::same_as<opt<str>, std::optional<std::string>>);

    STATIC_REQUIRE(OptionalType<opt<i32>>);
    STATIC_REQUIRE(OptionalType<opt<str>>);
}

TEST_CASE("job_obj grammar pointer types")
{
    STATIC_REQUIRE(std::same_as<Ptr<GrammarTestType>, std::shared_ptr<GrammarTestType>>);
    STATIC_REQUIRE(std::same_as<WPtr<GrammarTestType>, std::weak_ptr<GrammarTestType>>);
    STATIC_REQUIRE(std::same_as<UPtr<GrammarTestType>, std::unique_ptr<GrammarTestType>>);

    STATIC_REQUIRE(SharedPointer<Ptr<GrammarTestType>>);
    STATIC_REQUIRE(WeakPointer<WPtr<GrammarTestType>>);
    STATIC_REQUIRE(UniquePointer<UPtr<GrammarTestType>>);

    STATIC_REQUIRE(OwningSmartPointer<Ptr<GrammarTestType>>);
    STATIC_REQUIRE(OwningSmartPointer<UPtr<GrammarTestType>>);
    STATIC_REQUIRE_FALSE(OwningSmartPointer<WPtr<GrammarTestType>>);

    STATIC_REQUIRE(UnsupportedPersistentPointer<WPtr<GrammarTestType>>);
}

TEST_CASE("job_obj grammar sequence containers")
{
    STATIC_REQUIRE(std::same_as<vec<i32>, std::vector<std::int32_t>>);
    STATIC_REQUIRE(std::same_as<list<i32>, std::vector<std::int32_t>>);
    STATIC_REQUIRE(std::same_as<array<i32, 4>, std::array<std::int32_t, 4>>);
    STATIC_REQUIRE(std::same_as<linklist<i32>, std::list<std::int32_t>>);

    STATIC_REQUIRE(ReflectableContainer<vec<i32>>);
    STATIC_REQUIRE(ReflectableContainer<list<i32>>);
    STATIC_REQUIRE(FixedSequenceContainer<array<i32, 4>>);
    STATIC_REQUIRE(PushBackSequenceContainer<linklist<i32>>);

    STATIC_REQUIRE(PersistentContainer<vec<i32>>);
    STATIC_REQUIRE(PersistentContainer<list<i32>>);
    STATIC_REQUIRE(PersistentContainer<array<i32, 4>>);
    STATIC_REQUIRE(PersistentContainer<linklist<i32>>);
}

TEST_CASE("job_obj grammar associative containers")
{
    STATIC_REQUIRE(std::same_as<set<i32>, std::set<std::int32_t>>);
    STATIC_REQUIRE(std::same_as<uset<i32>, std::unordered_set<std::int32_t>>);

    STATIC_REQUIRE(std::same_as<map<str, i32>, std::map<std::string, std::int32_t>>);
    STATIC_REQUIRE(std::same_as<umap<str, i32>, std::unordered_map<std::string, std::int32_t>>);

    STATIC_REQUIRE(SetContainer<set<i32>>);
    STATIC_REQUIRE(SetContainer<uset<i32>>);

    STATIC_REQUIRE(MapContainer<map<str, i32>>);
    STATIC_REQUIRE(MapContainer<umap<str, i32>>);

    STATIC_REQUIRE(PersistentContainer<set<i32>>);
    STATIC_REQUIRE(PersistentContainer<uset<i32>>);
    STATIC_REQUIRE(PersistentContainer<map<str, i32>>);
    STATIC_REQUIRE(PersistentContainer<umap<str, i32>>);
}

TEST_CASE("job_obj grammar semantic types")
{
    STATIC_REQUIRE(std::same_as<pub_key, std::array<byte, 32>>);
    STATIC_REQUIRE(std::same_as<pri_key, std::array<byte, 64>>);
    STATIC_REQUIRE(std::same_as<hash, std::array<byte, 32>>);
    STATIC_REQUIRE(std::same_as<signature, std::array<byte, 64>>);
    STATIC_REQUIRE(std::same_as<salt, std::array<byte, 16>>);

    STATIC_REQUIRE(sizeof(pub_key)      == 32);
    STATIC_REQUIRE(sizeof(pri_key)      == 64);
    STATIC_REQUIRE(sizeof(hash)         == 32);
    STATIC_REQUIRE(sizeof(signature)    == 64);
    STATIC_REQUIRE(sizeof(salt)         == 16);

    STATIC_REQUIRE(StdArrayType<pub_key>);
    STATIC_REQUIRE(StdArrayType<pri_key>);
    STATIC_REQUIRE(StdArrayType<hash>);
    STATIC_REQUIRE(StdArrayType<signature>);
    STATIC_REQUIRE(StdArrayType<salt>);

    STATIC_REQUIRE(stdArraySizeV<pub_key>   == 32);
    STATIC_REQUIRE(stdArraySizeV<pri_key>   == 64);
    STATIC_REQUIRE(stdArraySizeV<hash>      == 32);
    STATIC_REQUIRE(stdArraySizeV<signature> == 64);
    STATIC_REQUIRE(stdArraySizeV<salt>      == 16);
}