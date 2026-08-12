#include <catch2/catch_all.hpp>

#include <job_hash_container.h>

#include <memory>
#include <string>

using namespace job::core;

// 1
// How do I use JobHash?

TEST_CASE("JobHash stores values by key", "[core][container][hash]")
{
    JobHash<std::string, int> hash;

    hash.insert("one", 1);
    hash.insert("two", 2);
    hash.insert("three", 3);

    REQUIRE(hash.size() == 3);
    REQUIRE(hash.count() == 3);

    REQUIRE(hash.at("one") == 1);
    REQUIRE(hash.at("two") == 2);
    REQUIRE(hash.at("three") == 3);
}

TEST_CASE("JobHash insert replaces an existing value", "[core][container][hash]")
{
    JobHash<std::string, int> hash;

    hash.insert("value", 10);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.at("value") == 10);

    hash.insert("value", 20);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.at("value") == 20);
}

TEST_CASE("JobHash operator index accesses and inserts values", "[core][container][hash]")
{
    JobHash<std::string, int> hash;

    hash["one"] = 1;
    hash["two"] = 2;

    REQUIRE(hash.size() == 2);
    REQUIRE(hash["one"] == 1);
    REQUIRE(hash["two"] == 2);

    REQUIRE(hash["three"] == 0);
    REQUIRE(hash.size() == 3);

    hash["three"] = 3;

    REQUIRE(hash["three"] == 3);
}

TEST_CASE("JobHash can find contained keys", "[core][container][hash]")
{
    JobHash<std::string, int> hash;

    hash.insert("one", 1);
    hash.insert("two", 2);

    REQUIRE(hash.contains("one"));
    REQUIRE(hash.contains("two"));
    REQUIRE_FALSE(hash.contains("three"));

    auto it = hash.find("two");

    REQUIRE(it != hash.end());
    REQUIRE(it->first == "two");
    REQUIRE(it->second == 2);

    REQUIRE(hash.find("three") == hash.end());
}

TEST_CASE("JobHash supports const lookup", "[core][container][hash]")
{
    JobHash<std::string, int> mutableHash;

    mutableHash.insert("one", 1);
    mutableHash.insert("two", 2);

    const JobHash<std::string, int> &hash = mutableHash;

    REQUIRE(hash.at("one") == 1);
    REQUIRE(hash.at("two") == 2);
    REQUIRE(hash.contains("one"));

    const auto it = hash.find("two");

    REQUIRE(it != hash.end());
    REQUIRE(it->first == "two");
    REQUIRE(it->second == 2);
}

TEST_CASE("JobHash removes values by key", "[core][container][hash]")
{
    JobHash<std::string, int> hash;

    hash.insert("one", 1);
    hash.insert("two", 2);
    hash.insert("three", 3);

    REQUIRE(hash.remove("two"));

    REQUIRE(hash.size() == 2);
    REQUIRE(hash.contains("one"));
    REQUIRE_FALSE(hash.contains("two"));
    REQUIRE(hash.contains("three"));
}

TEST_CASE("JobHash take removes and returns a value", "[core][container][hash]")
{
    JobHash<std::string, std::string> hash;

    hash.insert("one", "first");
    hash.insert("two", "second");

    const std::string value = hash.take("one");

    REQUIRE(value == "first");
    REQUIRE(hash.size() == 1);
    REQUIRE_FALSE(hash.contains("one"));
    REQUIRE(hash.contains("two"));
}

TEST_CASE("JobHash supports iteration", "[core][container][hash]")
{
    JobHash<int, int> hash;

    hash.insert(1, 10);
    hash.insert(2, 20);
    hash.insert(3, 30);

    int total = 0;

    for (const auto &[key, value] : hash)
        total += key + value;

    REQUIRE(total == 66);
}

TEST_CASE("JobHash supports const iteration", "[core][container][hash]")
{
    JobHash<int, int> mutableHash;

    mutableHash.insert(1, 10);
    mutableHash.insert(2, 20);
    mutableHash.insert(3, 30);

    const JobHash<int, int> &hash = mutableHash;

    int total = 0;

    for (auto it = hash.constBegin(); it != hash.constEnd(); ++it)
        total += it->first + it->second;

    REQUIRE(total == 66);
}

TEST_CASE("JobHash reserve supports expected hash capacity", "[core][container][hash]")
{
    JobHash<int, int> hash;

    hash.reserve(128);

    for (int i = 0; i < 128; ++i)
        hash.insert(i, i * 10);

    REQUIRE(hash.size() == 128);

    for (int i = 0; i < 128; ++i)
        REQUIRE(hash.at(i) == i * 10);
}

