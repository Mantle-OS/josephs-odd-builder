#include <catch2/catch_all.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <job_obj_hash.h>
#include <job_siphash.h>

#if defined(__GLIBCXX__) || defined(__GLIBCXX_HAVE_BUILTIN_TRAITS)
namespace std {
// Tell libstdc++ that JobSipHash is "heavy", so it should cache hash codes in map nodes
template<>
struct __is_fast_hash<job::core::JobSipHash> : public std::false_type {};
}
#endif

using namespace job::core;

namespace {

[[nodiscard]] std::string makeUid16(std::uint64_t id)
{
    char buf[17]{};
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(id));
    return std::string(buf, 16);
}

[[nodiscard]] std::string makeUidFallback(std::uint64_t id)
{
    return "fallback-object-" + std::to_string(id);
}

struct JobSipHashCachedTest
{
    JobSipHash hash;
    explicit JobSipHashCachedTest(const JobSipHash &value) : hash(value) {}
    [[nodiscard]] std::size_t operator()(const std::string &key) const { return hash(key); }
};

class TestObject
{
public:
    explicit TestObject(std::string uid, int value = 0) : m_uid(std::move(uid)), m_value(value) { ++s_alive; }
    ~TestObject() { --s_alive; ++s_destroyed; }

    TestObject(const TestObject &) = delete;
    TestObject &operator=(const TestObject &) = delete;
    TestObject(TestObject &&) = delete;
    TestObject &operator=(TestObject &&) = delete;

    [[nodiscard]] const std::string &uid() const noexcept { return m_uid; }
    [[nodiscard]] int value() const noexcept { return m_value; }
    void setValue(int value) noexcept { m_value = value; }

    [[nodiscard]] static int alive() noexcept { return s_alive; }
    [[nodiscard]] static int destroyed() noexcept { return s_destroyed; }
    static void resetCounters() noexcept { s_alive = 0; s_destroyed = 0; }

private:
    const std::string m_uid;
    int m_value{0};
    inline static int s_alive{0};
    inline static int s_destroyed{0};
};

using FastObjectHash    = JobObjHashFast<TestObject *>;
using FastHash          = FastObjectHash::hash_type;

[[nodiscard]] JobSipHash makeHash(bool useAvx = JOB_DEFAULT_USE_AVX)
{
    return JobSipHash{UINT64_C(0x0706050403020100), UINT64_C(0x0f0e0d0c0b0a0908), useAvx};
}

static_assert(JobObject<TestObject>);
static_assert(JobObjectPointer<TestObject *>);
static_assert(JobObjectPointer<std::shared_ptr<TestObject>>);
static_assert(JobObjectPointer<std::unique_ptr<TestObject>>);
static_assert(!JobObjectPointer<TestObject>);

static_assert(noexcept(std::declval<const JobSipHash &>()(std::declval<const std::string &>())));
static_assert(!noexcept(std::declval<const JobSipHashCachedTest &>()(std::declval<const std::string &>())));

} // namespace

// 1 How do I use JobObjHash?
TEST_CASE("JobObjHash stores raw pointer objects by uid", "[core][container][objhash]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{}; // defaults to sipHash

    hash.insert(new TestObject{"one", 10});
    hash.insert(new TestObject{"two", 20});
    hash.insert(new TestObject{"three", 30});

    REQUIRE(hash.size() == 3);
    REQUIRE(hash.count() == 3);

    REQUIRE(hash.contains("one"));
    REQUIRE(hash.contains("two"));
    REQUIRE(hash.contains("three"));

    REQUIRE(hash.at("one")->value() == 10);
    REQUIRE(hash.at("two")->value() == 20);
    REQUIRE(hash.at("three")->value() == 30);

    REQUIRE(TestObject::alive() == 3);
}

TEST_CASE("JobObjHash stores unique pointer objects by uid", "[core][container][objhash]")
{
    TestObject::resetCounters();

    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one", 10));
    hash.insert(std::make_unique<TestObject>("two", 20));

    REQUIRE(hash.size() == 2);
    REQUIRE(hash.at("one")->value() == 10);
    REQUIRE(hash.at("two")->value() == 20);

    REQUIRE(TestObject::alive() == 2);
}

