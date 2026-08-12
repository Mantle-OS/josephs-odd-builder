#include <catch2/catch_all.hpp>

#include <job_list.h>

#include <memory>
#include <string>

using namespace job::core;


// 1
// How do I use JobList?

TEST_CASE("JobList stores values in insertion order", "[core][container][list]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(30);

    REQUIRE(list.size() == 3);
    REQUIRE(list.count() == 3);

    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
    REQUIRE(list[2] == 30);
}

TEST_CASE("JobList supports append prepend and insert", "[core][container][list]")
{
    JobList<int> list;

    list.append(20);
    list.append(30);
    list.prepend(10);
    list.insert(3, 40);

    REQUIRE(list.size() == 4);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
    REQUIRE(list[2] == 30);
    REQUIRE(list[3] == 40);
}

TEST_CASE("JobList supports indexed access", "[core][container][list]")
{
    JobList<std::string> list;

    list.append("Joseph");
    list.append("Odd");
    list.append("Builder");

    REQUIRE(list.at(0) == "Joseph");
    REQUIRE(list.at(1) == "Odd");
    REQUIRE(list.at(2) == "Builder");

    list[1] = "Odd's";

    REQUIRE(list[1] == "Odd's");
}

TEST_CASE("JobList exposes first and last values", "[core][container][list]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(30);

    REQUIRE(list.first() == 10);
    REQUIRE(list.last() == 30);

    list.first() = 1;
    list.last() = 3;

    REQUIRE(list[0] == 1);
    REQUIRE(list[2] == 3);
}

TEST_CASE("JobList can find contained values", "[core][container][list]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(30);

    REQUIRE(list.contains(10));
    REQUIRE(list.contains(20));
    REQUIRE(list.contains(30));
    REQUIRE_FALSE(list.contains(40));

    REQUIRE(list.indexOf(10) == 0);
    REQUIRE(list.indexOf(20) == 1);
    REQUIRE(list.indexOf(30) == 2);

    // Current contract: not-found returns size().
    REQUIRE(list.indexOf(40) == list.size());
}

TEST_CASE("JobList can remove matching values", "[core][container][list]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(10);
    list.append(30);
    list.append(10);

    REQUIRE(list.removeFirst(10));

    REQUIRE(list.size() == 4);
    REQUIRE(list[0] == 20);
    REQUIRE(list[1] == 10);
    REQUIRE(list[2] == 30);
    REQUIRE(list[3] == 10);

    REQUIRE(list.removeLast(10));

    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == 20);
    REQUIRE(list[1] == 10);
    REQUIRE(list[2] == 30);

    REQUIRE(list.removeAll(10) == 1);

    REQUIRE(list.size() == 2);
    REQUIRE(list[0] == 20);
    REQUIRE(list[1] == 30);
}

TEST_CASE("JobList can remove values by index", "[core][container][list]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(30);

    list.removeAt(1);

    REQUIRE(list.size() == 2);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 30);
}

TEST_CASE("JobList take operations remove and return values", "[core][container][list]")
{
    JobList<std::string> list;

    list.append("one");
    list.append("two");
    list.append("three");
    list.append("four");

    REQUIRE(list.takeAt(1) == "two");

    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == "one");
    REQUIRE(list[1] == "three");
    REQUIRE(list[2] == "four");

    REQUIRE(list.takeFirst() == "one");
    REQUIRE(list.takeLast() == "four");

    REQUIRE(list.size() == 1);
    REQUIRE(list.first() == "three");
}

TEST_CASE("JobList supports range based iteration", "[core][container][list]")
{
    JobList<int> list;

    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);

    int sum = 0;

    for (const int value : list)
        sum += value;

    REQUIRE(sum == 10);
}

TEST_CASE("JobList reserve exposes vector capacity semantics", "[core][container][list]")
{
    JobList<int> list;

    list.reserve(128);

    REQUIRE(list.capacity() >= 128);

    list.append(42);

    REQUIRE(list.capacity() >= 128);
    REQUIRE(list.first() == 42);
}


// 2
// Edge cases and invariants

TEST_CASE("A default JobList is empty", "[core][container][list][edge]")
{
    const JobList<int> list;

    REQUIRE(list.isEmpty());
    REQUIRE(list.size() == 0);
    REQUIRE(list.count() == 0);

    REQUIRE(list.begin() == list.end());
    REQUIRE(list.constBegin() == list.constEnd());
}