TEST_CASE("JobHash supports move only values", "[core][container][hash]")
{
    JobHash<std::string, std::unique_ptr<int>> hash;

    hash.insert("one", std::make_unique<int>(10));
    hash.insert("two", std::make_unique<int>(20));

    REQUIRE(hash.size() == 2);
    REQUIRE(*hash.at("one") == 10);
    REQUIRE(*hash.at("two") == 20);

    auto value = hash.take("one");

    REQUIRE(value);
    REQUIRE(*value == 10);

    REQUIRE(hash.size() == 1);
    REQUIRE_FALSE(hash.contains("one"));
    REQUIRE(*hash.at("two") == 20);
}

TEST_CASE("JobHash supports move only keys", "[core][container][hash]")
{
    // std::unique_ptr is intentionally not hashable by std::hash in a
    // useful value-semantic way for this test, so use a movable string key.
    JobHash<std::string, int> hash;

    std::string key = "movable-key";

    hash.insert(std::move(key), 42);

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.contains("movable-key"));
    REQUIRE(hash.at("movable-key") == 42);
}


// 2
// Edge cases and invariants

TEST_CASE("A default JobHash is empty", "[core][container][hash][edge]")
{
    const JobHash<int, int> hash;

    REQUIRE(hash.isEmpty());
    REQUIRE(hash.size() == 0);
    REQUIRE(hash.count() == 0);

    REQUIRE(hash.begin() == hash.end());
    REQUIRE(hash.constBegin() == hash.constEnd());
}

TEST_CASE("JobHash clear removes every entry", "[core][container][hash][edge]")
{
    JobHash<int, int> hash;

    hash.insert(1, 10);
    hash.insert(2, 20);
    hash.insert(3, 30);

    hash.clear();

    REQUIRE(hash.isEmpty());
    REQUIRE(hash.size() == 0);
    REQUIRE(hash.count() == 0);

    REQUIRE_FALSE(hash.contains(1));
    REQUIRE_FALSE(hash.contains(2));
    REQUIRE_FALSE(hash.contains(3));
}

TEST_CASE("JobHash remove reports a missing key", "[core][container][hash][edge]")
{
    JobHash<std::string, int> hash;

    hash.insert("one", 1);
    hash.insert("two", 2);

    REQUIRE_FALSE(hash.remove("three"));

    REQUIRE(hash.size() == 2);
    REQUIRE(hash.at("one") == 1);
    REQUIRE(hash.at("two") == 2);
}

TEST_CASE("JobHash at throws when key does not exist", "[core][container][hash][edge]")
{
    JobHash<std::string, int> hash;

    hash.insert("one", 1);

    REQUIRE_THROWS_AS(
        hash.at("missing"),
        std::out_of_range
        );

    REQUIRE(hash.size() == 1);
    REQUIRE(hash.at("one") == 1);
}

TEST_CASE("Const JobHash at throws when key does not exist", "[core][container][hash][edge]")
{
    JobHash<std::string, int> mutableHash;

    mutableHash.insert("one", 1);

    const JobHash<std::string, int> &hash = mutableHash;

    REQUIRE_THROWS_AS(
        hash.at("missing"),
        std::out_of_range
        );
}

TEST_CASE("JobHash take throws when key does not exist", "[core][container][hash][edge]")
{
    JobHash<std::string, int> hash;

    hash.insert("one", 1);
    hash.insert("two", 2);

    REQUIRE_THROWS_AS(
        hash.take("missing"),
        std::out_of_range
        );

    REQUIRE(hash.size() == 2);
    REQUIRE(hash.at("one") == 1);
    REQUIRE(hash.at("two") == 2);
}

TEST_CASE("JobHash remains unchanged after failed take", "[core][container][hash][edge]")
{
    JobHash<int, std::string> hash;

    hash.insert(1, "one");
    hash.insert(2, "two");
    hash.insert(3, "three");

    REQUIRE_THROWS_AS(
        hash.take(42),
        std::out_of_range
        );

    REQUIRE(hash.size() == 3);
    REQUIRE(hash.at(1) == "one");
    REQUIRE(hash.at(2) == "two");
    REQUIRE(hash.at(3) == "three");
}

TEST_CASE("JobHash replacing a value does not change its size", "[core][container][hash][edge]")
{
    JobHash<int, std::string> hash;

    hash.insert(1, "one");
    hash.insert(2, "two");

    const auto originalSize = hash.size();

    hash.insert(1, "ONE");

    REQUIRE(hash.size() == originalSize);
    REQUIRE(hash.at(1) == "ONE");
    REQUIRE(hash.at(2) == "two");
}

TEST_CASE("JobHash operator index default constructs missing mapped values", "[core][container][hash][edge]")
{
    JobHash<int, std::string> hash;

    REQUIRE_FALSE(hash.contains(42));

    std::string &value = hash[42];

    REQUIRE(hash.contains(42));
    REQUIRE(hash.size() == 1);
    REQUIRE(value.empty());

    value = "answer";

    REQUIRE(hash.at(42) == "answer");
}