TEST_CASE("JobObjHash stores shared pointer objects by uid", "[core][container][objhash]")
{
    TestObject::resetCounters();

    JobObjHash<std::shared_ptr<TestObject>> hash{};

    auto one = std::make_shared<TestObject>("one", 10);
    auto two = std::make_shared<TestObject>("two", 20);

    hash.insert(one);
    hash.insert(two);

    REQUIRE(hash.size() == 2);

    REQUIRE(hash.at("one") == one.get());
    REQUIRE(hash.at("two") == two.get());

    REQUIRE(one.use_count() == 2);
    REQUIRE(two.use_count() == 2);
}

TEST_CASE("JobObjHash lookup returns mutable objects", "[core][container][objhash]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("object", 10));

    TestObject *object = hash.at("object");

    REQUIRE(object);
    REQUIRE(object->value() == 10);

    object->setValue(42);

    REQUIRE(hash.at("object")->value() == 42);
}

TEST_CASE("Const JobObjHash lookup returns const objects", "[core][container][objhash]")
{
    JobObjHash<std::unique_ptr<TestObject>> mutableHash{};

    mutableHash.insert(std::make_unique<TestObject>("object", 42));

    const JobObjHash<std::unique_ptr<TestObject>> &hash = mutableHash;

    const TestObject *object = hash.at("object");

    REQUIRE(object);
    REQUIRE(object->uid() == "object");
    REQUIRE(object->value() == 42);
}

TEST_CASE("JobObjHash can find objects by uid", "[core][container][objhash]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one", 10));
    hash.insert(std::make_unique<TestObject>("two", 20));

    auto it = hash.find("two");

    REQUIRE(it != hash.end());
    REQUIRE(it->first == "two");
    REQUIRE(it->second);
    REQUIRE(it->second->value() == 20);

    REQUIRE(hash.find("missing") == hash.end());
}

TEST_CASE("JobObjHash supports range based iteration", "[core][container][objhash]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one", 10));
    hash.insert(std::make_unique<TestObject>("two", 20));
    hash.insert(std::make_unique<TestObject>("three", 30));

    int total = 0;
    std::size_t count = 0;

    for (const auto &[uid, object] : hash) {
        REQUIRE(object);
        REQUIRE(object->uid() == uid);

        total += object->value();
        ++count;
    }

    REQUIRE(count == 3);
    REQUIRE(total == 60);
}

TEST_CASE("JobObjHash reserve supports expected object count", "[core][container][objhash]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.reserve(128);

    for (int i = 0; i < 128; ++i) {
        hash.insert(
            std::make_unique<TestObject>("object-" + std::to_string(i), i)
            );
    }

    REQUIRE(hash.size() == 128);

    for (int i = 0; i < 128; ++i) {
        REQUIRE(
            hash.at("object-" + std::to_string(i))->value() == i
            );
    }
}
TEST_CASE("Raw JobObjHash duplicate rejection destroys rejected object", "[core][container][objhash][edge][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{};

    hash.insert(new TestObject{"duplicate", 10});

    REQUIRE(TestObject::alive() == 1);
    REQUIRE(TestObject::destroyed() == 0);

    REQUIRE_THROWS_AS(hash.insert(new TestObject{"duplicate", 20}), std::invalid_argument);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.at("duplicate")->value() == 10);

    REQUIRE(TestObject::alive() == 1);
    REQUIRE(TestObject::destroyed() == 1);
}

TEST_CASE("Raw JobObjHash empty uid rejection destroys object",
          "[core][container][objhash][edge][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{};

    REQUIRE_THROWS_AS(hash.insert(new TestObject{""}), std::invalid_argument);

    REQUIRE(hash.isEmpty());
    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 1);
}
TEST_CASE("JobSipHash AVX matches scalar 128-bit hashes", "[core][hash][siphash][avx]")
{
    const JobSipHash scalar = makeHash(false);
    const JobSipHash avx = makeHash(true);

    const std::uint64_t uids[]{
        0x0001020304050607ULL, 0x08090a0b0c0d0e0fULL,
        0x1011121314151617ULL, 0x18191a1b1c1d1e1fULL,
        0x2021222324252627ULL, 0x28292a2b2c2d2e2fULL,
        0x3031323334353637ULL, 0x38393a3b3c3d3e3fULL
    };

    const std::uint64_t expected[]{
        scalar.hash128(&uids[0]),
        scalar.hash128(&uids[2]),
        scalar.hash128(&uids[4]),
        scalar.hash128(&uids[6])
    };

    const auto value = avx.hashAvx4(uids);
    alignas(32) std::uint64_t actual[4];
    job::simd::SIMD::mov_i64(reinterpret_cast<std::int64_t *>(actual), value);

    REQUIRE(actual[0] == expected[0]);
    REQUIRE(actual[1] == expected[1]);
    REQUIRE(actual[2] == expected[2]);
    REQUIRE(actual[3] == expected[3]);
}

