#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include <job_ggml.h>

using namespace job::ggml;

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("JobGgml exposes initialized GGML runtime information",
          "[ggml][usage][runtime]")
{
    JobGgml ggml;

    REQUIRE(ggml.isValid());

    REQUIRE_FALSE(ggml.version().empty());
    REQUIRE_FALSE(ggml.commit().empty());

    REQUIRE(ggml.deviceManager() != nullptr);
    REQUIRE(ggml.gguf() != nullptr);
}

TEST_CASE("JobGgml exposes GGML runtime timing",
          "[ggml][usage][time]")
{
    JobGgml ggml;

    const std::int64_t beforeUs = ggml.timeUs();
    const std::int64_t beforeMs = ggml.timeMs();

    REQUIRE(beforeUs >= 0);
    REQUIRE(beforeMs >= 0);

    const std::int64_t afterUs = ggml.timeUs();
    const std::int64_t afterMs = ggml.timeMs();

    REQUIRE(afterUs >= beforeUs);
    REQUIRE(afterMs >= beforeMs);
}

TEST_CASE("JobGgml exposes GGML cycle timing information",
          "[ggml][usage][cycles]")
{
    JobGgml ggml;

    const std::int64_t cyclesBefore = ggml.cycles();
    const std::int64_t cyclesAfter  = ggml.cycles();

    REQUIRE(cyclesBefore >= 0);
    REQUIRE(cyclesAfter >= cyclesBefore);
    REQUIRE(ggml.cyclesPerMs() > 0);
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("JobGgml runtime identity remains stable for its lifetime",
          "[ggml][edge][runtime]")
{
    JobGgml ggml;

    const std::string version = ggml.version();
    const std::string commit  = ggml.commit();

    REQUIRE_FALSE(version.empty());
    REQUIRE_FALSE(commit.empty());

    CHECK(ggml.version() == version);
    CHECK(ggml.commit() == commit);
}

TEST_CASE("JobGgml can initialize without scanning devices",
          "[ggml][edge][devices]")
{
    JobGgml ggml{false};

    REQUIRE(ggml.deviceManager() != nullptr);
    REQUIRE(ggml.gguf() != nullptr);

    REQUIRE_FALSE(ggml.version().empty());
    REQUIRE_FALSE(ggml.commit().empty());
}