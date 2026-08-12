#include <catch2/catch_all.hpp>

#include <job_map.h>

#include <functional>
#include <memory>
#include <string>

using namespace job::core;

// 1 How do I use JobMap?
TEST_CASE("JobMap stores values by key", "[core][container][map]")
{
    JobMap<std::string, int> map;

    map.insert("one", 1);
    map.insert("two", 2);
    map.insert("three", 3);

    REQUIRE(map.size() == 3);
    REQUIRE(map.count() == 3);

    REQUIRE(map.at("one") == 1);
    REQUIRE(map.at("two") == 2);
    REQUIRE(map.at("three") == 3);
}

TEST_CASE("JobMap insert replaces an existing value", "[core][container][map]")
{
    JobMap<std::string, int> map;

    map.insert("value", 10);

    REQUIRE(map.size() == 1);
    REQUIRE(map.at("value") == 10);

    map.insert("value", 20);

    REQUIRE(map.size() == 1);
    REQUIRE(map.at("value") == 20);
}

TEST_CASE("JobMap operator index accesses and inserts values", "[core][container][map]")
{
    JobMap<std::string, int> map;

    map["one"] = 1;
    map["two"] = 2;

    REQUIRE(map.size() == 2);
    REQUIRE(map["one"] == 1);
    REQUIRE(map["two"] == 2);

    REQUIRE(map["three"] == 0);
    REQUIRE(map.size() == 3);

    map["three"] = 3;

    REQUIRE(map["three"] == 3);
}

TEST_CASE("JobMap can find contained keys", "[core][container][map]")
{
    JobMap<std::string, int> map;

    map.insert("one", 1);
    map.insert("two", 2);

    REQUIRE(map.contains("one"));
    REQUIRE(map.contains("two"));
    REQUIRE_FALSE(map.contains("three"));

    auto it = map.find("two");

    REQUIRE(it != map.end());
    REQUIRE(it->first == "two");
    REQUIRE(it->second == 2);

    REQUIRE(map.find("three") == map.end());
}

TEST_CASE("JobMap supports const lookup", "[core][container][map]")
{
    JobMap<std::string, int> mutableMap;

    mutableMap.insert("one", 1);
    mutableMap.insert("two", 2);

    const JobMap<std::string, int> &map = mutableMap;

    REQUIRE(map.at("one") == 1);
    REQUIRE(map.at("two") == 2);
    REQUIRE(map.contains("one"));

    const auto it = map.find("two");

    REQUIRE(it != map.end());
    REQUIRE(it->first == "two");
    REQUIRE(it->second == 2);
}

TEST_CASE("JobMap removes values by key", "[core][container][map]")
{
    JobMap<std::string, int> map;

    map.insert("one", 1);
    map.insert("two", 2);
    map.insert("three", 3);

    REQUIRE(map.remove("two"));

    REQUIRE(map.size() == 2);
    REQUIRE(map.contains("one"));
    REQUIRE_FALSE(map.contains("two"));
    REQUIRE(map.contains("three"));
}

TEST_CASE("JobMap take removes and returns a value", "[core][container][map]")
{
    JobMap<std::string, std::string> map;

    map.insert("one", "first");
    map.insert("two", "second");

    const std::string value = map.take("one");

    REQUIRE(value == "first");
    REQUIRE(map.size() == 1);
    REQUIRE_FALSE(map.contains("one"));
    REQUIRE(map.contains("two"));
}

TEST_CASE("JobMap iterates in key order", "[core][container][map]")
{
    JobMap<int, std::string> map;

    map.insert(30, "thirty");
    map.insert(10, "ten");
    map.insert(40, "forty");
    map.insert(20, "twenty");

    auto it = map.begin();

    REQUIRE(it->first == 10);
    REQUIRE(it->second == "ten");

    ++it;
    REQUIRE(it->first == 20);
    REQUIRE(it->second == "twenty");

    ++it;
    REQUIRE(it->first == 30);
    REQUIRE(it->second == "thirty");

    ++it;
    REQUIRE(it->first == 40);
    REQUIRE(it->second == "forty");

    ++it;
    REQUIRE(it == map.end());
}

TEST_CASE("Const JobMap iterates in key order", "[core][container][map]")
{
    JobMap<int, std::string> mutableMap;

    mutableMap.insert(30, "thirty");
    mutableMap.insert(10, "ten");
    mutableMap.insert(20, "twenty");

    const JobMap<int, std::string> &map = mutableMap;

    auto it = map.constBegin();

    REQUIRE(it->first == 10);
    ++it;

    REQUIRE(it->first == 20);
    ++it;

    REQUIRE(it->first == 30);
    ++it;

    REQUIRE(it == map.constEnd());
}

TEST_CASE("JobMap supports range based iteration", "[core][container][map]")
{
    JobMap<int, int> map;

    map.insert(3, 30);
    map.insert(1, 10);
    map.insert(2, 20);

    int expectedKey = 1;
    int total = 0;

    for (const auto &[key, value] : map) {
        REQUIRE(key == expectedKey);
        ++expectedKey;

        total += key + value;
    }

    REQUIRE(total == 66);
}