TEST_CASE("JobSipHash 16-byte automatic path matches forced scalar", "[core][hash][siphash][avx]")
{
    const JobSipHash scalar = makeHash(false);
    const JobSipHash automatic = makeHash(true);
    const std::string uid = makeUid16(42);

    REQUIRE(uid.size() == 16);
    REQUIRE(automatic(uid) == scalar(uid));
}

TEST_CASE("JobSipHash non-16-byte input falls back to scalar", "[core][hash][siphash][avx]")
{
    const JobSipHash scalar = makeHash(false);
    const JobSipHash automatic = makeHash(true);
    const std::string uid = makeUidFallback(42);

    REQUIRE(uid.size() > 16);
    REQUIRE(automatic(uid) == scalar(uid));
}

TEST_CASE("JobObjHash accepts an explicit SipHash", "[core][container][objhash]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{makeHash()};
    hash.insert(std::make_unique<TestObject>("explicit", 42));
    REQUIRE(hash.at("explicit")->value() == 42);
}

TEST_CASE("JobObjHashFast uses the same object API", "[core][container][objhash]")
{
    JobObjHashFast<std::unique_ptr<TestObject>> hash;
    hash.insert(std::make_unique<TestObject>("fast", 42));
    REQUIRE(hash.size() == 1);
    REQUIRE(hash.contains("fast"));
    REQUIRE(hash.at("fast")->value() == 42);
}

// 2
// Ownership and lifetime and edge cases
TEST_CASE("Raw JobObjHash owns inserted objects", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    {
        JobObjHash<TestObject *> hash{};

        hash.insert(new TestObject{"one"});
        hash.insert(new TestObject{"two"});
        hash.insert(new TestObject{"three"});

        REQUIRE(TestObject::alive() == 3);
        REQUIRE(TestObject::destroyed() == 0);
    }

    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 3);
}

TEST_CASE("Raw JobObjHash remove destroys the object", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{};

    hash.insert(new TestObject{"one"});
    hash.insert(new TestObject{"two"});

    REQUIRE(TestObject::alive() == 2);

    REQUIRE(hash.remove("one"));

    REQUIRE(TestObject::alive() == 1);
    REQUIRE(TestObject::destroyed() == 1);

    REQUIRE_FALSE(hash.contains("one"));
    REQUIRE(hash.contains("two"));
}

TEST_CASE("Raw JobObjHash take transfers ownership to caller", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{};

    hash.insert(new TestObject{"object", 42});

    REQUIRE(TestObject::alive() == 1);

    TestObject *object = hash.take("object");

    REQUIRE(object);
    REQUIRE(object->uid() == "object");
    REQUIRE(object->value() == 42);

    REQUIRE(hash.isEmpty());

    REQUIRE(TestObject::alive() == 1);
    REQUIRE(TestObject::destroyed() == 0);

    delete object;

    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 1);
}

TEST_CASE("Unique JobObjHash take transfers unique ownership", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("object", 42));

    REQUIRE(TestObject::alive() == 1);

    auto object = hash.take("object");

    REQUIRE(object);
    REQUIRE(object->uid() == "object");
    REQUIRE(object->value() == 42);

    REQUIRE(hash.isEmpty());
    REQUIRE(TestObject::alive() == 1);
    REQUIRE(TestObject::destroyed() == 0);

    object.reset();

    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 1);
}

TEST_CASE("Shared JobObjHash take transfers its shared reference", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<std::shared_ptr<TestObject>> hash{};

    auto external = std::make_shared<TestObject>("object", 42);

    REQUIRE(external.use_count() == 1);

    hash.insert(external);

    REQUIRE(external.use_count() == 2);

    auto object = hash.take("object");

    REQUIRE(hash.isEmpty());

    REQUIRE(object == external);
    REQUIRE(external.use_count() == 2);

    object.reset();

    REQUIRE(external.use_count() == 1);
    REQUIRE(TestObject::alive() == 1);
}