TEST_CASE("JobList clear removes every value", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(30);

    list.clear();

    REQUIRE(list.isEmpty());
    REQUIRE(list.size() == 0);
    REQUIRE(list.count() == 0);
}

TEST_CASE("JobList remove operations report missing values", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);

    REQUIRE_FALSE(list.removeFirst(30));
    REQUIRE_FALSE(list.removeLast(30));
    REQUIRE(list.removeAll(30) == 0);

    REQUIRE(list.size() == 2);
}

TEST_CASE("JobList const access returns stored values", "[core][container][list]")
{
    JobList<int> mutableList;

    mutableList.append(10);
    mutableList.append(20);
    mutableList.append(30);

    const JobList<int> &list = mutableList;

    REQUIRE(list.at(0) == 10);
    REQUIRE(list[1] == 20);
    REQUIRE(list.first() == 10);
    REQUIRE(list.last() == 30);

    auto it = list.constBegin();

    REQUIRE(*it == 10);
    ++it;
    REQUIRE(*it == 20);
}

TEST_CASE("JobList supports move only values", "[core][container][list]")
{
    JobList<std::unique_ptr<int>> list;

    list.append(std::make_unique<int>(10));
    list.prepend(std::make_unique<int>(5));
    list.insert(1, std::make_unique<int>(7));

    REQUIRE(list.size() == 3);
    REQUIRE(*list[0] == 5);
    REQUIRE(*list[1] == 7);
    REQUIRE(*list[2] == 10);

    auto value = list.takeAt(1);

    REQUIRE(value);
    REQUIRE(*value == 7);

    REQUIRE(list.size() == 2);
    REQUIRE(*list[0] == 5);
    REQUIRE(*list[1] == 10);
}


TEST_CASE("JobList at throws when index is out of range", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);

    REQUIRE_THROWS_AS(list.at(1), std::out_of_range);
    REQUIRE_THROWS_AS(list.at(100), std::out_of_range);
}

TEST_CASE("Const JobList at throws when index is out of range", "[core][container][list][edge]")
{
    JobList<int> mutableList;

    mutableList.append(10);

    const JobList<int> &list = mutableList;

    REQUIRE_THROWS_AS(list.at(1), std::out_of_range);
}

TEST_CASE("JobList removeAt throws when index is out of range", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);

    REQUIRE_THROWS_AS(list.removeAt(2), std::out_of_range);
    REQUIRE_THROWS_AS(list.removeAt(100), std::out_of_range);

    REQUIRE(list.size() == 2);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
}

TEST_CASE("JobList takeAt throws when index is out of range", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);

    REQUIRE_THROWS_AS(list.takeAt(2), std::out_of_range);
    REQUIRE_THROWS_AS(list.takeAt(100), std::out_of_range);

    REQUIRE(list.size() == 2);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
}

TEST_CASE("JobList takeFirst throws when list is empty", "[core][container][list][edge]")
{
    JobList<int> list;

    REQUIRE_THROWS_AS(list.takeFirst(), std::out_of_range);
    REQUIRE(list.isEmpty());
}

TEST_CASE("JobList takeLast throws when list is empty", "[core][container][list][edge]")
{
    JobList<int> list;

    REQUIRE_THROWS_AS(list.takeLast(), std::out_of_range);
    REQUIRE(list.isEmpty());
}


TEST_CASE("JobList insert accepts index equal to size", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);

    list.insert(list.size(), 30);

    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
    REQUIRE(list[2] == 30);
}



TEST_CASE("JobList insert throws when index is beyond size", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);

    REQUIRE_THROWS_AS(
        list.insert(list.size() + 1, 30),
        std::out_of_range
        );

    REQUIRE(list.size() == 2);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
}


TEST_CASE("JobList rvalue insert throws when index is beyond size", "[core][container][list][edge]")
{
    JobList<std::unique_ptr<int>> list;

    list.append(std::make_unique<int>(10));

    REQUIRE_THROWS_AS(
        list.insert(
            list.size() + 1,
            std::make_unique<int>(20)
            ),
        std::out_of_range
        );

    REQUIRE(list.size() == 1);
    REQUIRE(*list.first() == 10);
}

TEST_CASE("JobList remains unchanged after an out of range operation", "[core][container][list][edge]")
{
    JobList<int> list;

    list.append(10);
    list.append(20);
    list.append(30);

    REQUIRE_THROWS_AS(list.takeAt(42), std::out_of_range);

    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == 10);
    REQUIRE(list[1] == 20);
    REQUIRE(list[2] == 30);
}