TEST_CASE("JobMap supports move only values", "[core][container][map]")
{
    JobMap<std::string, std::unique_ptr<int>> map;

    map.insert("one", std::make_unique<int>(10));
    map.insert("two", std::make_unique<int>(20));

    REQUIRE(map.size() == 2);
    REQUIRE(*map.at("one") == 10);
    REQUIRE(*map.at("two") == 20);

    auto value = map.take("one");

    REQUIRE(value);
    REQUIRE(*value == 10);

    REQUIRE(map.size() == 1);
    REQUIRE_FALSE(map.contains("one"));
    REQUIRE(*map.at("two") == 20);
}

TEST_CASE("JobMap accepts movable keys", "[core][container][map]")
{
    JobMap<std::string, int> map;

    std::string key = "movable-key";

    map.insert(std::move(key), 42);

    REQUIRE(map.size() == 1);
    REQUIRE(map.contains("movable-key"));
    REQUIRE(map.at("movable-key") == 42);
}

TEST_CASE("JobMap supports a custom comparison policy", "[core][container][map]")
{
    JobMap<int, std::string, std::greater<int>> map{
        std::greater<int>{}
    };

    map.insert(10, "ten");
    map.insert(30, "thirty");
    map.insert(20, "twenty");

    auto it = map.begin();

    REQUIRE(it->first == 30);
    ++it;

    REQUIRE(it->first == 20);
    ++it;

    REQUIRE(it->first == 10);
}


// 2
// Edge cases and invariants

TEST_CASE("A default JobMap is empty", "[core][container][map][edge]")
{
    const JobMap<int, int> map;

    REQUIRE(map.isEmpty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.count() == 0);

    REQUIRE(map.begin() == map.end());
    REQUIRE(map.constBegin() == map.constEnd());
}

TEST_CASE("JobMap clear removes every entry", "[core][container][map][edge]")
{
    JobMap<int, int> map;

    map.insert(1, 10);
    map.insert(2, 20);
    map.insert(3, 30);

    map.clear();

    REQUIRE(map.isEmpty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.count() == 0);

    REQUIRE_FALSE(map.contains(1));
    REQUIRE_FALSE(map.contains(2));
    REQUIRE_FALSE(map.contains(3));
}

TEST_CASE("JobMap remove reports a missing key", "[core][container][map][edge]")
{
    JobMap<std::string, int> map;

    map.insert("one", 1);
    map.insert("two", 2);

    REQUIRE_FALSE(map.remove("three"));

    REQUIRE(map.size() == 2);
    REQUIRE(map.at("one") == 1);
    REQUIRE(map.at("two") == 2);
}

TEST_CASE("JobMap at throws when key does not exist", "[core][container][map][edge]")
{
    JobMap<std::string, int> map;

    map.insert("one", 1);

    REQUIRE_THROWS_AS(
        map.at("missing"),
        std::out_of_range
        );

    REQUIRE(map.size() == 1);
    REQUIRE(map.at("one") == 1);
}

TEST_CASE("Const JobMap at throws when key does not exist", "[core][container][map][edge]")
{
    JobMap<std::string, int> mutableMap;

    mutableMap.insert("one", 1);

    const JobMap<std::string, int> &map = mutableMap;

    REQUIRE_THROWS_AS(
        map.at("missing"),
        std::out_of_range
        );
}

TEST_CASE("JobMap take throws when key does not exist", "[core][container][map][edge]")
{
    JobMap<std::string, int> map;

    map.insert("one", 1);
    map.insert("two", 2);

    REQUIRE_THROWS_AS(
        map.take("missing"),
        std::out_of_range
        );

    REQUIRE(map.size() == 2);
    REQUIRE(map.at("one") == 1);
    REQUIRE(map.at("two") == 2);
}

TEST_CASE("JobMap remains unchanged after failed take", "[core][container][map][edge]")
{
    JobMap<int, std::string> map;

    map.insert(1, "one");
    map.insert(2, "two");
    map.insert(3, "three");

    REQUIRE_THROWS_AS(
        map.take(42),
        std::out_of_range
        );

    REQUIRE(map.size() == 3);
    REQUIRE(map.at(1) == "one");
    REQUIRE(map.at(2) == "two");
    REQUIRE(map.at(3) == "three");
}

TEST_CASE("JobMap replacing a value does not change its size", "[core][container][map][edge]")
{
    JobMap<int, std::string> map;

    map.insert(1, "one");
    map.insert(2, "two");

    const auto originalSize = map.size();

    map.insert(1, "ONE");

    REQUIRE(map.size() == originalSize);
    REQUIRE(map.at(1) == "ONE");
    REQUIRE(map.at(2) == "two");
}

TEST_CASE("JobMap operator index default constructs missing mapped values", "[core][container][map][edge]")
{
    JobMap<int, std::string> map;

    REQUIRE_FALSE(map.contains(42));

    std::string &value = map[42];

    REQUIRE(map.contains(42));
    REQUIRE(map.size() == 1);
    REQUIRE(value.empty());

    value = "answer";

    REQUIRE(map.at(42) == "answer");
}

TEST_CASE("JobMap maintains ordering after insertion and removal", "[core][container][map][edge]")
{
    JobMap<int, int> map;

    map.insert(50, 500);
    map.insert(10, 100);
    map.insert(40, 400);
    map.insert(20, 200);
    map.insert(30, 300);

    REQUIRE(map.remove(30));
    map.insert(25, 250);

    const int expected[]{
        10,
        20,
        25,
        40,
        50
    };

    std::size_t index = 0;

    for (const auto &[key, value] : map) {
        REQUIRE(index < std::size(expected));
        REQUIRE(key == expected[index]);

        ++index;
    }

    REQUIRE(index == std::size(expected));
}