TEST_CASE("Raw JobObjHash clear destroys all objects", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{};

    hash.insert(new TestObject{"one"});
    hash.insert(new TestObject{"two"});
    hash.insert(new TestObject{"three"});

    REQUIRE(TestObject::alive() == 3);

    hash.clear();

    REQUIRE(hash.isEmpty());
    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 3);
}

TEST_CASE("Unique JobObjHash clear destroys all objects", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();
    JobObjHash<std::unique_ptr<TestObject>> hash{};
    hash.insert(std::make_unique<TestObject>("one"));
    hash.insert(std::make_unique<TestObject>("two"));
    hash.insert(std::make_unique<TestObject>("three"));

    REQUIRE(TestObject::alive() == 3);
    hash.clear();

    REQUIRE(hash.isEmpty());
    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 3);
}

TEST_CASE("Shared JobObjHash clear releases its references", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();
    auto object = std::make_shared<TestObject>("object");
    REQUIRE(object.use_count() == 1);

    {
        JobObjHash<std::shared_ptr<TestObject>> hash{};
        hash.insert(object);
        REQUIRE(object.use_count() == 2);

        hash.clear();

        REQUIRE(hash.isEmpty());
        REQUIRE(object.use_count() == 1);
        REQUIRE(TestObject::alive() == 1);
    }

    REQUIRE(TestObject::alive() == 1);

    object.reset();

    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 1);
}

// 3
// Edge cases and invariants

TEST_CASE("A default logical JobObjHash state is empty", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    REQUIRE(hash.isEmpty());
    REQUIRE(hash.size() == 0);
    REQUIRE(hash.count() == 0);

    REQUIRE(hash.begin() == hash.end());
    REQUIRE(hash.constBegin() == hash.constEnd());
}

TEST_CASE("JobObjHash rejects a null raw pointer", "[core][container][objhash][edge]")
{
    JobObjHash<TestObject *> hash{};
    REQUIRE_THROWS_AS(hash.insert(nullptr), std::invalid_argument);
    REQUIRE(hash.isEmpty());
}

TEST_CASE("JobObjHash rejects a null unique pointer", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};
    std::unique_ptr<TestObject> object;
    REQUIRE_THROWS_AS(hash.insert(std::move(object)), std::invalid_argument);
    REQUIRE(hash.isEmpty());
}

TEST_CASE("JobObjHash rejects a null shared pointer", "[core][container][objhash][edge]")
{
    JobObjHash<std::shared_ptr<TestObject>> hash{};
    std::shared_ptr<TestObject> object;
    REQUIRE_THROWS_AS(hash.insert(object), std::invalid_argument);
    REQUIRE(hash.isEmpty());
}

TEST_CASE("JobObjHash rejects an empty uid", "[core][container][objhash][edge]")
{
    TestObject::resetCounters();

    JobObjHash<std::unique_ptr<TestObject>> hash{};
    REQUIRE_THROWS_AS(hash.insert(std::make_unique<TestObject>("")), std::invalid_argument);

    REQUIRE(hash.isEmpty());

    // The temporary unique_ptr still owns the rejected object and destroys it.
    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 1);
}

TEST_CASE("JobObjHash rejects duplicate uid", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};
    hash.insert(std::make_unique<TestObject>("duplicate", 10));
    REQUIRE_THROWS_AS(hash.insert(std::make_unique<TestObject>("duplicate", 20)), std::invalid_argument);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.at("duplicate")->value() == 10);
}

TEST_CASE("JobObjHash duplicate rejection preserves existing object", "[core][container][objhash][edge]")
{
    JobObjHash<std::shared_ptr<TestObject>> hash{};

    auto original = std::make_shared<TestObject>("same", 10);
    auto duplicate = std::make_shared<TestObject>("same", 20);
    hash.insert(original);

    REQUIRE_THROWS_AS(hash.insert(duplicate), std::invalid_argument);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.at("same") == original.get());
    REQUIRE(hash.at("same") != duplicate.get());
    REQUIRE(hash.at("same")->value() == 10);
}

TEST_CASE("JobObjHash at throws when uid does not exist", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one"));

    REQUIRE_THROWS_AS(hash.at("euler_thinks_its_right_rk4_knows"), std::out_of_range);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.contains("one"));
}

TEST_CASE("Const JobObjHash at throws when uid does not exist", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> mutableHash{};
    mutableHash.insert(std::make_unique<TestObject>("one"));
    const JobObjHash<std::unique_ptr<TestObject>> &hash = mutableHash;
    REQUIRE_THROWS_AS(hash.at("euler_euler_right_way_who_knows"), std::out_of_range);
}

