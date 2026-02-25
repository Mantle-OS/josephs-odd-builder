#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>


#include <real_type.h>
#include <job_logger.h>

#include <aligned_allocator.h>
#include <ai_weights.h>
using Catch::Approx;
using namespace job::core;
using namespace job::ai::cords;

TEST_CASE("AiWeights data is 64-byte aligned", "[allocator][alignment]")
{
    AiWeights w(123456);

    auto *ptr = w.data();
    REQUIRE(ptr != nullptr);

    auto addr = reinterpret_cast<std::uintptr_t>(ptr);
    REQUIRE(addr % 64 == 0);
}

TEST_CASE("AiWeights preserves alignment after resize and push_back", "[allocator][growth]")
{
    AiWeights w;
    w.resize(10);
    auto addr1 = reinterpret_cast<std::uintptr_t>(w.data());
    REQUIRE(addr1 % 64 == 0);

    // Force reallocation
    for (int i = 0; i < 10'000; ++i)
        w.push_back(static_cast<float>(i));

    auto addr2 = reinterpret_cast<std::uintptr_t>(w.data());
    REQUIRE(addr2 % 64 == 0);
}

TEST_CASE("AiWeights copy and move semantics work", "[allocator][copy-move]")
{
    AiWeights w1(128, 1.0f);
    auto addr = reinterpret_cast<std::uintptr_t>(w1.data());
    REQUIRE(addr % 64 == 0);

    AiWeights w2 = w1;
    REQUIRE(w2.size() == w1.size());

    for (std::size_t i = 0; i < w2.size(); ++i)
        REQUIRE(w2[i] == 1.0f);

    auto addr2 = reinterpret_cast<std::uintptr_t>(w2.data());
    REQUIRE(addr2 % 64 == 0);

    AiWeights w3 = std::move(w1);
    REQUIRE(w3.size() == w2.size());
    auto addr3 = reinterpret_cast<std::uintptr_t>(w3.data());
    REQUIRE(addr3 % 64 == 0);
}