TEST_CASE("JobObjHash take throws when uid does not exist", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one", 10));
    hash.insert(std::make_unique<TestObject>("two", 20));

    REQUIRE_THROWS_AS(hash.take("euler_brother_is_rk4"), std::out_of_range);

    REQUIRE(hash.size() == 2);
    REQUIRE(hash.at("one")->value() == 10);
    REQUIRE(hash.at("two")->value() == 20);
}

TEST_CASE("JobObjHash remove reports a missing uid", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one"));

    REQUIRE_FALSE(hash.remove("rk4_says_euler_is_my_drunk_brother"));

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.contains("one"));
}

TEST_CASE("JobObjHash remains unchanged after failed take", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};

    hash.insert(std::make_unique<TestObject>("one", 10));
    hash.insert(std::make_unique<TestObject>("two", 20));
    hash.insert(std::make_unique<TestObject>("three", 30));

    REQUIRE_THROWS_AS(hash.take("missing"), std::out_of_range);

    REQUIRE(hash.size() == 3);
    REQUIRE(hash.at("one")->value() == 10);
    REQUIRE(hash.at("two")->value() == 20);
    REQUIRE(hash.at("three")->value() == 30);
}

TEST_CASE("JobObjHash key always comes from object uid", "[core][container][objhash][edge]")
{
    JobObjHash<std::unique_ptr<TestObject>> hash{};
    hash.insert(std::make_unique<TestObject>("the-object-decides-the-key", 42));
    REQUIRE(hash.contains("the-object-decides-the-key"));
    REQUIRE(hash.at("the-object-decides-the-key")->uid() == "the-object-decides-the-key");
}

TEST_CASE("Raw JobObjHash move transfers object ownership", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();
    {
        JobObjHash<TestObject *> source{};

        source.insert(new TestObject{"one"});
        source.insert(new TestObject{"two"});

        REQUIRE(TestObject::alive() == 2);

        JobObjHash<TestObject *> destination{std::move(source)};

        REQUIRE(destination.size() == 2);
        REQUIRE(destination.contains("one"));
        REQUIRE(destination.contains("two"));

        REQUIRE(source.isEmpty());

        REQUIRE(TestObject::alive() == 2);
        REQUIRE(TestObject::destroyed() == 0);
    }

    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 2);
}

TEST_CASE("Raw JobObjHash move assignment releases old objects and transfers ownership", "[core][container][objhash][lifetime]")
{
    TestObject::resetCounters();

    {
        JobObjHash<TestObject *> source{};
        JobObjHash<TestObject *> destination{};

        source.insert(new TestObject{"one"});
        source.insert(new TestObject{"two"});
        destination.insert(new TestObject{"old"});

        REQUIRE(TestObject::alive() == 3);

        destination = std::move(source);

        REQUIRE(source.isEmpty());
        REQUIRE(destination.size() == 2);
        REQUIRE(destination.contains("one"));
        REQUIRE(destination.contains("two"));
        REQUIRE_FALSE(destination.contains("old"));

        REQUIRE(TestObject::alive() == 2);
        REQUIRE(TestObject::destroyed() == 1);
    }

    REQUIRE(TestObject::alive() == 0);
    REQUIRE(TestObject::destroyed() == 3);
}

TEST_CASE("JobObjHash stores raw pointer objects by 16-byte uid", "[core][container][objhash]")
{
    TestObject::resetCounters();

    JobObjHash<TestObject *> hash{}; // defaults to sipHash + AVX

    hash.insert(new TestObject{makeUid16(1), 10});
    hash.insert(new TestObject{makeUid16(2), 20});
    hash.insert(new TestObject{makeUid16(3), 30});

    REQUIRE(hash.size() == 3);
    REQUIRE(hash.contains(makeUid16(1)));
    REQUIRE(hash.contains(makeUid16(2)));
    REQUIRE(hash.contains(makeUid16(3)));

    REQUIRE(hash.at(makeUid16(1))->value() == 10);
    REQUIRE(hash.at(makeUid16(2))->value() == 20);
    REQUIRE(hash.at(makeUid16(3))->value() == 30);

    REQUIRE(TestObject::alive() == 3);
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Job hash bucket distribution comparison", "[core][container][objhash][diagnostic]")
{
    constexpr std::size_t count = 1000000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    std::unordered_map<std::string, int, JobSipHash> sipMap(0, makeHash());
    std::unordered_map<std::string, int, FastHash> fastMap;
    sipMap.reserve(count);
    fastMap.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        sipMap.emplace(keys[i], static_cast<int>(i));
        fastMap.emplace(keys[i], static_cast<int>(i));
    }

    std::size_t sipEmpty = 0, sipMax = 0, sipColliding = 0;
    for (std::size_t i = 0; i < sipMap.bucket_count(); ++i) {
        const std::size_t size = sipMap.bucket_size(i);
        if (size == 0) ++sipEmpty;
        if (size > 1) ++sipColliding;
        sipMax = std::max(sipMax, size);
    }

    std::size_t fastEmpty = 0, fastMax = 0, fastColliding = 0;
    for (std::size_t i = 0; i < fastMap.bucket_count(); ++i) {
        const std::size_t size = fastMap.bucket_size(i);
        if (size == 0) ++fastEmpty;
        if (size > 1) ++fastColliding;
        fastMax = std::max(fastMax, size);
    }

    INFO("SipHash bucket count: " << sipMap.bucket_count());
    INFO("SipHash load factor: " << sipMap.load_factor());
    INFO("SipHash empty buckets: " << sipEmpty);
    INFO("SipHash colliding buckets: " << sipColliding);
    INFO("SipHash max bucket size: " << sipMax);
    INFO("Fast bucket count: " << fastMap.bucket_count());
    INFO("Fast load factor: " << fastMap.load_factor());
    INFO("Fast empty buckets: " << fastEmpty);
    INFO("Fast colliding buckets: " << fastColliding);
    INFO("Fast max bucket size: " << fastMax);

    REQUIRE(sipMap.size() == count);
    REQUIRE(fastMap.size() == count);
}

TEST_CASE("unordered_map 16-byte UID hasher lookup comparison 1M", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 1000000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    std::unordered_map<std::string, TestObject *, JobSipHash> sipMap(0, makeHash());
    std::unordered_map<std::string, TestObject *, FastHash> fastMap;
    sipMap.reserve(count);
    fastMap.reserve(count);

    std::vector<std::unique_ptr<TestObject>> objects;
    objects.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        objects.emplace_back(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        TestObject *ptr = objects.back().get();
        sipMap.emplace(keys[i], ptr);
        fastMap.emplace(keys[i], ptr);
    }

    BENCHMARK("unordered_map JobSipHash 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (sipMap.find(key) != sipMap.end()) ++found;
        return found;
    };

    BENCHMARK("unordered_map FastHash 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (fastMap.find(key) != fastMap.end()) ++found;
        return found;
    };
}

TEST_CASE("String hash benchmark prepared 16-byte UID comparison 1M", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 1000000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    const JobSipHash sipHash = makeHash();
    const FastHash fastHash;

    BENCHMARK("JobSipHash automatic 16-byte hash 1M")
    {
        std::size_t result = 0;
        for (const std::string &key : keys) result ^= sipHash(key);
        return result;
    };

    BENCHMARK("FastHash 16-byte hash 1M")
    {
        std::size_t result = 0;
        for (const std::string &key : keys) result ^= fastHash(key);
        return result;
    };
}

TEST_CASE("String hash benchmark scalar fallback comparison 1M", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 1000000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUidFallback(i));

    const JobSipHash automatic = makeHash(true);
    const JobSipHash scalar = makeHash(false);
    const FastHash fastHash;

    BENCHMARK("JobSipHash automatic fallback hash 1M")
    {
        std::size_t result = 0;
        for (const std::string &key : keys) result ^= automatic(key);
        return result;
    };

    BENCHMARK("JobSipHash forced scalar fallback hash 1M")
    {
        std::size_t result = 0;
        for (const std::string &key : keys) result ^= scalar(key);
        return result;
    };

    BENCHMARK("FastHash fallback UID hash 1M")
    {
        std::size_t result = 0;
        for (const std::string &key : keys) result ^= fastHash(key);
        return result;
    };
}

TEST_CASE("unordered_map 16-byte UID hasher caching comparison 1M", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 1000000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    const JobSipHash sipHash = makeHash();
    std::unordered_map<std::string, TestObject *, JobSipHash> sipMap(0, sipHash);
    std::unordered_map<std::string, TestObject *, JobSipHashCachedTest> cachedMap(0, JobSipHashCachedTest{sipHash});
    std::unordered_map<std::string, TestObject *, FastHash> fastMap;

    sipMap.reserve(count);
    cachedMap.reserve(count);
    fastMap.reserve(count);

    std::vector<std::unique_ptr<TestObject>> objects;
    objects.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        objects.emplace_back(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        TestObject *ptr = objects.back().get();
        sipMap.emplace(keys[i], ptr);
        cachedMap.emplace(keys[i], ptr);
        fastMap.emplace(keys[i], ptr);
    }

    REQUIRE(sipMap.size() == count);
    REQUIRE(cachedMap.size() == count);
    REQUIRE(fastMap.size() == count);

    BENCHMARK("unordered_map JobSipHash cached-node 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (sipMap.find(key) != sipMap.end()) ++found;
        return found;
    };

    BENCHMARK("unordered_map JobSipHash wrapper 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (cachedMap.find(key) != cachedMap.end()) ++found;
        return found;
    };

    BENCHMARK("unordered_map FastHash 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (fastMap.find(key) != fastMap.end()) ++found;
        return found;
    };
}

TEST_CASE("JobObjHash benchmark insertion", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    BENCHMARK("JobObjHash unique_ptr 16-byte insert 100k")
    {
        JobObjHash<std::unique_ptr<TestObject>> hash;
        hash.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
            hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        return hash.size();
    };

    BENCHMARK("JobObjHashFast unique_ptr 16-byte insert 100k")
    {
        JobObjHashFast<std::unique_ptr<TestObject>> hash;
        hash.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
            hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        return hash.size();
    };
}

TEST_CASE("JobObjHash benchmark hot lookup", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    JobObjHash<std::unique_ptr<TestObject>> hash;
    JobObjHashFast<std::unique_ptr<TestObject>> fastHash;
    hash.reserve(count);
    fastHash.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        const std::string uid = makeUid16(i);
        hash.insert(std::make_unique<TestObject>(uid, static_cast<int>(i)));
        fastHash.insert(std::make_unique<TestObject>(uid, static_cast<int>(i)));
    }

    const std::string existingKey = makeUid16(50000);
    const std::string missingKey = makeUid16(count + 1);

    BENCHMARK("JobObjHash at existing UID") { return hash.at(existingKey); };
    BENCHMARK("JobObjHashFast at existing UID") { return fastHash.at(existingKey); };
    BENCHMARK("JobObjHash contains existing UID") { return hash.contains(existingKey); };
    BENCHMARK("JobObjHashFast contains existing UID") { return fastHash.contains(existingKey); };
    BENCHMARK("JobObjHash find existing UID") { return hash.find(existingKey); };
    BENCHMARK("JobObjHashFast find existing UID") { return fastHash.find(existingKey); };
    BENCHMARK("JobObjHash contains missing UID") { return hash.contains(missingKey); };
    BENCHMARK("JobObjHashFast contains missing UID") { return fastHash.contains(missingKey); };
    BENCHMARK("JobObjHash find missing UID") { return hash.find(missingKey); };
    BENCHMARK("JobObjHashFast find missing UID") { return fastHash.find(missingKey); };
}

TEST_CASE("JobObjHash benchmark prepared UID lookup 100k", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    JobObjHash<std::unique_ptr<TestObject>> hash;
    hash.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));

    BENCHMARK("JobObjHash sequential 16-byte lookup 100k")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (hash.contains(key)) ++found;
        return found;
    };
}

TEST_CASE("JobObjHash benchmark prepared UID lookup comparison 100k", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    JobObjHash<std::unique_ptr<TestObject>> hash;
    JobObjHashFast<std::unique_ptr<TestObject>> fastHash;
    hash.reserve(count);
    fastHash.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        fastHash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
    }

    BENCHMARK("JobObjHash SipHash 16-byte lookup 100k")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (hash.contains(key)) ++found;
        return found;
    };

    BENCHMARK("JobObjHashFast 16-byte lookup 100k")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (fastHash.contains(key)) ++found;
        return found;
    };
}

TEST_CASE("JobObjHash benchmark scalar fallback lookup comparison 100k", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUidFallback(i));

    JobObjHash<std::unique_ptr<TestObject>> hash;
    JobObjHashFast<std::unique_ptr<TestObject>> fastHash;
    hash.reserve(count);
    fastHash.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        fastHash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
    }

    BENCHMARK("JobObjHash SipHash fallback lookup 100k")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (hash.contains(key)) ++found;
        return found;
    };

    BENCHMARK("JobObjHashFast fallback lookup 100k")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (fastHash.contains(key)) ++found;
        return found;
    };
}

TEST_CASE("JobObjHash benchmark prepared UID lookup comparison 1M", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 1000000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    JobObjHash<std::unique_ptr<TestObject>> hash;
    JobObjHashFast<std::unique_ptr<TestObject>> fastHash;
    hash.reserve(count);
    fastHash.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
        fastHash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));
    }

    BENCHMARK("JobObjHash SipHash 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (hash.contains(key)) ++found;
        return found;
    };

    BENCHMARK("JobObjHashFast 16-byte lookup 1M")
    {
        std::size_t found = 0;
        for (const std::string &key : keys)
            if (fastHash.contains(key)) ++found;
        return found;
    };
}

TEST_CASE("JobObjHash benchmark unique pointer remove batch", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    BENCHMARK("JobObjHash unique_ptr 16-byte insert/remove 100k")
    {
        JobObjHash<std::unique_ptr<TestObject>> hash;
        hash.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
            hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));

        for (const std::string &key : keys)
            (void)hash.remove(key);

        return hash.size();
    };

    BENCHMARK("JobObjHashFast unique_ptr 16-byte insert/remove 100k")
    {
        JobObjHashFast<std::unique_ptr<TestObject>> hash;
        hash.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
            hash.insert(std::make_unique<TestObject>(keys[i], static_cast<int>(i)));

        for (const std::string &key : keys)
            (void)hash.remove(key);

        return hash.size();
    };
}

TEST_CASE("JobObjHash benchmark raw pointer remove batch", "[core][container][objhash][benchmark]")
{
    constexpr std::size_t count = 100000;
    std::vector<std::string> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
        keys.emplace_back(makeUid16(i));

    BENCHMARK("JobObjHash raw pointer 16-byte insert/remove 100k")
    {
        JobObjHash<TestObject *> hash;
        hash.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
            hash.insert(new TestObject(keys[i], static_cast<int>(i)));

        for (const std::string &key : keys)
            (void)hash.remove(key);

        return hash.size();
    };

    BENCHMARK("JobObjHashFast raw pointer 16-byte insert/remove 100k")
    {
        JobObjHashFast<TestObject *> hash;
        hash.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
            hash.insert(new TestObject(keys[i], static_cast<int>(i)));

        for (const std::string &key : keys)
            (void)hash.remove(key);

        return hash.size();
    };
}

TEST_CASE("JobSipHash scalar and AVX benchmark 1M 128-bit UIDs", "[core][hash][siphash][avx][benchmark]")
{
    constexpr std::size_t count = 1000000;
    static_assert(count % 4 == 0);

    std::vector<std::uint64_t> uids(count * 2);

    for (std::size_t i = 0; i < count; ++i) {
        uids[(i * 2) + 0] = UINT64_C(0x1234567800000000) + static_cast<std::uint64_t>(i);
        uids[(i * 2) + 1] = UINT64_C(0xfedcba9800000000) ^ static_cast<std::uint64_t>(i);
    }

    const JobSipHash scalar = makeHash(false);
    const JobSipHash automatic = makeHash(true);

    BENCHMARK("JobSipHash forced scalar 1M 128-bit UIDs")
    {
        std::uint64_t result = 0;
        for (std::size_t i = 0; i < count; ++i)
            result ^= scalar.hash128(&uids[i * 2]);
        return result;
    };

    BENCHMARK("JobSipHash automatic single-UID AVX 1M 128-bit UIDs")
    {
        std::uint64_t result = 0;
        for (std::size_t i = 0; i < count; ++i)
            result ^= automatic.hash128(&uids[i * 2]);
        return result;
    };

    BENCHMARK("JobSipHash AVX x4 1M 128-bit UIDs")
    {
        std::uint64_t result = 0;
        alignas(32) std::uint64_t values[4];

        for (std::size_t i = 0; i < count; i += 4) {
            const auto value = automatic.hashAvx4(&uids[i * 2]);
            job::simd::SIMD::mov_i64(reinterpret_cast<std::int64_t *>(values), value);
            result ^= values[0] ^ values[1] ^ values[2] ^ values[3];
        }

        return result;
    };
}

#endif